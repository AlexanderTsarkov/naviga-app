#pragma once

#include <cstdint>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "hw_profile.h"
#include "services/radio_smoke_service.h"

namespace naviga {

class OledStatus {
 public:
  void init(const HwProfile& profile, const char* firmware_version, RadioRole role);
  void update(uint32_t now_ms, const RadioSmokeStats& stats);

 private:
  void render(const RadioSmokeStats& stats);

  Adafruit_SSD1306 display_{128, 64, &Wire, -1};
  bool ready_ = false;
  uint32_t last_render_ms_ = 0;
  const char* profile_name_ = nullptr;
  const char* firmware_version_ = nullptr;
  RadioRole role_ = RadioRole::RESP;
};

} // namespace naviga
