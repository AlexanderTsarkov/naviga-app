/**
 * @file clock.h
 * @brief Platform clock interface for core timing.
 */
#pragma once

#include <cstdint>

namespace naviga::platform {

/// Millisecond time base used by core services.
using millis_t = uint32_t;

/**
 * @brief Platform clock abstraction.
 */
struct IClock {
  virtual ~IClock() = default;

  /**
   * @brief Return time since boot in milliseconds.
   */
  virtual millis_t uptime_ms() const = 0;

  /**
   * @brief Sleep for the specified number of milliseconds.
   */
  virtual void sleep_ms(millis_t duration_ms) const = 0;
};

} // namespace naviga::platform
