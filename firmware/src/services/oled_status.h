#pragma once

#include <cstddef>
#include <cstdint>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "hw_profile.h"
#include "services/radio_smoke_service.h"

namespace naviga {

struct OledStatusData {
  const char* bt_short = nullptr;
  const char* firmware_version = nullptr;
  bool ble_connected = false;
  size_t nodes_seen = 0;
  uint16_t geo_seq = 0;
  uint32_t uptime_ms = 0;
};

class OledStatus {
 public:
  void init(const HwProfile& profile);
  void update(uint32_t now_ms, const RadioSmokeStats& stats, const OledStatusData& data);

 private:
  void render(const RadioSmokeStats& stats, const OledStatusData& data);

  Adafruit_SSD1306 display_{128, 64, &Wire, -1};
  bool ready_ = false;
  uint32_t last_render_ms_ = 0;
};

} // namespace naviga
