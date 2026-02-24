#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/node_table.h"
#include "../../protocol/geo_beacon_codec.h"

namespace naviga {
namespace domain {

/** Packet type for instrumentation logs. On-air we have a single format; we log CORE/ALIVE from intent/pos_valid. TAIL1/TAIL2 reserved for when tail packets exist. */
enum class PacketLogType {
  CORE,
  TAIL1,
  TAIL2,
  ALIVE,
};

class BeaconLogic {
 public:
  BeaconLogic();

  void set_min_interval_ms(uint32_t min_interval_ms);
  void set_max_silence_ms(uint32_t max_silence_ms);

  /** \a allow_core_at_min_interval: when false and trigger is min_interval, send ALIVE (no position).
   * Set true when position was just updated (SelfUpdatePolicy committed); per minDisplacement gating. */
  bool build_tx(uint32_t now_ms,
                const protocol::GeoBeaconFields& self_fields,
                uint8_t* out,
                size_t out_cap,
                size_t* out_len,
                PacketLogType* out_type = nullptr,
                uint16_t* out_core_seq = nullptr,
                bool allow_core_at_min_interval = true);

  bool on_rx(uint32_t now_ms,
             const uint8_t* payload,
             size_t len,
             int8_t rssi_dbm,
             NodeTable& table,
             uint64_t* out_node_id = nullptr,
             uint16_t* out_seq = nullptr,
             bool* out_pos_valid = nullptr,
             PacketLogType* out_type = nullptr,
             uint16_t* out_core_seq = nullptr);

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
