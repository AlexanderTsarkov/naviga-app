#pragma once

#include <cstdint>

namespace naviga {
namespace domain {

enum class LogEventId : uint16_t {
  RADIO_TX_OK = 0x0101,
  RADIO_TX_ERR = 0x0102,
  RADIO_RX_OK = 0x0103,
  RADIO_RX_ERR = 0x0104,
  DECODE_OK = 0x0201,
  DECODE_ERR = 0x0202,
  NODETABLE_UPDATE = 0x0301,
  BLE_CONNECT = 0x0401,
  BLE_DISCONNECT = 0x0402,
  BLE_READ = 0x0403,
};

} // namespace domain
} // namespace naviga
