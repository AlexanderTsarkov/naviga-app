#pragma once

#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

enum class RadioRole : uint8_t {
  INIT = 0,
  RESP = 1,
};

struct RadioSmokeStats {
  uint32_t tx_count = 0;
  uint32_t rx_count = 0;
  uint32_t last_seq = 0;
  uint32_t last_tx_ms = 0;
  uint32_t last_rx_ms = 0;
  int8_t last_rssi_dbm = 0;
  bool rssi_available = false;
  bool radio_ready = false;
  bool last_rx_was_pong = false;
};

class RadioSmokeService {
 public:
  void init(IRadio* radio, RadioRole role, bool radio_ready, bool rssi_available);
  void tick(uint32_t now_ms);

  const RadioSmokeStats& stats() const;
  RadioRole role() const;

 private:
  void send_ping(uint32_t now_ms);
  void send_pong(uint32_t seq, uint32_t now_ms);
  void handle_rx(uint32_t now_ms);

  IRadio* radio_ = nullptr;
  RadioRole role_ = RadioRole::RESP;
  uint32_t next_ping_ms_ = 0;
  RadioSmokeStats stats_{};
};

} // namespace naviga
