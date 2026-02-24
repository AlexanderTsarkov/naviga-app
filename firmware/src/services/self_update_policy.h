#pragma once

#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

enum class SelfUpdateReason {
  NONE,
  DISTANCE,
  MAX_SILENCE,
  FIRST_FIX,
};

struct SelfUpdateDecision {
  SelfUpdateReason reason;
  double distance_m;
  uint32_t dt_ms;
};

class SelfUpdatePolicy {
 public:
  void init();
  /** Role-derived cadence: max silence (ms) before MUST send alive; per role_profiles_policy_v0. */
  void set_max_silence_ms(uint32_t max_silence_ms);
  /** Role-derived: min time (ms) between position updates (minInterval); used for distance gating. */
  void set_min_time_ms(uint32_t min_time_ms);
  SelfUpdateDecision evaluate(uint32_t now_ms, const GnssSnapshot& snapshot);
  void commit(uint32_t now_ms, const GnssSnapshot& snapshot);

 private:
  bool has_commit_ = false;
  uint32_t last_committed_ms_ = 0;
  int32_t last_lat_e7_ = 0;
  int32_t last_lon_e7_ = 0;
  uint32_t max_silence_ms_ = 72000;
  uint32_t min_time_ms_ = 18000;
};

} // namespace naviga
