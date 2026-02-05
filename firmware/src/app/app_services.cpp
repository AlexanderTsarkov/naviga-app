#include "app/app_services.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include "app/node_table.h"
#include "hw_profile.h"
#include "platform/device_id.h"
#include "platform/e220_radio.h"
#include "platform/timebase.h"
#include "services/ble_service.h"
#include "services/gnss_stub_service.h"
#include "services/self_update_policy.h"
#include "utils/geo_utils.h"

namespace naviga {

namespace {

constexpr const char* kFirmwareVersion = "ootb-21-radio-smoke";

NodeTable node_table;
BleService ble_service;
GnssStubService gnss_stub;
SelfUpdatePolicy self_policy;
E220Radio* radio = nullptr;

#ifdef ARDUINO
inline void log_print(const char* value) { Serial.print(value); }
inline void log_print(int32_t value) { Serial.print(value); }
inline void log_print(uint32_t value) { Serial.print(value); }
inline void log_print(double value, int digits) { Serial.print(value, digits); }
inline void log_println(const char* value) { Serial.println(value); }
inline void log_println(uint32_t value) { Serial.println(value); }
inline void log_println(int32_t value) { Serial.println(value); }
#else
inline void log_print(const char*) {}
inline void log_print(int32_t) {}
inline void log_print(uint32_t) {}
inline void log_print(double, int) {}
inline void log_println(const char*) {}
inline void log_println(uint32_t) {}
inline void log_println(int32_t) {}
#endif

} // namespace

void AppServices::init() {
  last_heartbeat_ms_ = 0;
  last_summary_ms_ = 0;
  fix_logged_ = false;

  const auto& profile = get_hw_profile();
  pinMode(profile.pins.role_pin, INPUT_PULLUP);
  delay(30);
  role_ = (digitalRead(profile.pins.role_pin) == LOW) ? RadioRole::INIT : RadioRole::RESP;

  log_println("");
  log_println("=== Naviga OOTB skeleton ===");
  log_print("fw: ");
  log_println(kFirmwareVersion);
  log_print("hw_profile: ");
  log_println(profile.name);
  log_print("role: ");
  log_println(role_ == RadioRole::INIT ? "INIT (PING)" : "RESP (PONG)");

  uint8_t mac[6] = {0};
  get_device_mac_bytes(mac);
  const uint64_t full_id = get_device_full_id_u64();
  const uint16_t short_id = get_device_short_id_u16();
  node_table.init_self(full_id, short_id, uptime_ms());
  gnss_stub.init(full_id);
  self_policy.init();

  static E220Radio radio_instance(profile.pins);
  radio = &radio_instance;
  const bool radio_ready = radio->begin();
  radio_smoke_.init(radio, role_, radio_ready, radio->rssi_available());
  oled_.init(profile, kFirmwareVersion, role_);

  char full_id_hex[20] = {0};
  char mac_hex[13] = {0};
  char short_id_hex[5] = {0};
  format_full_id_u64_hex(full_id, full_id_hex, sizeof(full_id_hex));
  format_full_id_mac_hex(mac, mac_hex, sizeof(mac_hex));
  format_short_id_hex(short_id, short_id_hex, sizeof(short_id_hex));

  log_println("=== Node identity ===");
  log_print("full_id_u64: ");
  log_println(full_id_hex);
  log_print("full_id_mac: ");
  log_println(mac_hex);
  log_print("short_id: ");
  log_println(short_id_hex);

  const DeviceInfoData device_info = {
      .fw_version = kFirmwareVersion,
      .hw_profile_name = profile.name,
      .band_id = 433,
      .public_channel_id = 1,
      .full_id_u64 = full_id,
      .short_id = short_id,
  };
  ble_service.init(device_info, &node_table);
}

void AppServices::tick(uint32_t now_ms) {
  if (gnss_stub.tick(now_ms)) {
    const GnssSample sample = gnss_stub.latest();
    if (sample.has_fix && !fix_logged_) {
      log_println("GNSS: FIX acquired");
      fix_logged_ = true;
    }

    const SelfUpdateDecision decision = self_policy.evaluate(now_ms, sample);
    if (decision.reason != SelfUpdateReason::NONE) {
      self_policy.commit(now_ms, sample);
      node_table.update_self_position(sample.lat_e7, sample.lon_e7, now_ms);

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

      log_print("SELF_POS: updated reason=");
      log_print(reason_str);
      log_print(" lat_e7=");
      log_print(sample.lat_e7);
      log_print(" lon_e7=");
      log_print(sample.lon_e7);
      log_print(" d=");
      log_print(decision.distance_m, 2);
      log_print(" dt=");
      log_println(decision.dt_ms);
    }
  }

  radio_smoke_.tick(now_ms);
  oled_.update(now_ms, radio_smoke_.stats());

  if ((now_ms - last_heartbeat_ms_) >= 1000U) {
    last_heartbeat_ms_ = now_ms;
    log_print("tick: ");
    log_println(now_ms);
  }

  if ((now_ms - last_summary_ms_) >= 5000U) {
    last_summary_ms_ = now_ms;
    uint8_t page_buffer[NodeTable::kEntryBytes * NodeTable::kDefaultPageSize] = {0};
    const size_t bytes = node_table.get_page(0, NodeTable::kDefaultPageSize, page_buffer,
                                             sizeof(page_buffer));
    const size_t page0_count = bytes / NodeTable::kEntryBytes;
    log_print("nodetable: size=");
    log_print(static_cast<uint32_t>(node_table.size()));
    log_print(" page0_count=");
    log_println(static_cast<uint32_t>(page0_count));
  }
}

} // namespace naviga
