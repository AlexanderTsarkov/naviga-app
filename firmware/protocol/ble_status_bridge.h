#pragma once

#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {
namespace protocol {

class BleStatusBridge {
 public:
  static constexpr uint8_t kStatusFormatVer = 1;
  static constexpr uint8_t kTlvTypeGnss = 1;

  bool update_status(uint32_t now_ms, const GnssSnapshot& gnss_snapshot, IBleTransport& transport) const;
};

} // namespace protocol
} // namespace naviga
