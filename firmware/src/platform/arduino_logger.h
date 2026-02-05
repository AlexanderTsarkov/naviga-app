/**
 * @file arduino_logger.h
 * @brief Arduino-backed logger implementation.
 */
#pragma once

#include "naviga/platform/log.h"

namespace naviga::platform {

/**
 * @brief Arduino logger implementation (Serial).
 */
class ArduinoLogger final : public ILogger {
 public:
  void log(LogLevel level, const char* tag, const char* msg) override;
};

} // namespace naviga::platform
