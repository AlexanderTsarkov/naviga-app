#include "platform/timebase.h"

#include <Arduino.h>

namespace naviga {

uint32_t uptime_ms() {
  return millis();
}

} // namespace naviga
