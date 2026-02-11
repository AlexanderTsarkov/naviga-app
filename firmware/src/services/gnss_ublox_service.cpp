#include "services/gnss_ublox_service.h"

#include "hw_profile.h"

namespace naviga {

#if defined(GNSS_PROVIDER_UBLOX)
namespace {

const char* fix_state_to_cstr(GNSSFixState state) {
  switch (state) {
    case GNSSFixState::FIX_2D:
      return "FIX_2D";
    case GNSSFixState::FIX_3D:
      return "FIX_3D";
    case GNSSFixState::NO_FIX:
    default:
      return "NO_FIX";
  }
}

} // namespace
#endif

#if defined(GNSS_PROVIDER_UBLOX)
void GnssUbloxService::write_ubx_frame(uint8_t msg_class,
                                       uint8_t msg_id,
                                       const uint8_t* payload,
                                       uint16_t payload_len) {
  uint8_t ck_a = 0;
  uint8_t ck_b = 0;
  auto accum = [&](uint8_t v) {
    ck_a = static_cast<uint8_t>(ck_a + v);
    ck_b = static_cast<uint8_t>(ck_b + ck_a);
  };

  serial_.write(UbxStreamParser::kSync1);
  serial_.write(UbxStreamParser::kSync2);
  serial_.write(msg_class);
  serial_.write(msg_id);
  serial_.write(static_cast<uint8_t>(payload_len & 0xFF));
  serial_.write(static_cast<uint8_t>((payload_len >> 8) & 0xFF));

  accum(msg_class);
  accum(msg_id);
  accum(static_cast<uint8_t>(payload_len & 0xFF));
  accum(static_cast<uint8_t>((payload_len >> 8) & 0xFF));

  for (uint16_t i = 0; i < payload_len; ++i) {
    const uint8_t b = payload[i];
    serial_.write(b);
    accum(b);
  }

  serial_.write(ck_a);
  serial_.write(ck_b);
}

void GnssUbloxService::send_cfg_enable_nav_pvt() {
  // UBX-CFG-MSG payload (u-blox 8, 8 bytes):
  // msgClass, msgID, rateDDC, rateUART1, rateUART2, rateUSB, rateSPI, reserved.
  const uint8_t payload[8] = {
      UbxStreamParser::kClassNav,
      UbxStreamParser::kIdNavPvt,
      0U,
      1U,
      0U,
      0U,
      0U,
      0U,
  };
  write_ubx_frame(0x06U, 0x01U, payload, static_cast<uint16_t>(sizeof(payload)));
  Serial.println("GNSS_UBX cfg: enable NAV-PVT");
}
#endif

void GnssUbloxService::init(uint64_t /*seed*/) {
  snapshot_ = {
      .fix_state = GNSSFixState::NO_FIX,
      .pos_valid = false,
      .lat_e7 = 0,
      .lon_e7 = 0,
      .last_fix_ms = 0,
  };
  bytes_rx_ = 0;
  frames_ok_ = 0;
  frames_bad_ck_ = 0;
  last_frame_ms_ = 0;
#if defined(GNSS_PROVIDER_UBLOX)
  service_start_ms_ = millis();
  no_data_hint_logged_ = false;
  nmea_hint_logged_ = false;
  nmea_hint_ = false;
  nmea_window_remaining_ = 0;
#if GNSS_UBLOX_DIAG
  next_diag_log_ms_ = service_start_ms_ + kDiagLogPeriodMs;
#endif
#endif

  const auto& profile = get_hw_profile();
  if (profile.pins.gps_rx < 0 || profile.pins.gps_tx < 0) {
    uart_ready_ = false;
    return;
  }

  serial_.begin(kUartBaud, SERIAL_8N1, profile.pins.gps_rx, profile.pins.gps_tx);
  uart_ready_ = true;
#if defined(GNSS_PROVIDER_UBLOX)
  send_cfg_enable_nav_pvt();
#endif
}

void GnssUbloxService::apply_fix_from_nav_pvt(uint8_t fix_type,
                                              int32_t lat_e7,
                                              int32_t lon_e7,
                                              uint32_t now_ms) {
  GNSSFixState fix_state = GNSSFixState::NO_FIX;
  if (fix_type == 2U) {
    fix_state = GNSSFixState::FIX_2D;
  } else if (fix_type >= 3U) {
    fix_state = GNSSFixState::FIX_3D;
  }

  const bool pos_valid = (fix_state != GNSSFixState::NO_FIX);
  snapshot_.fix_state = fix_state;
  snapshot_.pos_valid = pos_valid;

  if (pos_valid) {
    snapshot_.lat_e7 = lat_e7;
    snapshot_.lon_e7 = lon_e7;
    // Policy: update last_fix_ms on each valid position sample.
    snapshot_.last_fix_ms = now_ms;
  }
}

#if defined(GNSS_PROVIDER_UBLOX)
void GnssUbloxService::update_nmea_hint(uint8_t byte) {
  if (nmea_hint_) {
    return;
  }

  if (byte == static_cast<uint8_t>('$')) {
    nmea_window_remaining_ = kNmeaHintWindowBytes;
    return;
  }

  if (nmea_window_remaining_ == 0) {
    return;
  }

  if (byte == static_cast<uint8_t>('\n')) {
    nmea_hint_ = true;
    return;
  }

  --nmea_window_remaining_;
}

void GnssUbloxService::maybe_log_diag(uint32_t now_ms) {
  if (!nmea_hint_logged_ && nmea_hint_) {
    Serial.println("GNSS_UBX hint=NMEA (parser expects UBX NAV-PVT)");
    nmea_hint_logged_ = true;
  }

  const uint32_t since_start_ms = now_ms - service_start_ms_;
  if (!no_data_hint_logged_ && since_start_ms >= kNoDataHintDelayMs && bytes_rx_ == 0U) {
    Serial.println("GNSS_UBX no UART data (check power, GND, TX->RX16, baud)");
    no_data_hint_logged_ = true;
  }

#if GNSS_UBLOX_DIAG
  if (static_cast<int32_t>(now_ms - next_diag_log_ms_) < 0) {
    return;
  }

  Serial.printf("GNSS_UBX rx=%lu ok=%lu bad=%lu last=%lu fix=%s lat=%ld lon=%ld\n",
                static_cast<unsigned long>(bytes_rx_),
                static_cast<unsigned long>(frames_ok_),
                static_cast<unsigned long>(frames_bad_ck_),
                static_cast<unsigned long>(last_frame_ms_),
                fix_state_to_cstr(snapshot_.fix_state),
                static_cast<long>(snapshot_.pos_valid ? snapshot_.lat_e7 : 0),
                static_cast<long>(snapshot_.pos_valid ? snapshot_.lon_e7 : 0));

  next_diag_log_ms_ = now_ms + kDiagLogPeriodMs;
#else
  (void)now_ms;
#endif
}
#endif

bool GnssUbloxService::tick(uint32_t now_ms) {
  if (!uart_ready_) {
    snapshot_.fix_state = GNSSFixState::NO_FIX;
    snapshot_.pos_valid = false;
#if defined(GNSS_PROVIDER_UBLOX)
    maybe_log_diag(now_ms);
#endif
    return true;
  }

  int available = serial_.available();
  if (available <= 0) {
#if defined(GNSS_PROVIDER_UBLOX)
    maybe_log_diag(now_ms);
#endif
    return false;
  }

  uint16_t to_read = static_cast<uint16_t>(available);
  if (to_read > kMaxReadPerTick) {
    to_read = kMaxReadPerTick;
  }

  bool updated = false;
  for (uint16_t i = 0; i < to_read; ++i) {
    const int c = serial_.read();
    if (c < 0) {
      break;
    }
    const uint8_t byte = static_cast<uint8_t>(c);
    ++bytes_rx_;
#if defined(GNSS_PROVIDER_UBLOX)
    update_nmea_hint(byte);
#endif

    UbxFrameView frame{};
    const UbxParseStatus status = parser_.push_byte(byte, &frame);
    if (status == UbxParseStatus::FrameBadChecksum) {
      ++frames_bad_ck_;
      continue;
    }
    if (status != UbxParseStatus::FrameOk) {
      continue;
    }

    ++frames_ok_;
    last_frame_ms_ = now_ms;

    uint8_t fix_type = 0;
    int32_t lat_e7 = 0;
    int32_t lon_e7 = 0;
    if (parse_nav_pvt_fix_lat_lon(frame, &fix_type, &lat_e7, &lon_e7)) {
      apply_fix_from_nav_pvt(fix_type, lat_e7, lon_e7, now_ms);
      updated = true;
    }
  }

#if defined(GNSS_PROVIDER_UBLOX)
  maybe_log_diag(now_ms);
#endif
  return updated;
}

bool GnssUbloxService::get_snapshot(GnssSnapshot* out) {
  if (!out) {
    return false;
  }
  *out = snapshot_;
  return true;
}

} // namespace naviga
