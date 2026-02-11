#pragma once

#include <cstdint>

#include <Arduino.h>

#include "naviga/hal/interfaces.h"
#include "services/ubx_nav_pvt_parser.h"

namespace naviga {

class GnssUbloxService : public IGnss {
 public:
  void init(uint64_t seed);
  bool tick(uint32_t now_ms);
  bool get_snapshot(GnssSnapshot* out) override;

 private:
  static constexpr uint32_t kUartBaud = 9600U;
  static constexpr uint16_t kMaxReadPerTick = 256U;

  HardwareSerial serial_{1};
  bool uart_ready_ = false;
  UbxStreamParser parser_{};
  GnssSnapshot snapshot_{};

  uint32_t bytes_rx_ = 0;
  uint32_t frames_ok_ = 0;
  uint32_t frames_bad_ck_ = 0;
  uint32_t last_frame_ms_ = 0;

  void apply_fix_from_nav_pvt(uint8_t fix_type, int32_t lat_e7, int32_t lon_e7, uint32_t now_ms);
};

} // namespace naviga
