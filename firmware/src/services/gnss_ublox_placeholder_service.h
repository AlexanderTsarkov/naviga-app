#pragma once

#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

// Placeholder for GNSS_PROVIDER_UBLOX build path until #128 is implemented.
class GnssUbloxPlaceholderService : public IGnss {
 public:
  void init(uint64_t seed);
  bool tick(uint32_t now_ms);
  bool get_snapshot(GnssSnapshot* out) override;

 private:
  GnssSnapshot snapshot_{};
};

} // namespace naviga
