#include "app/app_services.h"

#include <cstdio>

#include "domain/node_table.h"
#include "hw_profile.h"
#include "platform/arduino_clock.h"
#include "platform/arduino_gpio.h"
#include "platform/arduino_logger.h"
#include "platform/device_id.h"
#include "platform/device_id_provider.h"
#include "platform/e220_radio.h"
#include "platform/log_export_uart.h"
#include "platform/timebase.h"
#include "services/gnss_stub_service.h"
#include "services/self_update_policy.h"
#include "utils/geo_utils.h"

namespace naviga {

namespace {

constexpr const char* kFirmwareVersion = "ootb-74-m1-runtime";

GnssStubService gnss_stub;
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

inline void write_i32_le(uint8_t* out, int32_t value) {
  const uint32_t raw = static_cast<uint32_t>(value);
  out[0] = static_cast<uint8_t>(raw & 0xFF);
  out[1] = static_cast<uint8_t>((raw >> 8) & 0xFF);
  out[2] = static_cast<uint8_t>((raw >> 16) & 0xFF);
  out[3] = static_cast<uint8_t>((raw >> 24) & 0xFF);
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
  log_kv("role: ", role_ == RadioRole::INIT ? "INIT" : "RESP");

  const platform::DeviceId device_id = device_id_provider_.get();
  const uint64_t full_id = full_id_from_mac(device_id.bytes);
  const uint16_t short_id = domain::NodeTable::compute_short_id(full_id);
  gnss_stub.init(full_id);
  self_policy.init();

  static E220Radio radio_instance(profile.pins);
  radio = &radio_instance;
  const bool radio_ready = radio->begin();
  oled_.init(profile, kFirmwareVersion, role_);

  char full_id_hex[20] = {0};
  char mac_hex[13] = {0};
  char short_id_hex[5] = {0};
  format_full_id_u64_hex(full_id, full_id_hex, sizeof(full_id_hex));
  format_full_id_mac_hex(device_id.bytes, mac_hex, sizeof(mac_hex));
  format_short_id_hex(short_id, short_id_hex, sizeof(short_id_hex));

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
  device_info.short_id = short_id;
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

  runtime_.init(full_id, short_id, uptime_ms(), device_info, radio, radio_ready,
                radio->rssi_available(), &event_logger_);
}

void AppServices::tick(uint32_t now_ms) {
  if (gnss_stub.tick(now_ms)) {
    const GnssSample sample = gnss_stub.latest();
    if (sample.has_fix && !fix_logged_) {
      log_line("GNSS: FIX acquired");
      fix_logged_ = true;
    }

    const SelfUpdateDecision decision = self_policy.evaluate(now_ms, sample);
    if (decision.reason != SelfUpdateReason::NONE) {
      self_policy.commit(now_ms, sample);
      runtime_.set_self_position(sample.has_fix, sample.lat_e7, sample.lon_e7, 0, now_ms);
      uint8_t payload[8] = {};
      write_i32_le(payload, sample.lat_e7);
      write_i32_le(payload + 4, sample.lon_e7);
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
                    reason_str, static_cast<long>(sample.lat_e7),
                    static_cast<long>(sample.lon_e7),
                    static_cast<double>(decision.distance_m),
                    static_cast<unsigned long>(decision.dt_ms));
      log_line(buffer);
    } else if (!sample.has_fix) {
      runtime_.set_self_position(false, 0, 0, 0, now_ms);
    }
  }

  runtime_.tick(now_ms);
  oled_.update(now_ms, runtime_.stats());

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
