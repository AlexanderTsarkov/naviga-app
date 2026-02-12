#pragma once

#include <cstdint>

#include <Arduino.h>

#include "naviga/hal/interfaces.h"
#include "services/ubx_nav_pvt_parser.h"

#ifndef GNSS_UBLOX_DIAG
#define GNSS_UBLOX_DIAG 0
#endif

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

#if defined(GNSS_PROVIDER_UBLOX)
  static constexpr uint32_t kNoDataHintDelayMs = 5000U;
  static constexpr uint16_t kNmeaHintWindowBytes = 96U;

  uint32_t service_start_ms_ = 0;
  bool no_data_hint_logged_ = false;
  bool nmea_hint_logged_ = false;
  bool nmea_hint_ = false;
  uint16_t nmea_window_remaining_ = 0;

#if GNSS_UBLOX_DIAG
  static constexpr uint32_t kDiagLogPeriodMs = 2000U;
  uint32_t next_diag_log_ms_ = 0;
#endif

  void maybe_log_diag(uint32_t now_ms);
  void update_nmea_hint(uint8_t byte);
  void send_cfg_enable_nav_pvt();
  void write_ubx_frame(uint8_t msg_class, uint8_t msg_id, const uint8_t* payload, uint16_t payload_len);
#endif
};

} // namespace naviga
