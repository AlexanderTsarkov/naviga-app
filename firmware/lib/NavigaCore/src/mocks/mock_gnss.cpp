#include "naviga/hal/mocks/mock_gnss.h"

namespace naviga {

bool MockGnss::get_snapshot(GnssSnapshot* out) {
  if (!has_snapshot_ || !out) {
    return false;
  }
  *out = snapshot_;
  return true;
}

void MockGnss::set_snapshot(const GnssSnapshot& snapshot) {
  snapshot_ = snapshot;
  has_snapshot_ = true;
}

} // namespace naviga
