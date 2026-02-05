#pragma once

#include <cstdint>

#include "domain/logger.h"
#include "services/oled_status.h"
#include "services/radio_smoke_service.h"

namespace naviga {

class AppServices {
 public:
  void init();
  void tick(uint32_t now_ms);

 private:
  uint32_t last_heartbeat_ms_ = 0;
  uint32_t last_summary_ms_ = 0;
  bool fix_logged_ = false;
  RadioRole role_ = RadioRole::RESP;
  domain::Logger event_logger_;
  RadioSmokeService radio_smoke_;
  OledStatus oled_;
};

} // namespace naviga
