#include "app/app.h"

#include <Arduino.h>

namespace naviga {

namespace {

uint32_t last_heartbeat_ms = 0;

} // namespace

void app_init() {
  last_heartbeat_ms = 0;
}

void app_tick(uint32_t now_ms) {
  if ((now_ms - last_heartbeat_ms) >= 1000U) {
    last_heartbeat_ms = now_ms;
    Serial.print("tick: ");
    Serial.println(now_ms);
  }
}

} // namespace naviga
