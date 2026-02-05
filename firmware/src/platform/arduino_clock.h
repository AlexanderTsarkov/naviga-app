/**
 * @file arduino_clock.h
 * @brief Arduino-backed platform clock.
 */
#pragma once

#include "naviga/platform/clock.h"

namespace naviga::platform {

/**
 * @brief Arduino clock implementation.
 */
class ArduinoClock final : public IClock {
 public:
  millis_t uptime_ms() const override;
  void sleep_ms(millis_t duration_ms) const override;
};

} // namespace naviga::platform
