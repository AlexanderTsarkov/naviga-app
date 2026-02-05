/**
 * @file arduino_clock.cpp
 * @brief Arduino-backed platform clock.
 */
#include "platform/arduino_clock.h"

#include <Arduino.h>

namespace naviga::platform {

millis_t ArduinoClock::uptime_ms() const {
  return ::millis();
}

void ArduinoClock::sleep_ms(millis_t duration_ms) const {
  ::delay(duration_ms);
}

} // namespace naviga::platform
