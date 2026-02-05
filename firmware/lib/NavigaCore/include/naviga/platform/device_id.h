/**
 * @file device_id.h
 * @brief Platform device identifier abstraction.
 */
#pragma once

#include <cstdint>

namespace naviga::platform {

/**
 * @brief Device identifier (e.g., MAC or unique hardware ID).
 */
struct DeviceId {
  uint8_t bytes[6] = {0};
};

/**
 * @brief Platform device ID provider.
 */
struct IDeviceIdProvider {
  virtual ~IDeviceIdProvider() = default;

  /**
   * @brief Return the device identifier.
   */
  virtual DeviceId get() const = 0;
};

} // namespace naviga::platform
