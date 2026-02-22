#pragma once

#include <cstdint>

#include "app/m1_runtime.h"
#include "domain/logger.h"
#include "services/oled_status.h"
#include "services/radio_smoke_service.h"
#include "services/serial_provisioning.h"

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
  uint16_t short_id_ = 0;
  char short_id_hex_[5] = {0};
  char mac_hex_[18] = {0};
  char bt_short_[5] = {0};
  domain::Logger event_logger_;
  M1Runtime runtime_;
  OledStatus oled_;
  SerialProvisioning serial_provisioning_;
};

} // namespace naviga
