#pragma once

#include <cstddef>
#include <cstdint>

#include "naviga/hal/interfaces.h"
#include "services/ubx_nav_pvt_parser.h"

#ifndef GNSS_UBLOX_DIAG
#define GNSS_UBLOX_DIAG 0
#endif

namespace naviga {

class IGnssUbxIo {
 public:
  virtual ~IGnssUbxIo() = default;
  virtual bool begin(uint32_t baud, int8_t rx_pin, int8_t tx_pin) = 0;
  virtual int available() = 0;
  virtual int read_byte() = 0;
  virtual size_t write_bytes(const uint8_t* data, size_t len) = 0;
};

struct GnssUbloxDiag {
  uint32_t bytes_rx = 0;
  uint32_t frames_ok = 0;
  uint32_t frames_bad_ck = 0;
  uint32_t last_frame_ms = 0;
  GNSSFixState fix_state = GNSSFixState::NO_FIX;
  int32_t lat_e7 = 0;
  int32_t lon_e7 = 0;
};

struct GnssUbloxDiagEvents {
  bool cfg_nav_pvt_sent = false;
  bool hint_nmea = false;
  bool hint_no_data = false;
};

class GnssUbloxService : public IGnss {
 public:
  void set_io(IGnssUbxIo* io);
  void init(uint64_t seed);
  bool tick(uint32_t now_ms);
  bool get_snapshot(GnssSnapshot* out) override;
  bool get_diag(GnssUbloxDiag* out) const;
  bool take_diag_events(GnssUbloxDiagEvents* out);

 private:
  static constexpr uint32_t kUartBaud = 9600U;
  static constexpr uint16_t kMaxReadPerTick = 256U;

  IGnssUbxIo* io_ = nullptr;
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
  bool nmea_hint_ = false;
  uint16_t nmea_window_remaining_ = 0;
  bool cfg_nav_pvt_event_pending_ = false;
  bool nmea_hint_event_pending_ = false;
  bool no_data_hint_event_pending_ = false;
  bool nmea_hint_event_emitted_ = false;
  bool no_data_hint_event_emitted_ = false;

  void update_diag_events(uint32_t now_ms);
  void update_nmea_hint(uint8_t byte);
  void send_cfg_enable_nav_pvt();
  void write_ubx_frame(uint8_t msg_class, uint8_t msg_id, const uint8_t* payload, uint16_t payload_len);
#endif
};

} // namespace naviga
