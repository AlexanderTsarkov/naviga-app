#pragma once

#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {
namespace domain {

class BeaconSendPolicy {
 public:
  void init(uint32_t seed);
  void set_jitter_ms(uint32_t max_jitter_ms);
  void set_backoff_ms(uint32_t base_ms, uint32_t max_ms);
  void enable_sense(bool enabled);

  bool has_pending() const;
  bool ready_to_attempt(uint32_t now_ms) const;
  bool should_sense(const IChannelSense* sense) const;

  void on_payload_built(uint32_t now_ms);
  void on_channel_busy(uint32_t now_ms);
  void on_send_result(bool ok, uint32_t now_ms);

 private:
  uint32_t next_attempt_ms_ = 0;
  uint32_t jitter_ms_ = 0;
  uint32_t base_backoff_ms_ = 200;
  uint32_t max_backoff_ms_ = 2000;
  uint32_t backoff_ms_ = 0;
  uint32_t rng_state_ = 1;
  bool pending_ = false;
  bool sense_enabled_ = false;

  uint32_t next_jitter_ms();
  void schedule_after(uint32_t now_ms, uint32_t delay_ms);
  void increase_backoff();
};

} // namespace domain
} // namespace naviga
