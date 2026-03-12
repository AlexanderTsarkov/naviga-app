#include "domain/beacon_logic.h"

#include <cmath>
#include <cstring>

#include "../../protocol/pos_full_codec.h"
#include "../../protocol/status_codec.h"

namespace naviga {
namespace domain {

BeaconLogic::BeaconLogic() = default;

void BeaconLogic::set_min_interval_ms(uint32_t min_interval_ms) {
  min_interval_ms_ = min_interval_ms;
}

void BeaconLogic::set_max_silence_ms(uint32_t max_silence_ms) {
  max_silence_ms_ = max_silence_ms;
}

void BeaconLogic::set_min_status_interval_ms(uint32_t ms) {
  min_status_interval_ms_ = ms;
}

void BeaconLogic::set_T_status_max_ms(uint32_t ms) {
  T_status_max_ms_ = ms;
}

void BeaconLogic::on_status_sent(uint32_t now_ms) {
  last_status_tx_ms_ = now_ms;
  if (status_bootstrap_count_ < 2) {
    status_bootstrap_count_++;
  }
}

void BeaconLogic::set_initial_seq16(uint16_t value) {
  seq_ = value;
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

  const uint16_t seq = next_seq16();

  if (self_fields.pos_valid != 0) {
    // Position valid → send Node_Pos_Full v0.2 (msg_type=0x06).
    protocol::PosFullFields pos{};
    pos.node_id = self_fields.node_id;
    pos.seq16 = seq;
    pos.lat_deg = self_fields.lat_deg;
    pos.lon_deg = self_fields.lon_deg;
    pos.fix_type = 1;
    pos.pos_sats = 0;
    pos.pos_accuracy_bucket = 0;
    pos.pos_flags_small = 0;
    const size_t written = protocol::encode_pos_full_frame(pos, out, out_cap);
    if (written == 0) {
      seq_ = static_cast<uint16_t>(seq_ - 1u);
      *out_len = 0;
      return false;
    }
    *out_len = written;
    last_tx_ms_ = now_ms;
    if (out_type)     { *out_type = PacketLogType::POS_FULL; }
    if (out_core_seq) { *out_core_seq = 0; }
    return true;
  }

  if (!time_for_silence) {
    seq_ = static_cast<uint16_t>(seq_ - 1u);
    *out_len = 0;
    return false;
  }

  protocol::AliveFields alive{};
  alive.node_id = self_fields.node_id;
  alive.seq = seq;
  alive.has_status = false;
  const size_t written = protocol::encode_alive_frame(alive, out, out_cap);
  if (written == 0) {
    seq_ = static_cast<uint16_t>(seq_ - 1u);
    *out_len = 0;
    return false;
  }
  *out_len = written;
  last_tx_ms_ = now_ms;
  if (out_type)     { *out_type = PacketLogType::ALIVE; }
  if (out_core_seq) { *out_core_seq = 0; }
  return true;
}

// ── Slot-based TX queue API (#435 v0.2) ───────────────────────────────────────
//
// TX sends v0.2 only: Node_Pos_Full (0x06), Node_Status (0x07), Alive (0x02).
// Formation: PosFull when pos_valid and (time_for_min+allow_core) or time_for_silence;
// Alive when !pos_valid and time_for_silence; Status per lifecycle (min_status_interval_ms,
// T_status_max, bootstrap, no hitchhiking). See packet_truth_table_v02.md, packet_migration_v01_v02.md.

void BeaconLogic::update_tx_queue(uint32_t now_ms,
                                  const protocol::GeoBeaconFields& self_fields,
                                  const SelfTelemetry& telemetry,
                                  bool allow_core) {
  const uint32_t elapsed = (last_tx_ms_ == 0) ? now_ms : (now_ms - last_tx_ms_);
  const bool time_for_min     = elapsed >= min_interval_ms_;
  const bool time_for_silence = max_silence_ms_ > 0 && elapsed >= max_silence_ms_;

  bool pos_or_alive_enqueued = false;

  // ── Node_Pos_Full (0x06) / Alive (0x02) formation ──────────────────────────
  if (self_fields.pos_valid != 0) {
    const bool should_pos = (time_for_min && allow_core) || time_for_silence;
    if (should_pos) {
      const uint16_t seq = next_seq16();
      protocol::PosFullFields pos{};
      pos.node_id = self_fields.node_id;
      pos.seq16 = seq;
      pos.lat_deg = self_fields.lat_deg;
      pos.lon_deg = self_fields.lon_deg;
      pos.fix_type = 1;
      pos.pos_sats = 0;
      pos.pos_accuracy_bucket = 0;
      pos.pos_flags_small = 0;
      uint8_t pos_frame[protocol::kPosFullFrameSize] = {};
      const size_t pos_len = protocol::encode_pos_full_frame(pos, pos_frame, sizeof(pos_frame));
      if (pos_len > 0) {
        enqueue_slot(kSlotPosFull, TxPriority::P0_MUST_PERIODIC, TxBestEffortClass::BE_LOW,
                     PacketLogType::POS_FULL, pos_frame, pos_len, now_ms, 0);
        last_tx_ms_ = now_ms;
        pos_or_alive_enqueued = true;
      }
    }
  } else {
    if (time_for_silence) {
      const uint16_t alive_seq = next_seq16();
      protocol::AliveFields alive{};
      alive.node_id = self_fields.node_id;
      alive.seq = alive_seq;
      alive.has_status = false;
      uint8_t alive_frame[protocol::kAliveFrameMin] = {};
      const size_t alive_len = protocol::encode_alive_frame(alive, alive_frame, sizeof(alive_frame));
      if (alive_len > 0) {
        enqueue_slot(kSlotAlive, TxPriority::P0_MUST_PERIODIC, TxBestEffortClass::BE_LOW,
                     PacketLogType::ALIVE, alive_frame, alive_len, now_ms, 0);
        last_tx_ms_ = now_ms;
        pos_or_alive_enqueued = true;
      }
    }
  }

  // ── Node_Status (0x07) formation (#435: full snapshot, no hitchhiking) ───────
  const bool status_interval_ok = (last_status_tx_ms_ == 0) || (now_ms - last_status_tx_ms_ >= min_status_interval_ms_);
  const bool status_T_max_force = (last_status_enqueue_ms_ != 0) && (now_ms - last_status_enqueue_ms_ >= T_status_max_ms_);
  const bool status_bootstrap_ok = status_bootstrap_count_ < 2;
  const bool status_throttle_ok = status_bootstrap_ok || status_interval_ok || status_T_max_force;
  const bool status_eligible = !pos_or_alive_enqueued && (time_for_min || time_for_silence) && status_throttle_ok;

  const bool has_status = telemetry.has_battery || telemetry.has_uptime ||
                          telemetry.has_max_silence || telemetry.has_hw_profile || telemetry.has_fw_version;

  if (status_eligible && has_status) {
    const uint16_t status_seq = next_seq16();
    protocol::StatusFields st{};
    st.node_id = self_fields.node_id;
    st.seq16 = status_seq;
    st.battery_percent = telemetry.battery_percent;
    st.battery_est_rem_time = 0xFF;
    st.tx_power_ch_throttle = 0;
    st.uptime10m = (telemetry.uptime_sec != 0xFFFFFFFFu) ? static_cast<uint8_t>(telemetry.uptime_sec / 600u) : 0u;
    st.role_id = telemetry.role_id;
    st.max_silence_10s = telemetry.max_silence_10s;
    st.hw_profile_id = telemetry.hw_profile_id;
    st.fw_version_id = telemetry.fw_version_id;
    uint8_t status_frame[protocol::kStatusFrameSize] = {};
    const size_t status_len = protocol::encode_status_frame(st, status_frame, sizeof(status_frame));
    if (status_len > 0) {
      enqueue_slot(kSlotStatus, TxPriority::P3_THROTTLED, TxBestEffortClass::BE_LOW,
                   PacketLogType::STATUS, status_frame, status_len, now_ms, 0);
      last_status_enqueue_ms_ = now_ms;
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
  //   1. TxPriority (primary): P0 > P1 > P2 > P3
  //   2. TxBestEffortClass be_rank (within P2 only; P3 slots use BE_LOW)
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

  // Starvation increment: every other present slot was due but not sent this round.
  const size_t best_u = static_cast<size_t>(best);
  for (size_t i = 0; i < kTxSlotCount; ++i) {
    if (i != best_u && slots_[i].present) {
      slots_[i].replaced_count++;
    }
  }

  // Clear the selected slot (reset on send).
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

  // v0.2 Node_Pos_Full (0x06) — single-packet position + Pos_Quality (#435).
  if (hdr.msg_type == protocol::MsgType::BeaconPosFull) {
    protocol::PosFullFields pos{};
    const protocol::PosFullDecodeError err =
        protocol::decode_pos_full_payload(payload, payload_len, &pos);
    if (err != protocol::PosFullDecodeError::Ok) {
      return false;
    }
    if (out_node_id)  { *out_node_id  = pos.node_id; }
    if (out_seq)      { *out_seq      = pos.seq16; }
    if (out_pos_valid){ *out_pos_valid = true; }
    if (out_type)     { *out_type     = PacketLogType::POS_FULL; }
    if (out_core_seq) { *out_core_seq = 0; }

    const int32_t lat_e7 = static_cast<int32_t>(std::llround(pos.lat_deg * 1e7));
    const int32_t lon_e7 = static_cast<int32_t>(std::llround(pos.lon_deg * 1e7));
    return table.apply_pos_full(pos.node_id, pos.seq16,
                                lat_e7, lon_e7,
                                pos.fix_type, pos.pos_sats,
                                pos.pos_accuracy_bucket, pos.pos_flags_small,
                                rssi_dbm, now_ms);
  }

  // v0.2 Node_Status (0x07) — full status snapshot (#435).
  if (hdr.msg_type == protocol::MsgType::BeaconStatus) {
    protocol::StatusFields st{};
    const protocol::StatusDecodeError err =
        protocol::decode_status_payload(payload, payload_len, &st);
    if (err != protocol::StatusDecodeError::Ok) {
      return false;
    }
    if (out_node_id)  { *out_node_id  = st.node_id; }
    if (out_seq)      { *out_seq      = st.seq16; }
    if (out_pos_valid){ *out_pos_valid = false; }
    if (out_type)     { *out_type     = PacketLogType::STATUS; }
    if (out_core_seq) { *out_core_seq = 0; }

    return table.apply_status(st.node_id, st.seq16,
                              st.battery_percent, st.battery_est_rem_time,
                              st.tx_power_ch_throttle, st.uptime10m,
                              st.role_id, st.max_silence_10s,
                              st.hw_profile_id, st.fw_version_id,
                              rssi_dbm, now_ms);
  }

  return false;
}

} // namespace domain
} // namespace naviga
