#include "services/gnss_ublox_service.h"

#include "hw_profile.h"

namespace naviga {

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

  const auto& profile = get_hw_profile();
  if (profile.pins.gps_rx < 0 || profile.pins.gps_tx < 0) {
    uart_ready_ = false;
    return;
  }

  serial_.begin(kUartBaud, SERIAL_8N1, profile.pins.gps_rx, profile.pins.gps_tx);
  uart_ready_ = true;
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

bool GnssUbloxService::tick(uint32_t now_ms) {
  if (!uart_ready_) {
    snapshot_.fix_state = GNSSFixState::NO_FIX;
    snapshot_.pos_valid = false;
    return true;
  }

  int available = serial_.available();
  if (available <= 0) {
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
    ++bytes_rx_;

    UbxFrameView frame{};
    const UbxParseStatus status = parser_.push_byte(static_cast<uint8_t>(c), &frame);
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
