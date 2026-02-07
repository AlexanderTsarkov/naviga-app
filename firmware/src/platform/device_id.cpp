#include "platform/device_id.h"

#include <Arduino.h>
#include <cinttypes>
#include <cstring>

#include "esp_system.h"

namespace naviga {

namespace {

constexpr uint16_t kCrc16Init = 0xFFFF;
constexpr uint16_t kCrc16Poly = 0x1021;

uint16_t crc16_ccitt_false(const uint8_t* data, size_t len) {
  uint16_t crc = kCrc16Init;
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 0x8000) {
        crc = static_cast<uint16_t>((crc << 1) ^ kCrc16Poly);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

} // namespace

void get_device_mac_bytes(uint8_t out_mac[6]) {
  if (!out_mac) {
    return;
  }
  std::memset(out_mac, 0, 6);
  uint8_t base_mac[6] = {0};
#if defined(ESP_MAC_BLE)
  if (esp_read_mac(base_mac, ESP_MAC_BLE) == ESP_OK) {
    std::memcpy(out_mac, base_mac, 6);
    return;
  }
#endif
#if defined(ESP_MAC_BT)
  if (esp_read_mac(base_mac, ESP_MAC_BT) == ESP_OK) {
    std::memcpy(out_mac, base_mac, 6);
    return;
  }
#endif
  if (esp_efuse_mac_get_default(base_mac) == ESP_OK) {
    std::memcpy(out_mac, base_mac, 6);
  }
}

uint64_t get_device_full_id_u64() {
  uint8_t mac[6] = {0};
  get_device_mac_bytes(mac);
  return full_id_from_mac(mac);
}

uint16_t get_device_short_id_u16() {
  uint8_t mac[6] = {0};
  get_device_mac_bytes(mac);
  return short_id_from_mac(mac);
}

uint64_t full_id_from_mac(const uint8_t mac[6]) {
  if (!mac) {
    return 0;
  }
  uint64_t id = 0;
  // Canonical order: MAC bytes [0..5] map to hex string and full_id_u64.
  // full_id_u64 = 0x0000 <mac[0]..mac[5]> (big-endian packing into lower 6 bytes).
  for (int i = 0; i < 6; ++i) {
    id = (id << 8) | mac[i];
  }
  return id;
}

uint16_t short_id_from_mac(const uint8_t mac[6]) {
  if (!mac) {
    return 0;
  }
  // CRC16-CCITT-FALSE over the 6 MAC bytes.
  return crc16_ccitt_false(mac, 6);
}

void format_full_id_u64_hex(uint64_t full_id, char* out, size_t out_len) {
  if (!out || out_len == 0) {
    return;
  }
  std::snprintf(out, out_len, "0x%016" PRIX64, full_id);
}

void format_full_id_mac_hex(const uint8_t mac[6], char* out, size_t out_len) {
  if (!out || out_len == 0) {
    return;
  }
  if (!mac) {
    std::snprintf(out, out_len, "000000000000");
    return;
  }
  std::snprintf(out, out_len, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2],
                mac[3], mac[4], mac[5]);
}

void format_short_id_hex(uint16_t short_id, char* out, size_t out_len) {
  if (!out || out_len == 0) {
    return;
  }
  std::snprintf(out, out_len, "%04X", short_id);
}

void format_mac_colon_hex(const uint8_t mac[6], char* out, size_t out_len) {
  if (!out || out_len == 0) {
    return;
  }
  if (!mac) {
    std::snprintf(out, out_len, "00:00:00:00:00:00");
    return;
  }
  std::snprintf(out, out_len, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

} // namespace naviga
