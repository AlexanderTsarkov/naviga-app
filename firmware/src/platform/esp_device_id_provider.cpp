/**
 * @file esp_device_id_provider.cpp
 * @brief ESP32 device ID provider (efuse MAC).
 */
#include "platform/esp_device_id_provider.h"

#include <cstring>

#include "esp_system.h"

namespace naviga::platform {

DeviceId EspDeviceIdProvider::get() const {
  DeviceId id{};
  uint8_t base_mac[6] = {0};
  if (esp_efuse_mac_get_default(base_mac) == ESP_OK) {
    std::memcpy(id.bytes, base_mac, sizeof(id.bytes));
  }
  return id;
}

} // namespace naviga::platform
