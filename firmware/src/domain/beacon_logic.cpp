#include "domain/beacon_logic.h"

namespace naviga {
namespace domain {

BeaconLogic::BeaconLogic() = default;

void BeaconLogic::set_min_interval_ms(uint32_t min_interval_ms) {
  min_interval_ms_ = min_interval_ms;
}

void BeaconLogic::set_max_silence_ms(uint32_t max_silence_ms) {
  max_silence_ms_ = max_silence_ms;
}

bool BeaconLogic::build_tx(uint32_t now_ms,
                           const protocol::GeoBeaconFields& self_fields,
                           uint8_t* out,
                           size_t out_cap,
                           size_t* out_len,
                           PacketLogType* out_type,
                           uint16_t* out_core_seq,
                           bool allow_core_at_min_interval) {
  if (!out || !out_len) {
    return false;
  }

  const uint32_t elapsed = (last_tx_ms_ == 0) ? now_ms : (now_ms - last_tx_ms_);
  const bool time_for_min = elapsed >= min_interval_ms_;
  const bool time_for_silence = max_silence_ms_ > 0 && elapsed >= max_silence_ms_;
  if (!time_for_min && !time_for_silence) {
    *out_len = 0;
    return false;
  }

  // CORE = position-bearing; ALIVE = liveness only. Per field_cadence_v0: at maxSilence send CORE
  // if fix, ALIVE if no fix. At min_interval: send CORE only if allow_core (position just updated
  // per minDisplacement); else ALIVE so we don't send position-bearing until moved.
  const bool send_position = time_for_silence
      ? (self_fields.pos_valid != 0)
      : (allow_core_at_min_interval && self_fields.pos_valid != 0);
  const PacketLogType ptype = send_position ? PacketLogType::CORE : PacketLogType::ALIVE;
  if (out_type) {
    *out_type = ptype;
  }
  if (out_core_seq) {
    *out_core_seq = 0;  // no tail on-air yet; core_seq only for TAIL1/TAIL2
  }

  protocol::GeoBeaconFields fields = self_fields;
  if (!send_position) {
    fields.pos_valid = 0;  // encode as ALIVE (no position)
  }
  fields.seq = static_cast<uint16_t>(seq_ + 1u);
  const size_t written = protocol::encode_geo_beacon(fields, protocol::ByteSpan{out, out_cap});
  if (written == 0) {
    *out_len = 0;
    return false;
  }

  *out_len = written;
  seq_ = fields.seq;
  last_tx_ms_ = now_ms;
  return true;
}

bool BeaconLogic::on_rx(uint32_t now_ms,
                        const uint8_t* payload,
                        size_t len,
                        int8_t rssi_dbm,
                        NodeTable& table,
                        uint64_t* out_node_id,
                        uint16_t* out_seq,
                        bool* out_pos_valid,
                        PacketLogType* out_type,
                        uint16_t* out_core_seq) {
  protocol::GeoBeaconFields fields{};
  const protocol::DecodeError err =
      protocol::decode_geo_beacon(protocol::ConstByteSpan{payload, len}, &fields);
  if (err != protocol::DecodeError::Ok) {
    return false;
  }
  if (out_node_id) {
    *out_node_id = fields.node_id;
  }
  if (out_seq) {
    *out_seq = fields.seq;
  }
  if (out_pos_valid) {
    *out_pos_valid = (fields.pos_valid != 0);
  }
  if (out_type) {
    *out_type = (fields.pos_valid != 0) ? PacketLogType::CORE : PacketLogType::ALIVE;
  }
  if (out_core_seq) {
    *out_core_seq = 0;  // single format has no tail; core_seq only when tail decoded
  }
  return table.upsert_remote(fields.node_id,
                             fields.pos_valid != 0,
                             fields.lat_e7,
                             fields.lon_e7,
                             fields.pos_age_s,
                             rssi_dbm,
                             fields.seq,
                             now_ms);
}

} // namespace domain
} // namespace naviga
