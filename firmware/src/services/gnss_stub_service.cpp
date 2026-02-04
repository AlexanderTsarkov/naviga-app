#include "services/gnss_stub_service.h"

#include <cmath>

namespace naviga {

namespace {

constexpr int32_t kBaseLatE7 = 571844000; // Rostov Veliky, RU (approx).
constexpr int32_t kBaseLonE7 = 383663000; // Rostov Veliky, RU (approx).
constexpr double kEarthRadiusM = 6371000.0;

constexpr bool kForceNoFix = false;

uint32_t lcg_next(uint32_t& state) {
  state = state * 1664525u + 1013904223u;
  return state;
}

void apply_move_m(int32_t& lat_e7, int32_t& lon_e7, double east_m, double north_m) {
  const double lat_rad = (lat_e7 / 1e7) * M_PI / 180.0;
  const double d_lat = north_m / kEarthRadiusM;
  const double d_lon = east_m / (kEarthRadiusM * std::cos(lat_rad));

  const double new_lat = lat_rad + d_lat;
  const double new_lon = (lon_e7 / 1e7) * M_PI / 180.0 + d_lon;

  lat_e7 = static_cast<int32_t>(new_lat * 180.0 / M_PI * 1e7);
  lon_e7 = static_cast<int32_t>(new_lon * 180.0 / M_PI * 1e7);
}

} // namespace

void GnssStubService::init(uint64_t seed) {
  rng_state_ = static_cast<uint32_t>(seed ^ (seed >> 32));
  last_sample_ms_ = 0;
  sample_ = {
      .has_fix = !kForceNoFix,
      .lat_e7 = kBaseLatE7,
      .lon_e7 = kBaseLonE7,
  };
}

bool GnssStubService::tick(uint32_t now_ms) {
  if (last_sample_ms_ != 0 && (now_ms - last_sample_ms_) < 1000U) {
    return false;
  }
  last_sample_ms_ = now_ms;

  if (kForceNoFix) {
    sample_.has_fix = false;
    return true;
  }

  sample_.has_fix = true;

  // Deterministic drift: sometimes <25m, sometimes >25m.
  const uint32_t r = lcg_next(rng_state_);
  const bool small_step = (r % 5u) == 0u;
  const double step_m = small_step ? (5.0 + (r % 10u)) : (30.0 + (r % 10u));
  const double angle = (lcg_next(rng_state_) % 360u) * (M_PI / 180.0);

  const double north_m = step_m * std::cos(angle);
  const double east_m = step_m * std::sin(angle);
  apply_move_m(sample_.lat_e7, sample_.lon_e7, east_m, north_m);

  return true;
}

GnssSample GnssStubService::latest() const {
  return sample_;
}

} // namespace naviga
