#include "domain/beacon_send_policy.h"

namespace naviga {
namespace domain {

namespace {

uint32_t lcg_next(uint32_t& state) {
  state = state * 1664525u + 1013904223u;
  return state;
}

} // namespace

void BeaconSendPolicy::init(uint32_t seed) {
  rng_state_ = seed == 0 ? 1u : seed;
  backoff_ms_ = 0;
  pending_ = false;
  next_attempt_ms_ = 0;
}

void BeaconSendPolicy::set_jitter_ms(uint32_t max_jitter_ms) {
  jitter_ms_ = max_jitter_ms;
}

void BeaconSendPolicy::set_backoff_ms(uint32_t base_ms, uint32_t max_ms) {
  base_backoff_ms_ = base_ms;
  max_backoff_ms_ = max_ms;
}

void BeaconSendPolicy::enable_sense(bool enabled) {
  sense_enabled_ = enabled;
}

bool BeaconSendPolicy::has_pending() const {
  return pending_;
}

bool BeaconSendPolicy::ready_to_attempt(uint32_t now_ms) const {
  return pending_ && (now_ms >= next_attempt_ms_);
}

bool BeaconSendPolicy::should_sense(const IChannelSense* sense) const {
  return sense_enabled_ && sense && sense->can_sense();
}

void BeaconSendPolicy::on_payload_built(uint32_t now_ms) {
  pending_ = true;
  schedule_after(now_ms, next_jitter_ms());
}

void BeaconSendPolicy::on_channel_busy(uint32_t now_ms) {
  increase_backoff();
  schedule_after(now_ms, backoff_ms_);
}

void BeaconSendPolicy::on_send_result(bool ok, uint32_t now_ms) {
  if (ok) {
    pending_ = false;
    backoff_ms_ = 0;
    return;
  }
  increase_backoff();
  schedule_after(now_ms, backoff_ms_);
}

uint32_t BeaconSendPolicy::next_jitter_ms() {
  if (jitter_ms_ == 0) {
    return 0;
  }
  return lcg_next(rng_state_) % (jitter_ms_ + 1);
}

void BeaconSendPolicy::schedule_after(uint32_t now_ms, uint32_t delay_ms) {
  next_attempt_ms_ = now_ms + delay_ms;
}

void BeaconSendPolicy::increase_backoff() {
  if (backoff_ms_ == 0) {
    backoff_ms_ = base_backoff_ms_;
    return;
  }
  const uint32_t doubled = backoff_ms_ * 2;
  backoff_ms_ = doubled > max_backoff_ms_ ? max_backoff_ms_ : doubled;
}

} // namespace domain
} // namespace naviga
