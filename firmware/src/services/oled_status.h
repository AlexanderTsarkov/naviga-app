#pragma once

#include <cstddef>
#include <cstdint>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "hw_profile.h"

namespace naviga {

/** #450: S03-aligned OLED status data. Compact labels only; effective runtime values. */
struct OledStatusData {
  const char* display_name = nullptr;   ///< Short_ID or Node_Name (non-empty); for line 1
  uint32_t uptime_ms = 0;
  uint8_t tx_power_dbm = 0;
  size_t active_nodes = 0;
  const char* fix_state = nullptr;     ///< "N", "2D", "3D"
  const char* role_name = nullptr;      ///< "Person", "Dog", "Infra"
  uint16_t min_interval_sec = 0;
  uint16_t min_distance_m = 0;
  uint8_t max_silence_10s = 0;
  uint32_t pos_tx = 0;
  uint32_t st_tx = 0;
  uint32_t pos_rx = 0;
  uint32_t st_rx = 0;
};

class OledStatus {
 public:
  void init(const HwProfile& profile);
  void update(uint32_t now_ms, const OledStatusData& data);

 private:
  void render(const OledStatusData& data);

  Adafruit_SSD1306 display_{128, 64, &Wire, -1};
  bool ready_ = false;
  uint32_t last_render_ms_ = 0;
};

} // namespace naviga
