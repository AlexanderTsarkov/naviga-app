#include "app/app_services.h"

#include <Arduino.h>

#include "app/node_table.h"
#include "hw_profile.h"
#include "platform/device_id.h"
#include "platform/timebase.h"
#include "services/ble_service.h"

namespace naviga {

namespace {

constexpr const char* kFirmwareVersion = "ootb-11.3-ble";

NodeTable node_table;
BleService ble_service;

} // namespace

void AppServices::init() {
  last_heartbeat_ms_ = 0;
  last_summary_ms_ = 0;

  const auto& profile = get_hw_profile();

  Serial.println();
  Serial.println("=== Naviga OOTB skeleton ===");
  Serial.print("fw: ");
  Serial.println(kFirmwareVersion);
  Serial.print("hw_profile: ");
  Serial.println(profile.name);

  uint8_t mac[6] = {0};
  get_device_mac_bytes(mac);
  const uint64_t full_id = get_device_full_id_u64();
  const uint16_t short_id = get_device_short_id_u16();
  node_table.init_self(full_id, short_id, uptime_ms());

  char full_id_hex[20] = {0};
  char mac_hex[13] = {0};
  char short_id_hex[5] = {0};
  format_full_id_u64_hex(full_id, full_id_hex, sizeof(full_id_hex));
  format_full_id_mac_hex(mac, mac_hex, sizeof(mac_hex));
  format_short_id_hex(short_id, short_id_hex, sizeof(short_id_hex));

  Serial.println("=== Node identity ===");
  Serial.print("full_id_u64: ");
  Serial.println(full_id_hex);
  Serial.print("full_id_mac: ");
  Serial.println(mac_hex);
  Serial.print("short_id: ");
  Serial.println(short_id_hex);

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
  if ((now_ms - last_heartbeat_ms_) >= 1000U) {
    last_heartbeat_ms_ = now_ms;
    Serial.print("tick: ");
    Serial.println(now_ms);
  }

  if ((now_ms - last_summary_ms_) >= 5000U) {
    last_summary_ms_ = now_ms;
    uint8_t page_buffer[NodeTable::kEntryBytes * NodeTable::kDefaultPageSize] = {0};
    const size_t bytes = node_table.get_page(0, NodeTable::kDefaultPageSize, page_buffer,
                                             sizeof(page_buffer));
    const size_t page0_count = bytes / NodeTable::kEntryBytes;
    Serial.print("nodetable: size=");
    Serial.print(node_table.size());
    Serial.print(" page0_count=");
    Serial.println(page0_count);
  }
}

} // namespace naviga
