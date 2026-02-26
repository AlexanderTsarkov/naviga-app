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
  // Source policy (#298): use factory/base eFuse MAC only.
  // Interface MACs (BLE, BT) may differ across ESP32 variants and are not stable.
  uint8_t base_mac[6] = {0};
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
  // Delegate to canonical path: full_id → compute_short_id (6-byte LE CRC16 + reserved fix).
  // Callers that already hold a uint64_t node_id should use NodeTable::compute_short_id directly.
  const uint64_t node_id = full_id_from_mac(mac);
  // Inline the canonical algorithm here to avoid a domain dependency in platform layer.
  constexpr uint16_t kInit = 0xFFFF;
  constexpr uint16_t kPoly = 0x1021;
  uint8_t bytes[6] = {0};
  for (int i = 0; i < 6; ++i) {
    bytes[i] = static_cast<uint8_t>((node_id >> (8 * i)) & 0xFF);
  }
  uint16_t crc = kInit;
  for (size_t i = 0; i < 6; ++i) {
    crc ^= static_cast<uint16_t>(bytes[i]) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 0x8000) {
        crc = static_cast<uint16_t>((crc << 1) ^ kPoly);
      } else {
        crc <<= 1;
      }
    }
  }
  if (crc == 0x0000u || crc == 0xFFFFu) {
    return static_cast<uint16_t>(crc ^ 0x0001u);
  }
  return crc;
}

uint64_t full_id_from_mac(const uint8_t mac[6]) {
  if (!mac) {
    return 0;
  }
  // Domain NodeId: lower 48 bits = MAC48 (big-endian display order), upper 16 bits = 0x0000.
  // Per nodeid_policy_v0 §1.
  uint64_t id = 0;
  for (int i = 0; i < 6; ++i) {
    id = (id << 8) | mac[i];
  }
  return id & UINT64_C(0x0000FFFFFFFFFFFF);
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
