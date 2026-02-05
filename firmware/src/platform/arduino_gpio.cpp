/**
 * @file arduino_gpio.cpp
 * @brief Arduino GPIO helpers for platform layer.
 */
#include "platform/arduino_gpio.h"

#include <Arduino.h>

namespace naviga::platform {

void configure_input_pullup(int pin) {
  ::pinMode(pin, INPUT_PULLUP);
}

bool read_digital(int pin) {
  return ::digitalRead(pin) == HIGH;
}

} // namespace naviga::platform
