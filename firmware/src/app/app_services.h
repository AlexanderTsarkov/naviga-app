#pragma once

#include <cstdint>

#include "app/m1_runtime.h"
#include "domain/logger.h"
#include "services/oled_status.h"
#include "services/radio_smoke_service.h"

namespace naviga {

class ProvisioningAdapter;

class AppServices {
 public:
  AppServices();
  ~AppServices();
  void init();
  void tick(uint32_t now_ms);

  /** Called by instrumentation logger callback when instrumentation is enabled. */
  void log_instrumentation_line(const char* line);

 private:
  uint32_t last_heartbeat_ms_ = 0;
  uint32_t last_summary_ms_ = 0;
  uint32_t last_peer_dump_ms_ = 0;
  bool fix_logged_ = false;
  bool instrumentation_enabled_ = false;
  RadioRole role_ = RadioRole::RESP;
  uint16_t short_id_ = 0;
  char short_id_hex_[5] = {0};
  char mac_hex_[18] = {0};
  char bt_short_[5] = {0};
  domain::Logger event_logger_;
  M1Runtime runtime_;
  OledStatus oled_;
  ProvisioningAdapter* provisioning_ = nullptr;
};

}  // namespace naviga
