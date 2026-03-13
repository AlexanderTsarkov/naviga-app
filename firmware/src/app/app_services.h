#pragma once

#include <cstdint>

#include "app/m1_runtime.h"
#include "domain/beacon_logic.h"
#include "domain/logger.h"
#include "naviga/hal/interfaces.h"
#include "services/gnss_scenario_override.h"
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
  uint32_t last_gnss_override_log_ms_ = 0;
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
  GnssScenarioOverride gnss_override_;

  // Self telemetry for 0x04/0x05 formation. Populated in init() (static fields)
  // and updated each tick() (dynamic fields). Passed to runtime_ before tick().
  domain::SelfTelemetry self_telemetry_{};
  // maxSilence10s from the active role profile; persisted from init() for tick() use.
  uint8_t effective_max_silence_10s_ = 0;
  // #417: per-TX persistence; validity separate so seq16 0 (wraparound) is persisted.
  uint16_t last_persisted_seq16_ = 0;
  bool has_persisted_seq16_ = false;
  // #418: NodeTable snapshot save debounce (dirty + min interval).
  uint32_t last_nodetable_save_ms_ = 0;

  // #450: effective role/profile/tx for OLED (set once in init).
  uint32_t effective_role_id_ = 0;
  uint16_t effective_min_interval_sec_ = 0;
  uint16_t effective_min_distance_m_ = 0;
  uint8_t effective_tx_power_dbm_ = 21;
  GNSSFixState last_fix_state_ = GNSSFixState::NO_FIX;
  static constexpr size_t kOledDisplayNameLen = 33;
  char oled_display_name_buf_[kOledDisplayNameLen] = {0};
};

}  // namespace naviga
