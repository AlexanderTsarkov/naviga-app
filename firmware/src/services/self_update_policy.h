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
  SelfUpdateDecision evaluate(uint32_t now_ms, const GnssSnapshot& snapshot);
  void commit(uint32_t now_ms, const GnssSnapshot& snapshot);

 private:
  bool has_commit_ = false;
  uint32_t last_committed_ms_ = 0;
  int32_t last_lat_e7_ = 0;
  int32_t last_lon_e7_ = 0;
};

} // namespace naviga
