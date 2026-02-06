#include "naviga/hal/mocks/mock_channel_sense.h"

namespace naviga {

void MockChannelSense::set_can_sense(bool enabled) {
  can_sense_ = enabled;
}

void MockChannelSense::set_next_result(ChannelSenseResult result) {
  next_result_ = result;
}

int MockChannelSense::sense_calls() const {
  return sense_calls_;
}

bool MockChannelSense::can_sense() const {
  return can_sense_;
}

ChannelSenseResult MockChannelSense::sense(uint32_t /*timeout_ms*/) {
  sense_calls_++;
  return next_result_;
}

} // namespace naviga
