#include "app/app_services.h"

#include <cstdio>
#include <cstring>

#include "domain/node_table.h"
#include "hw_profile.h"
#include "platform/arduino_clock.h"
#include "platform/arduino_gpio.h"
#include "platform/arduino_logger.h"
#include "platform/device_id.h"
#include "platform/device_id_provider.h"
#include "platform/e220_radio.h"
#include "platform/gnss_ubx_uart_io.h"
#include "platform/log_export_uart.h"
#include "platform/timebase.h"
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

} // namespace

void AppServices::init() {
  last_heartbeat_ms_ = 0;
  last_summary_ms_ = 0;
  fix_logged_ = false;

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
  uint8_t mac_bytes[6] = {0};
  get_device_mac_bytes(mac_bytes);
  const uint64_t full_id = full_id_from_mac(device_id.bytes);
  short_id_ = domain::NodeTable::compute_short_id(full_id);
#if defined(GNSS_PROVIDER_UBLOX)
  gnss_provider_.set_io(&gnss_ubx_io_);
#endif
  gnss_provider_.init(full_id);
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
  device_info.channel_id = 1;
  device_info.public_channel_id = 1;
  device_info.capabilities = 0;

  runtime_.init(full_id, short_id_, uptime_ms(), device_info, radio, radio_ready,
                radio->rssi_available(), &event_logger_, nullptr);
}

void AppServices::tick(uint32_t now_ms) {
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

  if (gnss_provider_.tick(now_ms)) {
    GnssSnapshot snapshot{};
    if (!gnss_provider_.get_snapshot(&snapshot)) {
      return;
    }
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
    platform::drain_logs_uart(event_logger_);
  }
}

} // namespace naviga
