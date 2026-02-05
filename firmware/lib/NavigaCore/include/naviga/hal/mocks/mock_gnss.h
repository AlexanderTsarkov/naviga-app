#pragma once

#include "naviga/hal/interfaces.h"

namespace naviga {

class MockGnss : public IGnss {
 public:
  bool get_snapshot(GnssSnapshot* out) override;
  void set_snapshot(const GnssSnapshot& snapshot);

 private:
  GnssSnapshot snapshot_{};
  bool has_snapshot_ = false;
};

} // namespace naviga
