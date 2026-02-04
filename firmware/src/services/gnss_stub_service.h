#pragma once

#include <cstdint>

namespace naviga {

struct GnssSample {
  bool has_fix;
  int32_t lat_e7;
  int32_t lon_e7;
};

class GnssStubService {
 public:
  void init(uint64_t seed);
  bool tick(uint32_t now_ms);
  GnssSample latest() const;

 private:
  uint32_t rng_state_ = 0;
  uint32_t last_sample_ms_ = 0;
  GnssSample sample_{};
};

} // namespace naviga
