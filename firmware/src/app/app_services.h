#pragma once

#include <cstdint>

namespace naviga {

class AppServices {
 public:
  void init();
  void tick(uint32_t now_ms);

 private:
  uint32_t last_heartbeat_ms_ = 0;
  uint32_t last_summary_ms_ = 0;
  bool fix_logged_ = false;
};

} // namespace naviga
