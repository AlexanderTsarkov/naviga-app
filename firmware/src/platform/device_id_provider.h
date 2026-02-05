/**
 * @file device_id_provider.h
 * @brief Default device ID provider alias for the platform.
 */
#pragma once

#include "platform/esp_device_id_provider.h"

namespace naviga::platform {

/**
 * @brief Default device ID provider for this platform.
 */
using DefaultDeviceIdProvider = EspDeviceIdProvider;

} // namespace naviga::platform
