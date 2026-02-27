#include "domain/beacon_logic.h"

#include <cmath>

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
  const bool time_for_min     = elapsed >= min_interval_ms_;
  const bool time_for_silence = max_silence_ms_ > 0 && elapsed >= max_silence_ms_;
  if (!time_for_min && !time_for_silence) {
    *out_len = 0;
    return false;
  }

  // Per field_cadence_v0: at min_interval without position commit → no send.
  if (time_for_min && !allow_core_at_min_interval) {
    *out_len = 0;
    return false;
  }

  const uint16_t next_seq = static_cast<uint16_t>(seq_ + 1u);

  if (self_fields.pos_valid != 0) {
    // Position valid → send BeaconCore (msg_type=0x01).
    protocol::GeoBeaconFields fields = self_fields;
    fields.seq = next_seq;
    const size_t written = protocol::encode_geo_beacon(
        fields, protocol::ByteSpan{out, out_cap});
    if (written == 0) {
      *out_len = 0;
      return false;
    }
    *out_len = written;
    seq_ = next_seq;
    last_tx_ms_ = now_ms;
    if (out_type)     { *out_type = PacketLogType::CORE; }
    if (out_core_seq) { *out_core_seq = 0; }
    return true;
  }

  // No fix → send Alive only at maxSilence (field_cadence_v0).
  if (!time_for_silence) {
    *out_len = 0;
    return false;
  }

  // Send Alive (msg_type=0x02).
  protocol::AliveFields alive{};
  alive.node_id    = self_fields.node_id;
  alive.seq        = next_seq;
  alive.has_status = false;
  const size_t written = protocol::encode_alive_frame(alive, out, out_cap);
  if (written == 0) {
    *out_len = 0;
    return false;
  }
  *out_len = written;
  seq_ = next_seq;
  last_tx_ms_ = now_ms;
  if (out_type)     { *out_type = PacketLogType::ALIVE; }
  if (out_core_seq) { *out_core_seq = 0; }
  return true;
}

bool BeaconLogic::on_rx(uint32_t now_ms,
                        const uint8_t* frame,
                        size_t len,
                        int8_t rssi_dbm,
                        NodeTable& table,
                        uint64_t* out_node_id,
                        uint16_t* out_seq,
                        bool* out_pos_valid,
                        PacketLogType* out_type,
                        uint16_t* out_core_seq) {
  if (!frame || len < protocol::kHeaderSize) {
    return false;
  }

  // Dispatch on msg_type from the 2-byte frame header.
  protocol::PacketHeader hdr;
  if (!protocol::decode_header(frame, len, &hdr)) {
    return false;  // unknown or reserved msg_type → drop
  }
  if (!protocol::validate_header(hdr, len - protocol::kHeaderSize)) {
    return false;  // payload_len mismatch → drop
  }

  const uint8_t* payload     = frame + protocol::kHeaderSize;
  const size_t   payload_len = len - protocol::kHeaderSize;

  if (hdr.msg_type == protocol::MsgType::BeaconCore) {
    protocol::GeoBeaconFields fields{};
    const protocol::DecodeError err =
        protocol::decode_geo_beacon(protocol::ConstByteSpan{payload, payload_len}, &fields);
    if (err != protocol::DecodeError::Ok) {
      return false;
    }
    if (out_node_id)  { *out_node_id  = fields.node_id; }
    if (out_seq)      { *out_seq      = fields.seq; }
    if (out_pos_valid){ *out_pos_valid = (fields.pos_valid != 0); }
    if (out_type)     { *out_type     = PacketLogType::CORE; }
    if (out_core_seq) { *out_core_seq = 0; }

    const int32_t lat_e7 = static_cast<int32_t>(std::llround(fields.lat_deg * 1e7));
    const int32_t lon_e7 = static_cast<int32_t>(std::llround(fields.lon_deg * 1e7));
    return table.upsert_remote(fields.node_id,
                               true,
                               lat_e7,
                               lon_e7,
                               0,  // pos_age_s not in BeaconCore v0
                               rssi_dbm,
                               fields.seq,
                               now_ms);
  }

  if (hdr.msg_type == protocol::MsgType::BeaconAlive) {
    protocol::AliveFields alive{};
    const protocol::AliveDecodeError err =
        protocol::decode_alive_payload(payload, payload_len, &alive);
    if (err != protocol::AliveDecodeError::Ok) {
      return false;
    }
    if (out_node_id)  { *out_node_id  = alive.node_id; }
    if (out_seq)      { *out_seq      = alive.seq; }
    if (out_pos_valid){ *out_pos_valid = false; }
    if (out_type)     { *out_type     = PacketLogType::ALIVE; }
    if (out_core_seq) { *out_core_seq = 0; }

    // Alive updates liveness (lastRxAt, seq) but does not carry position.
    return table.upsert_remote(alive.node_id,
                               false,
                               0,
                               0,
                               0,
                               rssi_dbm,
                               alive.seq,
                               now_ms);
  }

  if (hdr.msg_type == protocol::MsgType::BeaconTail1) {
    protocol::Tail1Fields tail1{};
    const protocol::Tail1DecodeError err =
        protocol::decode_tail1_payload(payload, payload_len, &tail1);
    if (err != protocol::Tail1DecodeError::Ok) {
      return false;
    }
    if (out_node_id)  { *out_node_id  = tail1.node_id; }
    if (out_seq)      { *out_seq      = tail1.ref_core_seq16; }
    if (out_pos_valid){ *out_pos_valid = false; }
    if (out_type)     { *out_type     = PacketLogType::TAIL1; }
    if (out_core_seq) { *out_core_seq = tail1.ref_core_seq16; }

    // Apply Tail-1: ref_core_seq16 match enforced inside apply_tail1.
    // Return value indicates match (true) or silent drop (false).
    // Either way we return true to the caller — the frame was valid.
    table.apply_tail1(tail1.node_id,
                      tail1.ref_core_seq16,
                      tail1.has_pos_flags, tail1.pos_flags,
                      tail1.has_sats, tail1.sats,
                      rssi_dbm,
                      now_ms);
    return true;
  }

  if (hdr.msg_type == protocol::MsgType::BeaconTail2) {
    protocol::Tail2Fields tail2{};
    const protocol::Tail2DecodeError err =
        protocol::decode_tail2_payload(payload, payload_len, &tail2);
    if (err != protocol::Tail2DecodeError::Ok) {
      return false;
    }
    if (out_node_id)  { *out_node_id  = tail2.node_id; }
    if (out_seq)      { *out_seq      = 0; }
    if (out_pos_valid){ *out_pos_valid = false; }
    if (out_type)     { *out_type     = PacketLogType::TAIL2; }
    if (out_core_seq) { *out_core_seq = 0; }

    return table.apply_tail2(tail2.node_id,
                             tail2.has_max_silence, tail2.max_silence_10s,
                             tail2.has_battery,     tail2.battery_percent,
                             tail2.has_hw_profile,  tail2.hw_profile_id,
                             tail2.has_fw_version,  tail2.fw_version_id,
                             tail2.has_uptime,      tail2.uptime_sec,
                             rssi_dbm,
                             now_ms);
  }

  // Unknown/future msg_type → drop.
  return false;
}

} // namespace domain
} // namespace naviga
