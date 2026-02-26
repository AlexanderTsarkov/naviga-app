#include "app/app_services.h"

#include <cstdio>
#include <cstring>

#include "domain/node_table.h"
#include "hw_profile.h"
#include "platform/arduino_clock.h"
#include "platform/provisioning_adapter.h"
#include "platform/arduino_gpio.h"
#include "platform/arduino_logger.h"
#include "platform/device_id.h"
#include "platform/device_id_provider.h"
#include "platform/e220_radio.h"
#include "platform/gnss_ubx_uart_io.h"
#include "platform/log_export_uart.h"
#include "platform/naviga_storage.h"
#include "platform/timebase.h"
#include "services/gnss_scenario_override.h"
#include "services/gnss_stub_service.h"
#include "services/gnss_ublox_service.h"
#include "services/self_update_policy.h"
#include "utils/geo_utils.h"

namespace naviga {

namespace {

constexpr const char* kFirmwareVersion = "ootb-74-m1-runtime";

// Exactly one GNSS provider must be selected at compile-time.
#if defined(GNSS_PROVIDER_STUB) && defined(GNSS_PROVIDER_UBLOX)
#error "Select exactly one GNSS provider: GNSS_PROVIDER_STUB or GNSS_PROVIDER_UBLOX"
#elif !defined(GNSS_PROVIDER_STUB) && !defined(GNSS_PROVIDER_UBLOX)
#error "No GNSS provider selected. Define GNSS_PROVIDER_STUB or GNSS_PROVIDER_UBLOX"
#endif

#if defined(GNSS_PROVIDER_STUB)
GnssStubService gnss_provider_;
constexpr const char* kGnssProviderName = "STUB";
#elif defined(GNSS_PROVIDER_UBLOX)
GnssUbloxService gnss_provider_;
platform::GnssUbxUartIo gnss_ubx_io_{1};
constexpr const char* kGnssProviderName = "UBLOX";
#if GNSS_UBLOX_DIAG
constexpr uint32_t kGnssDiagPeriodMs = 2000U;
uint32_t gnss_diag_next_ms = 0;
#endif
#endif

SelfUpdatePolicy self_policy;
E220Radio* radio = nullptr;

platform::ArduinoClock clock_;
platform::ArduinoLogger logger_;
platform::DefaultDeviceIdProvider device_id_provider_;

constexpr const char* kLogTag = "app";

inline void log_line(const char* msg) {
  logger_.log(platform::LogLevel::kInfo, kLogTag, msg);
}

inline void log_kv(const char* key, const char* value) {
  char buffer[128] = {0};
  std::snprintf(buffer, sizeof(buffer), "%s%s", key, value ? value : "-");
  log_line(buffer);
}

inline void log_kv_u32(const char* key, uint32_t value) {
  char buffer[128] = {0};
  std::snprintf(buffer, sizeof(buffer), "%s%lu", key, static_cast<unsigned long>(value));
  log_line(buffer);
}

#if defined(GNSS_PROVIDER_UBLOX)
inline const char* fix_state_to_cstr(GNSSFixState state) {
  switch (state) {
    case GNSSFixState::FIX_2D:
      return "FIX_2D";
    case GNSSFixState::FIX_3D:
      return "FIX_3D";
    case GNSSFixState::NO_FIX:
    default:
      return "NO_FIX";
  }
}
#endif

inline void write_i32_le(uint8_t* out, int32_t value) {
  const uint32_t raw = static_cast<uint32_t>(value);
  out[0] = static_cast<uint8_t>(raw & 0xFF);
  out[1] = static_cast<uint8_t>((raw >> 8) & 0xFF);
  out[2] = static_cast<uint8_t>((raw >> 16) & 0xFF);
  out[3] = static_cast<uint8_t>((raw >> 24) & 0xFF);
}

void extract_bt_short(const char* mac, char* out, size_t out_len) {
  if (!out || out_len == 0) {
    return;
  }
  if (!mac) {
    std::snprintf(out, out_len, "----");
    return;
  }
  const size_t len = std::strlen(mac);
  if (len < 4) {
    std::snprintf(out, out_len, "%s", mac);
    return;
  }
  std::snprintf(out, out_len, "%s", mac + (len - 4));
}

void app_instrumentation_log(const char* line, void* ctx) {
  if (ctx) {
    static_cast<AppServices*>(ctx)->log_instrumentation_line(line);
  }
}

}  // namespace

AppServices::AppServices() : provisioning_(new ProvisioningAdapter()) {}

AppServices::~AppServices() {
  delete provisioning_;
  provisioning_ = nullptr;
}

void AppServices::init() {
  last_heartbeat_ms_ = 0;
  last_summary_ms_ = 0;
  fix_logged_ = false;

  // --- Phase A: HW bring-up (module boot configs) per boot_pipeline_v0 ---
  const auto& profile = get_hw_profile();
  platform::configure_input_pullup(profile.pins.role_pin);
  clock_.sleep_ms(30);
  role_ = platform::read_digital(profile.pins.role_pin) ? RadioRole::RESP : RadioRole::INIT;

  log_line("");
  log_line("=== Naviga OOTB skeleton ===");
  log_kv("fw: ", kFirmwareVersion);
  log_kv("hw_profile: ", profile.name);
  log_kv("gnss_provider: ", kGnssProviderName);
  log_kv("role: ", role_ == RadioRole::INIT ? "INIT" : "RESP");

  const platform::DeviceId device_id = device_id_provider_.get();
  const uint64_t full_id = full_id_from_mac(device_id.bytes);
  short_id_ = domain::NodeTable::compute_short_id(full_id);
#if defined(GNSS_PROVIDER_UBLOX)
  gnss_provider_.set_io(&gnss_ubx_io_);
#endif
  gnss_provider_.init(full_id);
  constexpr uint32_t kGnssVerifyTimeoutMs = 500U;
  const bool first_verify = gnss_provider_.verify_on_boot(kGnssVerifyTimeoutMs);
  if (first_verify) {
    log_line("GNSS boot: ok");
  } else {
    gnss_provider_.init(full_id);
    const bool second_verify = gnss_provider_.verify_on_boot(kGnssVerifyTimeoutMs);
    log_line(second_verify ? "GNSS boot: repaired" : "GNSS boot: failed");
  }
  self_policy.init();
#if defined(GNSS_PROVIDER_UBLOX) && GNSS_UBLOX_DIAG
  gnss_diag_next_ms = 0;
#endif

#if defined(GNSS_PROVIDER_UBLOX)
  GnssUbloxDiagEvents startup_events{};
  if (gnss_provider_.take_diag_events(&startup_events) && startup_events.cfg_nav_pvt_sent) {
    log_line("GNSS_UBX cfg: enable NAV-PVT");
  }
#endif

  static E220Radio radio_instance(profile.pins);
  radio = &radio_instance;
  const bool radio_ready = radio->begin();
  {
    char buf[80] = {0};
    switch (radio->last_boot_config_result()) {
      case E220BootConfigResult::Ok:
        log_line("E220 boot: config ok");
        break;
      case E220BootConfigResult::Repaired:
        std::snprintf(buf, sizeof(buf), "E220 boot: repaired (%s)", radio->last_boot_config_message());
        log_line(buf);
        break;
      case E220BootConfigResult::RepairFailed:
        std::snprintf(buf, sizeof(buf), "E220 boot: repair failed (%s)", radio->last_boot_config_message());
        log_line(buf);
        break;
    }
  }
  provisioning_->set_radio_boot_info(static_cast<int>(radio->last_boot_config_result()),
                                    radio->last_boot_config_message());

  // --- Phase B: Provision role + radio profile (boot_pipeline_v0) ---
  // Cadence params from current role profile record in flash (#289); pointers for role/radio id (display, consistency).
  constexpr uint32_t kDefaultRoleId = 0;
  constexpr uint32_t kDefaultRadioProfileId = 0;
  PersistedPointers ptrs{};
  const bool loaded = load_pointers(&ptrs);
  bool use_persisted = loaded && ptrs.has_current_role && ptrs.has_current_radio;
  uint32_t effective_role_id = kDefaultRoleId;
  uint32_t effective_radio_id = kDefaultRadioProfileId;
  if (use_persisted) {
    effective_role_id = ptrs.current_role_id;
    effective_radio_id = ptrs.current_radio_profile_id;
    if (effective_role_id > 2) { effective_role_id = kDefaultRoleId; use_persisted = false; }
    if (effective_radio_id != 0) { effective_radio_id = kDefaultRadioProfileId; use_persisted = false; }
  }
  const bool role_changed = loaded && ptrs.has_current_role && (effective_role_id != ptrs.current_role_id);
  const bool radio_changed = loaded && ptrs.has_current_radio && (effective_radio_id != ptrs.current_radio_profile_id);
  const uint32_t new_previous_role = role_changed ? ptrs.current_role_id : ptrs.previous_role_id;
  const uint32_t new_previous_radio = radio_changed ? ptrs.current_radio_profile_id : ptrs.previous_radio_profile_id;

  // Cadence from current profile record in flash (registry); fallback to default when missing/invalid (#289).
  RoleProfileRecord profile_record{};
  bool profile_valid = false;
  (void)load_current_role_profile_record(&profile_record, &profile_valid);
  if (!profile_valid) {
    get_ootb_role_profile(effective_role_id, &profile_record);
    (void)save_current_role_profile_record(profile_record);
  }
  const uint16_t effective_interval_s = profile_record.min_interval_sec;
  const uint8_t effective_max_silence_10s = profile_record.max_silence_10s;
  const double effective_min_distance_m = static_cast<double>(profile_record.min_displacement_m);

  const uint8_t effective_channel = (effective_radio_id == 0) ? 1 : 1;  // V1-A: only profile 0 → channel 1
  const uint32_t min_interval_ms = static_cast<uint32_t>(effective_interval_s) * 1000U;
  const uint32_t max_silence_ms = static_cast<uint32_t>(effective_max_silence_10s) * 10U * 1000U;

  save_pointers(effective_role_id, effective_radio_id, new_previous_role, new_previous_radio);
  {
    const bool fallback = loaded && !use_persisted;
    char buf[80] = {0};
    std::snprintf(buf, sizeof(buf), "Phase B: role=%lu profile=%lu source=%s%s",
                  static_cast<unsigned long>(effective_role_id),
                  static_cast<unsigned long>(effective_radio_id),
                  use_persisted ? "persisted" : "default",
                  fallback ? " (invalid persisted id)" : "");
    log_line(buf);
  }
  {
    char buf[80] = {0};
    std::snprintf(buf, sizeof(buf), "Applied: interval_s=%u maxSilence10s=%u channel=%u (role=%lu radio=%lu) cadence=registry",
                  static_cast<unsigned>(effective_interval_s),
                  static_cast<unsigned>(effective_max_silence_10s),
                  static_cast<unsigned>(effective_channel),
                  static_cast<unsigned long>(effective_role_id),
                  static_cast<unsigned long>(effective_radio_id));
    log_line(buf);
  }

  self_policy.set_max_silence_ms(max_silence_ms);
  self_policy.set_min_time_ms(min_interval_ms);
  self_policy.set_min_distance_m(effective_min_distance_m);

  // --- Phase C: Start comms — wire runtime; tick() runs Alive/Beacon cadence ---
  format_short_id_hex(short_id_, short_id_hex_, sizeof(short_id_hex_));
  format_mac_colon_hex(mac_bytes, mac_hex_, sizeof(mac_hex_));
  extract_bt_short(mac_hex_, bt_short_, sizeof(bt_short_));
  oled_.init(profile);

  char full_id_hex[20] = {0};
  char mac_hex[13] = {0};
  char short_id_hex[5] = {0};
  format_full_id_u64_hex(full_id, full_id_hex, sizeof(full_id_hex));
  format_full_id_mac_hex(device_id.bytes, mac_hex, sizeof(mac_hex));
  format_short_id_hex(short_id_, short_id_hex, sizeof(short_id_hex));

  log_line("=== Node identity ===");
  log_kv("full_id_u64: ", full_id_hex);
  log_kv("full_id_mac: ", mac_hex);
  log_kv("short_id: ", short_id_hex);

  protocol::DeviceInfoModel device_info{};
  device_info.format_ver = 1;
  device_info.ble_schema_ver = 1;
  device_info.radio_proto_ver = 0;
  device_info.include_radio_proto_ver = true;
  device_info.node_id = full_id;
  device_info.short_id = short_id_;
  device_info.device_type = 1;
  device_info.firmware_version = kFirmwareVersion;
  device_info.radio_module_model = "E220";
  device_info.band_id = 433;
  device_info.channel_min = 0;
  device_info.channel_max = 255;
  device_info.network_mode = 0;
  device_info.channel_id = effective_channel;
  device_info.public_channel_id = effective_channel;
  device_info.capabilities = 0;

  runtime_.init(full_id, short_id_, uptime_ms(), device_info, radio, radio_ready,
                radio->rssi_available(), effective_interval_s, min_interval_ms, max_silence_ms,
                &event_logger_, nullptr);
  provisioning_->set_instrumentation_flag(&instrumentation_enabled_);
  provisioning_->set_gnss_override(&gnss_override_);
  runtime_.set_instrumentation_logger(app_instrumentation_log, this);
}

void AppServices::log_instrumentation_line(const char* line) {
  if (instrumentation_enabled_ && line) {
    log_line(line);
  }
}

void AppServices::tick(uint32_t now_ms) {
  provisioning_->tick(now_ms);

#if defined(GNSS_PROVIDER_UBLOX)
  GnssUbloxDiagEvents ubx_events{};
  if (gnss_provider_.take_diag_events(&ubx_events)) {
    if (ubx_events.hint_nmea) {
      log_line("GNSS_UBX hint=NMEA (parser expects UBX NAV-PVT)");
    }
    if (ubx_events.hint_no_data) {
      log_line("GNSS_UBX no UART data (check power, GND, TX->RX16, baud)");
    }
  }
#endif

#if defined(GNSS_PROVIDER_UBLOX) && GNSS_UBLOX_DIAG
  if (gnss_diag_next_ms == 0 || static_cast<int32_t>(now_ms - gnss_diag_next_ms) >= 0) {
    GnssUbloxDiag diag{};
    if (gnss_provider_.get_diag(&diag)) {
      char buffer[160] = {0};
      std::snprintf(buffer, sizeof(buffer),
                    "GNSS_UBX rx=%lu ok=%lu bad=%lu last=%lu fix=%s lat=%ld lon=%ld",
                    static_cast<unsigned long>(diag.bytes_rx),
                    static_cast<unsigned long>(diag.frames_ok),
                    static_cast<unsigned long>(diag.frames_bad_ck),
                    static_cast<unsigned long>(diag.last_frame_ms),
                    fix_state_to_cstr(diag.fix_state),
                    static_cast<long>(diag.lat_e7),
                    static_cast<long>(diag.lon_e7));
      log_line(buffer);
    }
    gnss_diag_next_ms = now_ms + kGnssDiagPeriodMs;
  }
#endif

  bool have_snapshot = false;
  GnssSnapshot snapshot{};
  if (gnss_override_.get_snapshot_if_active(&snapshot, now_ms)) {
    have_snapshot = true;
    if (last_gnss_override_log_ms_ == 0 || (now_ms - last_gnss_override_log_ms_) >= 5000U) {
      last_gnss_override_log_ms_ = now_ms;
      char buf[80] = {0};
      if (snapshot.pos_valid) {
        std::snprintf(buf, sizeof(buf), "GNSS override: FIX lat_e7=%ld lon_e7=%ld",
                      static_cast<long>(snapshot.lat_e7), static_cast<long>(snapshot.lon_e7));
      } else {
        std::snprintf(buf, sizeof(buf), "GNSS override: NO_FIX");
      }
      log_line(buf);
    }
  } else if (gnss_provider_.tick(now_ms) && gnss_provider_.get_snapshot(&snapshot)) {
    have_snapshot = true;
  }
  if (have_snapshot) {
    if (snapshot.pos_valid && !fix_logged_) {
      log_line("GNSS: FIX acquired");
      fix_logged_ = true;
    }

    const SelfUpdateDecision decision = self_policy.evaluate(now_ms, snapshot);
    const uint16_t pos_age_s = snapshot.pos_valid && snapshot.last_fix_ms > 0
                                   ? static_cast<uint16_t>((now_ms - snapshot.last_fix_ms) / 1000U)
                                   : 0;
    if (decision.reason != SelfUpdateReason::NONE) {
      self_policy.commit(now_ms, snapshot);
      runtime_.set_self_position(snapshot.pos_valid,
                                 snapshot.lat_e7,
                                 snapshot.lon_e7,
                                 pos_age_s,
                                 snapshot.fix_state,
                                 now_ms);
      runtime_.set_allow_core_send(true);  // minDisplacement: allow CORE at next TX
      uint8_t payload[8] = {};
      write_i32_le(payload, snapshot.lat_e7);
      write_i32_le(payload + 4, snapshot.lon_e7);
      event_logger_.log(now_ms, domain::LogEventId::NODETABLE_UPDATE,
                        domain::LogLevel::kInfo, payload, sizeof(payload));

      const char* reason_str = "NONE";
      switch (decision.reason) {
        case SelfUpdateReason::DISTANCE:
          reason_str = "DISTANCE";
          break;
        case SelfUpdateReason::MAX_SILENCE:
          reason_str = "MAX_SILENCE";
          break;
        case SelfUpdateReason::FIRST_FIX:
          reason_str = "FIRST_FIX";
          break;
        case SelfUpdateReason::NONE:
        default:
          reason_str = "NONE";
          break;
      }

      char buffer[160] = {0};
      std::snprintf(buffer, sizeof(buffer),
                    "SELF_POS: updated reason=%s lat_e7=%ld lon_e7=%ld d=%.2f dt=%lu",
                    reason_str, static_cast<long>(snapshot.lat_e7),
                    static_cast<long>(snapshot.lon_e7),
                    static_cast<double>(decision.distance_m),
                    static_cast<unsigned long>(decision.dt_ms));
      log_line(buffer);
    } else if (!snapshot.pos_valid) {
      runtime_.set_self_position(false, 0, 0, 0, GNSSFixState::NO_FIX, now_ms);
    }
  } else if (gnss_override_.enabled()) {
    // Override active but get_snapshot_if_active returned false (shouldn't happen)
    runtime_.set_self_position(false, 0, 0, 0, GNSSFixState::NO_FIX, now_ms);
  }

  runtime_.tick(now_ms);
  OledStatusData oled_data{};
  oled_data.bt_short = bt_short_;
  oled_data.firmware_version = kFirmwareVersion;
  oled_data.ble_connected = runtime_.ble_connected();
  oled_data.nodes_seen = runtime_.node_count();
  oled_data.geo_seq = runtime_.geo_seq();
  oled_data.uptime_ms = now_ms;
  oled_.update(now_ms, runtime_.stats(), oled_data);

  if ((now_ms - last_heartbeat_ms_) >= 1000U) {
    last_heartbeat_ms_ = now_ms;
    log_kv_u32("tick: ", now_ms);
  }

  if ((now_ms - last_summary_ms_) >= 5000U) {
    last_summary_ms_ = now_ms;
    char buffer[128] = {0};
    std::snprintf(buffer, sizeof(buffer), "nodetable: size=%lu",
                  static_cast<unsigned long>(runtime_.node_count()));
    log_line(buffer);
    if (instrumentation_enabled_ &&
        (last_peer_dump_ms_ == 0 || (now_ms - last_peer_dump_ms_) >= 3000U)) {
      last_peer_dump_ms_ = now_ms;
      runtime_.log_peer_dump(now_ms);
    }
    platform::drain_logs_uart(event_logger_);
  }
}

} // namespace naviga
