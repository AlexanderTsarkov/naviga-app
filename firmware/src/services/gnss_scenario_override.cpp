#include "services/gnss_scenario_override.h"

namespace naviga {

void GnssScenarioOverride::set_nofix() {
  enabled_ = true;
  snapshot_.fix_state = GNSSFixState::NO_FIX;
  snapshot_.pos_valid = false;
  snapshot_.lat_e7 = 0;
  snapshot_.lon_e7 = 0;
  snapshot_.last_fix_ms = 0;
}

void GnssScenarioOverride::set_fix(int32_t lat_e7, int32_t lon_e7) {
  enabled_ = true;
  snapshot_.fix_state = GNSSFixState::FIX_3D;
  snapshot_.pos_valid = true;
  snapshot_.lat_e7 = lat_e7;
  snapshot_.lon_e7 = lon_e7;
  snapshot_.last_fix_ms = 0;  // set at get_snapshot_if_active to now_ms
}

void GnssScenarioOverride::move(int32_t dlat_e7, int32_t dlon_e7) {
  if (!enabled_) return;
  snapshot_.lat_e7 += dlat_e7;
  snapshot_.lon_e7 += dlon_e7;
}

bool GnssScenarioOverride::get_snapshot_if_active(GnssSnapshot* out, uint32_t now_ms) const {
  if (!enabled_ || !out) return false;
  *out = snapshot_;
  if (out->pos_valid)
    out->last_fix_ms = now_ms;  // position appears fresh
  return true;
}

}  // namespace naviga
