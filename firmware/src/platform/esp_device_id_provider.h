/**
 * @file esp_device_id_provider.h
 * @brief ESP32 device ID provider (efuse MAC).
 */
#pragma once

#include "naviga/platform/device_id.h"

namespace naviga::platform {

/**
 * @brief ESP32 implementation of device ID provider.
 */
class EspDeviceIdProvider final : public IDeviceIdProvider {
 public:
  DeviceId get() const override;
};

} // namespace naviga::platform
