#include "services/gnss_ublox_service.h"

#include "hw_profile.h"

namespace naviga {

#if defined(GNSS_PROVIDER_UBLOX)
void GnssUbloxService::write_ubx_frame(uint8_t msg_class,
                                       uint8_t msg_id,
                                       const uint8_t* payload,
                                       uint16_t payload_len) {
  if (!io_) {
    return;
  }

  uint8_t header[6] = {
      UbxStreamParser::kSync1,
      UbxStreamParser::kSync2,
      msg_class,
      msg_id,
      static_cast<uint8_t>(payload_len & 0xFF),
      static_cast<uint8_t>((payload_len >> 8) & 0xFF),
  };
  uint8_t ck_a = 0;
  uint8_t ck_b = 0;
  auto accum = [&](uint8_t v) {
    ck_a = static_cast<uint8_t>(ck_a + v);
    ck_b = static_cast<uint8_t>(ck_b + ck_a);
  };

  io_->write_bytes(header, sizeof(header));

  accum(msg_class);
  accum(msg_id);
  accum(static_cast<uint8_t>(payload_len & 0xFF));
  accum(static_cast<uint8_t>((payload_len >> 8) & 0xFF));

  if (payload && payload_len > 0) {
    io_->write_bytes(payload, payload_len);
    for (uint16_t i = 0; i < payload_len; ++i) {
      accum(payload[i]);
    }
  }

  uint8_t checksum[2] = {ck_a, ck_b};
  io_->write_bytes(checksum, sizeof(checksum));
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
  cfg_nav_pvt_event_pending_ = true;
}
#endif

void GnssUbloxService::set_io(IGnssUbxIo* io) {
  io_ = io;
}

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
  service_start_ms_ = 0;
  nmea_hint_ = false;
  nmea_window_remaining_ = 0;
  cfg_nav_pvt_event_pending_ = false;
  nmea_hint_event_pending_ = false;
  no_data_hint_event_pending_ = false;
  nmea_hint_event_emitted_ = false;
  no_data_hint_event_emitted_ = false;
#endif

  const auto& profile = get_hw_profile();
  if (!io_ || profile.pins.gps_rx < 0 || profile.pins.gps_tx < 0) {
    uart_ready_ = false;
    return;
  }

  uart_ready_ = io_->begin(kUartBaud, profile.pins.gps_rx, profile.pins.gps_tx);
#if defined(GNSS_PROVIDER_UBLOX)
  if (uart_ready_) {
    send_cfg_enable_nav_pvt();
  }
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
    if (!nmea_hint_event_emitted_) {
      nmea_hint_event_pending_ = true;
      nmea_hint_event_emitted_ = true;
    }
    return;
  }

  --nmea_window_remaining_;
}

void GnssUbloxService::update_diag_events(uint32_t now_ms) {
  if (service_start_ms_ == 0) {
    service_start_ms_ = now_ms;
  }

  const uint32_t since_start_ms = now_ms - service_start_ms_;
  if (!no_data_hint_event_emitted_ && since_start_ms >= kNoDataHintDelayMs && bytes_rx_ == 0U) {
    no_data_hint_event_pending_ = true;
    no_data_hint_event_emitted_ = true;
  }
}
#endif

bool GnssUbloxService::tick(uint32_t now_ms) {
  if (!uart_ready_) {
    snapshot_.fix_state = GNSSFixState::NO_FIX;
    snapshot_.pos_valid = false;
#if defined(GNSS_PROVIDER_UBLOX)
    update_diag_events(now_ms);
#endif
    return true;
  }

  int available = io_ ? io_->available() : 0;
  if (available <= 0) {
#if defined(GNSS_PROVIDER_UBLOX)
    update_diag_events(now_ms);
#endif
    return false;
  }

  uint16_t to_read = static_cast<uint16_t>(available);
  if (to_read > kMaxReadPerTick) {
    to_read = kMaxReadPerTick;
  }

  bool updated = false;
  for (uint16_t i = 0; i < to_read; ++i) {
    const int c = io_->read_byte();
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
  update_diag_events(now_ms);
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

bool GnssUbloxService::get_diag(GnssUbloxDiag* out) const {
  if (!out) {
    return false;
  }
  out->bytes_rx = bytes_rx_;
  out->frames_ok = frames_ok_;
  out->frames_bad_ck = frames_bad_ck_;
  out->last_frame_ms = last_frame_ms_;
  out->fix_state = snapshot_.fix_state;
  out->lat_e7 = snapshot_.pos_valid ? snapshot_.lat_e7 : 0;
  out->lon_e7 = snapshot_.pos_valid ? snapshot_.lon_e7 : 0;
  return true;
}

bool GnssUbloxService::take_diag_events(GnssUbloxDiagEvents* out) {
  if (!out) {
    return false;
  }

#if defined(GNSS_PROVIDER_UBLOX)
  out->cfg_nav_pvt_sent = cfg_nav_pvt_event_pending_;
  out->hint_nmea = nmea_hint_event_pending_;
  out->hint_no_data = no_data_hint_event_pending_;

  cfg_nav_pvt_event_pending_ = false;
  nmea_hint_event_pending_ = false;
  no_data_hint_event_pending_ = false;
#else
  out->cfg_nav_pvt_sent = false;
  out->hint_nmea = false;
  out->hint_no_data = false;
#endif
  return true;
}

} // namespace naviga
