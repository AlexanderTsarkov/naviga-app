#pragma once

#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

/// Stub GNSS provider for dev/testing. Implements IGnss with deterministic
/// behavior: starts NO_FIX, acquires FIX_3D after warmup, optional fix loss.
class GnssStubService : public IGnss {
 public:
  void init(uint64_t seed);
  bool tick(uint32_t now_ms);
  /** Lightweight verify: stub has no hardware; always reports ok. */
  bool verify_on_boot(uint32_t timeout_ms);
  bool get_snapshot(GnssSnapshot* out) override;

 private:
  uint32_t rng_state_ = 0;
  uint32_t last_sample_ms_ = 0;
  uint32_t boot_ms_ = 0;  // first tick sets this
  GnssSnapshot snapshot_{};
};

} // namespace naviga
