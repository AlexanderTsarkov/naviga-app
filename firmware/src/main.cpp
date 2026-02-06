// Minimal boot + tick skeleton (OOTB 11.1). No radio/BLE/GNSS logic.

#include <Arduino.h>

#include "app/app.h"
#include "hw_profile.h"
#include "platform/timebase.h"

namespace {

constexpr const char* kFirmwareVersion = "ootb-74-m1-runtime";

} // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // Wait for Serial on USB-enabled boards.
  }

  const auto& profile = naviga::get_hw_profile();
  Serial.println();
  Serial.println("=== Naviga OOTB skeleton ===");
  Serial.print("fw: ");
  Serial.println(kFirmwareVersion);
  Serial.print("hw_profile: ");
  Serial.println(profile.name);

  naviga::app_init();
}

void loop() {
  naviga::app_tick(naviga::uptime_ms());
}
