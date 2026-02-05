/**
 * @file log.h
 * @brief Platform logging interface for core diagnostics.
 */
#pragma once

#include <cstdint>

namespace naviga::platform {

/**
 * @brief Log severity level.
 */
enum class LogLevel : uint8_t {
  kDebug = 0,
  kInfo = 1,
  kWarn = 2,
  kError = 3,
};

/**
 * @brief Platform logger abstraction.
 */
struct ILogger {
  virtual ~ILogger() = default;

  /**
   * @brief Emit a log message.
   * @param level Severity level.
   * @param tag Short component tag (null-terminated).
   * @param msg Message string (null-terminated).
   */
  virtual void log(LogLevel level, const char* tag, const char* msg) = 0;
};

} // namespace naviga::platform
