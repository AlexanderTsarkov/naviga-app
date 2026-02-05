/**
 * @file arduino_gpio.h
 * @brief Arduino GPIO helpers for platform layer.
 */
#pragma once

#include <cstdint>

namespace naviga::platform {

/**
 * @brief Configure GPIO as input with pull-up.
 */
void configure_input_pullup(int pin);

/**
 * @brief Read GPIO digital value.
 * @return true if HIGH, false if LOW.
 */
bool read_digital(int pin);

} // namespace naviga::platform
