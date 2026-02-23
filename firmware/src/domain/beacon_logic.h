#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/node_table.h"
#include "../../protocol/geo_beacon_codec.h"

namespace naviga {
namespace domain {

class BeaconLogic {
 public:
  BeaconLogic();

  void set_min_interval_ms(uint32_t min_interval_ms);
  void set_max_silence_ms(uint32_t max_silence_ms);

  bool build_tx(uint32_t now_ms,
                const protocol::GeoBeaconFields& self_fields,
                uint8_t* out,
                size_t out_cap,
                size_t* out_len);

  bool on_rx(uint32_t now_ms,
             const uint8_t* payload,
             size_t len,
             int8_t rssi_dbm,
             NodeTable& table,
             uint64_t* out_node_id = nullptr,
             uint16_t* out_seq = nullptr);

  uint16_t seq() const {
    return seq_;
  }

 private:
  uint32_t min_interval_ms_ = 5000;
  uint32_t max_silence_ms_ = 30000;
  uint32_t last_tx_ms_ = 0;
  uint16_t seq_ = 0;
};

} // namespace domain
} // namespace naviga
