#pragma once

#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

/**
 * GNSS scenario override for deterministic bench tests (#288).
 * When enabled, get_snapshot_if_active() returns injected state instead of real GNSS.
 * Default: disabled (use real GNSS).
 */
class GnssScenarioOverride {
 public:
  GnssScenarioOverride() = default;

  void set_off() { enabled_ = false; }
  void set_nofix();
  void set_fix(int32_t lat_e7, int32_t lon_e7);
  void move(int32_t dlat_e7, int32_t dlon_e7);

  /** If enabled, write current override snapshot to *out (last_fix_ms set to now_ms when pos_valid) and return true. */
  bool get_snapshot_if_active(GnssSnapshot* out, uint32_t now_ms) const;

  bool enabled() const { return enabled_; }

 private:
  bool enabled_ = false;
  GnssSnapshot snapshot_{};
};

}  // namespace naviga
