#include "services/gnss_ublox_placeholder_service.h"

namespace naviga {

void GnssUbloxPlaceholderService::init(uint64_t /*seed*/) {
  snapshot_ = {
      .fix_state = GNSSFixState::NO_FIX,
      .pos_valid = false,
      .lat_e7 = 0,
      .lon_e7 = 0,
      .last_fix_ms = 0,
  };
}

bool GnssUbloxPlaceholderService::tick(uint32_t /*now_ms*/) {
  // TODO(#128): replace with real u-blox UART/UBX polling.
  snapshot_.fix_state = GNSSFixState::NO_FIX;
  snapshot_.pos_valid = false;
  return true;
}

bool GnssUbloxPlaceholderService::get_snapshot(GnssSnapshot* out) {
  if (!out) {
    return false;
  }
  *out = snapshot_;
  return true;
}

} // namespace naviga
