/**
 * @file arduino_logger.cpp
 * @brief Arduino-backed logger implementation.
 */
#include "platform/arduino_logger.h"

#include <Arduino.h>

namespace naviga::platform {

void ArduinoLogger::log(LogLevel level, const char* tag, const char* msg) {
  (void)level;
  (void)tag;
  if (!msg) {
    return;
  }
  if (Serial) {
    Serial.println(msg);
  }
}

} // namespace naviga::platform
