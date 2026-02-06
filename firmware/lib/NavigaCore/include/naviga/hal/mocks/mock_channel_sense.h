#pragma once

#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

class MockChannelSense : public IChannelSense {
 public:
  void set_can_sense(bool enabled);
  void set_next_result(ChannelSenseResult result);
  int sense_calls() const;

  bool can_sense() const override;
  ChannelSenseResult sense(uint32_t timeout_ms) override;

 private:
  bool can_sense_ = true;
  ChannelSenseResult next_result_{ChannelSenseState::UNSUPPORTED, 0};
  int sense_calls_ = 0;
};

} // namespace naviga
