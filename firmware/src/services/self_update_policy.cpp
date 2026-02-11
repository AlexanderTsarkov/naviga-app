#include "services/self_update_policy.h"

#include "utils/geo_utils.h"

namespace naviga {

namespace {

constexpr double kMinDistanceM = 25.0;
constexpr uint32_t kMinTimeMs = 18000;
constexpr uint32_t kMaxSilenceMs = 72000;

} // namespace

void SelfUpdatePolicy::init() {
  has_commit_ = false;
  last_committed_ms_ = 0;
  last_lat_e7_ = 0;
  last_lon_e7_ = 0;
}

SelfUpdateDecision SelfUpdatePolicy::evaluate(uint32_t now_ms, const GnssSnapshot& snapshot) {
  if (!snapshot.pos_valid) {
    return {SelfUpdateReason::NONE, 0.0, 0};
  }

  if (!has_commit_) {
    return {SelfUpdateReason::FIRST_FIX, 0.0, 0};
  }

  const uint32_t dt_ms = now_ms - last_committed_ms_;
  const double distance_m =
      distance_m_e7(last_lat_e7_, last_lon_e7_, snapshot.lat_e7, snapshot.lon_e7);

  if (dt_ms >= kMaxSilenceMs) {
    return {SelfUpdateReason::MAX_SILENCE, distance_m, dt_ms};
  }

  if (dt_ms >= kMinTimeMs && distance_m >= kMinDistanceM) {
    return {SelfUpdateReason::DISTANCE, distance_m, dt_ms};
  }

  return {SelfUpdateReason::NONE, distance_m, dt_ms};
}

void SelfUpdatePolicy::commit(uint32_t now_ms, const GnssSnapshot& snapshot) {
  has_commit_ = true;
  last_committed_ms_ = now_ms;
  last_lat_e7_ = snapshot.lat_e7;
  last_lon_e7_ = snapshot.lon_e7;
}

} // namespace naviga
