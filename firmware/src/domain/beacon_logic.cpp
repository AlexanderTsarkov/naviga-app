#include "domain/beacon_logic.h"

#include <cmath>
#include <cstring>

namespace naviga {
namespace domain {

BeaconLogic::BeaconLogic() = default;

void BeaconLogic::set_min_interval_ms(uint32_t min_interval_ms) {
  min_interval_ms_ = min_interval_ms;
}

void BeaconLogic::set_max_silence_ms(uint32_t max_silence_ms) {
  max_silence_ms_ = max_silence_ms;
}

uint16_t BeaconLogic::next_seq16() {
  seq_ = static_cast<uint16_t>(seq_ + 1u);
  return seq_;
}

void BeaconLogic::enqueue_slot(size_t slot_idx,
                               TxPriority priority,
                               TxBestEffortClass be_rank,
                               PacketLogType pkt_type,
                               const uint8_t* frame,
                               size_t frame_len,
                               uint32_t now_ms,
                               uint16_t ref_core_seq16) {
  TxSlot& slot = slots_[slot_idx];
  if (slot.present) {
    slot.replaced_count++;
    // created_at_ms preserved intentionally (fairness: age from first enqueue)
  } else {
    slot.created_at_ms  = now_ms;
    slot.replaced_count = 0;
  }
  slot.present        = true;
  slot.priority       = priority;
  slot.be_rank        = be_rank;
  slot.pkt_type       = pkt_type;
  slot.ref_core_seq16 = ref_core_seq16;
  slot.frame_len      = frame_len;
  if (frame_len > 0 && frame_len <= protocol::kMaxFrameSize) {
    std::memcpy(slot.frame, frame, frame_len);
  }
}

// ── Legacy single-packet TX API ──────────────────────────────────────────────

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

  const uint16_t core_seq = next_seq16();

  if (self_fields.pos_valid != 0) {
    // Position valid → send BeaconCore (msg_type=0x01).
    protocol::GeoBeaconFields fields = self_fields;
    fields.seq = core_seq;
    const size_t written = protocol::encode_geo_beacon(
        fields, protocol::ByteSpan{out, out_cap});
    if (written == 0) {
      seq_ = static_cast<uint16_t>(seq_ - 1u);  // roll back unused seq
      *out_len = 0;
      return false;
    }
    *out_len = written;
    last_tx_ms_ = now_ms;
    if (out_type)     { *out_type = PacketLogType::CORE; }
    if (out_core_seq) { *out_core_seq = 0; }
    return true;
  }

  // No fix → send Alive only at maxSilence (field_cadence_v0).
  if (!time_for_silence) {
    seq_ = static_cast<uint16_t>(seq_ - 1u);  // roll back unused seq
    *out_len = 0;
    return false;
  }

  // Send Alive (msg_type=0x02).
  protocol::AliveFields alive{};
  alive.node_id    = self_fields.node_id;
  alive.seq        = core_seq;
  alive.has_status = false;
  const size_t written = protocol::encode_alive_frame(alive, out, out_cap);
  if (written == 0) {
    seq_ = static_cast<uint16_t>(seq_ - 1u);  // roll back unused seq
    *out_len = 0;
    return false;
  }
  *out_len = written;
  last_tx_ms_ = now_ms;
  if (out_type)     { *out_type = PacketLogType::ALIVE; }
  if (out_core_seq) { *out_core_seq = 0; }
  return true;
}

// ── Slot-based TX queue API ──────────────────────────────────────────────────

void BeaconLogic::update_tx_queue(uint32_t now_ms,
                                  const protocol::GeoBeaconFields& self_fields,
                                  const SelfTelemetry& telemetry,
                                  bool allow_core) {
  const uint32_t elapsed = (last_tx_ms_ == 0) ? now_ms : (now_ms - last_tx_ms_);
  const bool time_for_min     = elapsed >= min_interval_ms_;
  const bool time_for_silence = max_silence_ms_ > 0 && elapsed >= max_silence_ms_;

  // ── Core_Pos / Alive formation ────────────────────────────────────────────
  if (self_fields.pos_valid != 0) {
    // Core_Pos: enqueue when time_for_min (with allow_core gate) or time_for_silence.
    const bool should_core = (time_for_min && allow_core) || time_for_silence;
    if (should_core) {
      const uint16_t core_seq = next_seq16();

      // Encode Core_Pos frame.
      protocol::GeoBeaconFields fields = self_fields;
      fields.seq = core_seq;
      uint8_t core_frame[protocol::kGeoBeaconFrameSize] = {};
      const size_t core_len = protocol::encode_geo_beacon(
          fields, protocol::ByteSpan{core_frame, sizeof(core_frame)});
      if (core_len > 0) {
        enqueue_slot(kSlotCore, TxPriority::P0_MUST_PERIODIC, TxBestEffortClass::BE_LOW,
                     PacketLogType::CORE, core_frame, core_len, now_ms, 0);
        last_tx_ms_ = now_ms;

        // Core_Tail: formed immediately after Core_Pos, using next seq16.
        // ref_core_seq16 = core_seq; tail's own seq16 = core_seq + 1.
        const uint16_t tail_seq = next_seq16();
        protocol::Tail1Fields tail{};
        tail.node_id        = self_fields.node_id;
        tail.seq16          = tail_seq;
        tail.ref_core_seq16 = core_seq;
        uint8_t tail_frame[protocol::kTail1FrameMin] = {};
        const size_t tail_len = protocol::encode_tail1_frame(tail, tail_frame, sizeof(tail_frame));
        if (tail_len > 0) {
          // Core_Tail is P2_BEST_EFFORT / BE_HIGH: best-effort but above Op/Info.
          enqueue_slot(kSlotTail1, TxPriority::P2_BEST_EFFORT, TxBestEffortClass::BE_HIGH,
                       PacketLogType::TAIL1, tail_frame, tail_len, now_ms, core_seq);
        }
      }
    }
  } else {
    // No fix: Alive only at maxSilence.
    if (time_for_silence) {
      const uint16_t alive_seq = next_seq16();
      protocol::AliveFields alive{};
      alive.node_id    = self_fields.node_id;
      alive.seq        = alive_seq;
      alive.has_status = false;
      uint8_t alive_frame[protocol::kAliveFrameMin] = {};
      const size_t alive_len = protocol::encode_alive_frame(alive, alive_frame, sizeof(alive_frame));
      if (alive_len > 0) {
        enqueue_slot(kSlotAlive, TxPriority::P0_MUST_PERIODIC, TxBestEffortClass::BE_LOW,
                     PacketLogType::ALIVE, alive_frame, alive_len, now_ms, 0);
        last_tx_ms_ = now_ms;
      }
    }
  }

  // ── Operational (0x04) formation ─────────────────────────────────────────
  if (telemetry.has_battery || telemetry.has_uptime) {
    const uint16_t op_seq = next_seq16();
    protocol::Tail2Fields op{};
    op.node_id         = self_fields.node_id;
    op.seq16           = op_seq;
    op.has_battery     = telemetry.has_battery;
    op.battery_percent = telemetry.battery_percent;
    op.has_uptime      = telemetry.has_uptime;
    op.uptime_sec      = telemetry.uptime_sec;
    uint8_t op_frame[protocol::kTail2FrameMax] = {};
    const size_t op_len = protocol::encode_tail2_frame(op, op_frame, sizeof(op_frame));
    if (op_len > 0) {
      enqueue_slot(kSlotTail2, TxPriority::P2_BEST_EFFORT, TxBestEffortClass::BE_LOW,
                   PacketLogType::TAIL2, op_frame, op_len, now_ms, 0);
    }
  }

  // ── Informative (0x05) formation ─────────────────────────────────────────
  if (telemetry.has_max_silence || telemetry.has_hw_profile || telemetry.has_fw_version) {
    const uint16_t info_seq = next_seq16();
    protocol::InfoFields info{};
    info.node_id         = self_fields.node_id;
    info.seq16           = info_seq;
    info.has_max_silence = telemetry.has_max_silence;
    info.max_silence_10s = telemetry.max_silence_10s;
    info.has_hw_profile  = telemetry.has_hw_profile;
    info.hw_profile_id   = telemetry.hw_profile_id;
    info.has_fw_version  = telemetry.has_fw_version;
    info.fw_version_id   = telemetry.fw_version_id;
    uint8_t info_frame[protocol::kInfoFrameMax] = {};
    const size_t info_len = protocol::encode_info_frame(info, info_frame, sizeof(info_frame));
    if (info_len > 0) {
      enqueue_slot(kSlotInfo, TxPriority::P2_BEST_EFFORT, TxBestEffortClass::BE_LOW,
                   PacketLogType::INFO, info_frame, info_len, now_ms, 0);
    }
  }
}

bool BeaconLogic::dequeue_tx(uint8_t* out,
                             size_t out_cap,
                             size_t* out_len,
                             PacketLogType* out_type,
                             uint16_t* out_core_seq) {
  if (!out || !out_len) {
    return false;
  }
  *out_len = 0;

  // Selection order (lower value = higher priority in each dimension):
  //   1. TxPriority (primary)
  //   2. TxBestEffortClass be_rank (secondary; only meaningful within P2_BEST_EFFORT)
  //   3. replaced_count descending (most-starved first)
  //   4. created_at_ms ascending (oldest first)
  int best = -1;
  for (size_t i = 0; i < kTxSlotCount; ++i) {
    if (!slots_[i].present) {
      continue;
    }
    if (best < 0) {
      best = static_cast<int>(i);
      continue;
    }
    const TxSlot& b = slots_[static_cast<size_t>(best)];
    const TxSlot& c = slots_[i];

    const uint8_t bp = static_cast<uint8_t>(b.priority);
    const uint8_t cp = static_cast<uint8_t>(c.priority);
    if (cp < bp) {
      best = static_cast<int>(i);
    } else if (cp == bp) {
      const uint8_t bb = static_cast<uint8_t>(b.be_rank);
      const uint8_t cb = static_cast<uint8_t>(c.be_rank);
      if (cb < bb) {
        best = static_cast<int>(i);
      } else if (cb == bb) {
        if (c.replaced_count > b.replaced_count) {
          best = static_cast<int>(i);
        } else if (c.replaced_count == b.replaced_count) {
          // Older created_at_ms wins (smaller value = older).
          if (c.created_at_ms < b.created_at_ms) {
            best = static_cast<int>(i);
          }
        }
      }
    }
  }

  if (best < 0) {
    return false;
  }

  TxSlot& slot = slots_[static_cast<size_t>(best)];
  if (slot.frame_len > out_cap) {
    return false;
  }

  std::memcpy(out, slot.frame, slot.frame_len);
  *out_len = slot.frame_len;
  if (out_type)     { *out_type     = slot.pkt_type; }
  if (out_core_seq) { *out_core_seq = slot.ref_core_seq16; }

  // Clear the slot.
  slot = TxSlot{};

  return true;
}

bool BeaconLogic::has_pending_tx() const {
  for (size_t i = 0; i < kTxSlotCount; ++i) {
    if (slots_[i].present) {
      return true;
    }
  }
  return false;
}

// ── RX dispatch ─────────────────────────────────────────────────────────────

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
    if (out_seq)      { *out_seq      = tail1.seq16; }
    if (out_pos_valid){ *out_pos_valid = false; }
    if (out_type)     { *out_type     = PacketLogType::TAIL1; }
    if (out_core_seq) { *out_core_seq = tail1.ref_core_seq16; }

    // Apply Tail-1: ref_core_seq16 match enforced inside apply_tail1.
    // seq16 is the packet's own global counter; ref_core_seq16 is the Core linkage key.
    // Return value indicates match (true) or silent drop (false).
    // Either way we return true to the caller — the frame was valid.
    table.apply_tail1(tail1.node_id,
                      tail1.seq16,
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
    if (out_seq)      { *out_seq      = tail2.seq16; }
    if (out_pos_valid){ *out_pos_valid = false; }
    if (out_type)     { *out_type     = PacketLogType::TAIL2; }
    if (out_core_seq) { *out_core_seq = 0; }

    return table.apply_tail2(tail2.node_id,
                             tail2.seq16,
                             tail2.has_battery,  tail2.battery_percent,
                             tail2.has_uptime,   tail2.uptime_sec,
                             rssi_dbm,
                             now_ms);
  }

  if (hdr.msg_type == protocol::MsgType::BeaconInfo) {
    protocol::InfoFields info{};
    const protocol::InfoDecodeError err =
        protocol::decode_info_payload(payload, payload_len, &info);
    if (err != protocol::InfoDecodeError::Ok) {
      return false;
    }
    if (out_node_id)  { *out_node_id  = info.node_id; }
    if (out_seq)      { *out_seq      = info.seq16; }
    if (out_pos_valid){ *out_pos_valid = false; }
    if (out_type)     { *out_type     = PacketLogType::INFO; }
    if (out_core_seq) { *out_core_seq = 0; }

    return table.apply_info(info.node_id,
                            info.seq16,
                            info.has_max_silence, info.max_silence_10s,
                            info.has_hw_profile,  info.hw_profile_id,
                            info.has_fw_version,  info.fw_version_id,
                            rssi_dbm,
                            now_ms);
  }

  // Unknown/future msg_type → drop.
  return false;
}

} // namespace domain
} // namespace naviga
