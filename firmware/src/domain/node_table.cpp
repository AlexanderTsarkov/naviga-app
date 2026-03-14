#include "domain/node_table.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace naviga {
namespace domain {

namespace {

constexpr uint16_t kCrc16Init = 0xFFFF;
constexpr uint16_t kCrc16Poly = 0x1021;

/** RX semantics v0: seq16 ordering with wrap. Per rx_semantics_v0 §1. */
enum class Seq16Order { Same, Newer, Older };

Seq16Order seq16_order(uint16_t incoming, uint16_t last) {
  const uint16_t delta = static_cast<uint16_t>(incoming - last);
  if (delta == 0) {
    return Seq16Order::Same;
  }
  /* Forward in ring (0..32767) = newer; backward (32768..65535) = older. */
  return (delta <= 32767u) ? Seq16Order::Newer : Seq16Order::Older;
}

void write_u16_le(uint8_t* out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void write_u32_le(uint8_t* out, uint32_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  out[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  out[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void write_u64_le(uint8_t* out, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out[i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
  }
}

uint16_t crc16_ccitt(const uint8_t* data, size_t len) {
  uint16_t crc = kCrc16Init;
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i] << 8);
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ kCrc16Poly)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

uint16_t clamp_u16(uint32_t value) {
  return value > 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(value);
}

} // namespace

uint16_t NodeTable::compute_short_id(uint64_t node_id) {
  // Canonical ShortId per nodeid_policy_v0 §4:
  // CRC16-CCITT-FALSE over the 6-byte LE wire representation of NodeID48.
  // Upper 16 bits of node_id are always 0x0000 (domain invariant).
  uint8_t bytes[6] = {0};
  for (int i = 0; i < 6; ++i) {
    bytes[i] = static_cast<uint8_t>((node_id >> (8 * i)) & 0xFF);
  }
  const uint16_t crc = crc16_ccitt(bytes, sizeof(bytes));
  // Reserved-values fix: if and only if crc == 0x0000 or crc == 0xFFFF,
  // compute short_id = crc XOR 0x0001 exactly once; otherwise short_id = crc.
  if (crc == 0x0000u || crc == 0xFFFFu) {
    return static_cast<uint16_t>(crc ^ 0x0001u);
  }
  return crc;
}

void NodeTable::set_expected_interval_s(uint16_t expected_interval_s) {
  expected_interval_s_ = expected_interval_s;
  grace_s_ = compute_grace_s(expected_interval_s_);
}

void NodeTable::init_self(uint64_t node_id, uint32_t now_ms) {
  const int existing = find_entry_index(node_id);
  if (existing >= 0) {
    NodeEntry& entry = entries_[static_cast<size_t>(existing)];
    entry.is_self = true;
    entry.last_seen_ms = now_ms;
    entry.pos_valid = false;
    entry.lat_e7 = 0;
    entry.lon_e7 = 0;
    entry.pos_age_s = 0;
    self_index_ = existing;
    recompute_collisions();
    set_dirty();
    return;
  }

  int index = find_free_index();
  if (index < 0) {
    if (!evict_oldest_grey(now_ms)) {
      // As a last resort, evict the oldest non-self entry to make room for self.
      uint32_t oldest = UINT32_MAX;
      int oldest_index = -1;
      for (size_t i = 0; i < entries_.size(); ++i) {
        if (!entries_[i].in_use || entries_[i].is_self) {
          continue;
        }
        if (entries_[i].last_seen_ms < oldest) {
          oldest = entries_[i].last_seen_ms;
          oldest_index = static_cast<int>(i);
        }
      }
      if (oldest_index >= 0) {
        entries_[static_cast<size_t>(oldest_index)] = NodeEntry{};
        size_--;
      } else {
        return;
      }
    }
    index = find_free_index();
    if (index < 0) {
      return;
    }
  }

  NodeEntry& entry = entries_[static_cast<size_t>(index)];
  entry = NodeEntry{};
  entry.node_id = node_id;
  entry.short_id = compute_short_id(node_id);
  entry.is_self = true;
  entry.last_seen_ms = now_ms;
  entry.in_use = true;
  self_index_ = index;
  size_++;
  recompute_collisions();
  set_dirty();
}

void NodeTable::update_self_position(int32_t lat_e7,
                                     int32_t lon_e7,
                                     uint16_t pos_age_s,
                                     uint32_t now_ms) {
  if (self_index_ < 0) {
    return;
  }
  NodeEntry& entry = entries_[static_cast<size_t>(self_index_)];
  entry.pos_valid = true;
  entry.lat_e7 = lat_e7;
  entry.lon_e7 = lon_e7;
  entry.pos_age_s = pos_age_s;
  entry.last_seen_ms = now_ms;
  set_dirty();
}

void NodeTable::touch_self(uint32_t now_ms) {
  if (self_index_ < 0) {
    return;
  }
  entries_[static_cast<size_t>(self_index_)].last_seen_ms = now_ms;
  set_dirty();
}

bool NodeTable::set_self_node_name(const char* name) {
  if (self_index_ < 0 || !name) {
    return false;
  }
  NodeEntry& entry = entries_[static_cast<size_t>(self_index_)];
  // S04 #466: Cap at display label size so we never write past node_name[24]; null at copy_len (0..24).
  const size_t len = std::min(
      static_cast<size_t>(strnlen(name, kNodeTableNodeNameDisplayMaxBytes + 1)),
      kNodeTableNodeNameDisplayMaxBytes);
  std::memcpy(entry.node_name, name, len);
  entry.node_name[len] = '\0';
  set_dirty();
  return true;
}

// ── RX/apply semantics (#421: canon rx_semantics_v0; #438: v0.2-only, no Tail) ─
//
// upsert_remote (Alive path) and apply_pos_full / apply_status implement
// accepted vs duplicate vs out-of-order per rx_semantics_v0 §1 (seq16 wrap rule).
// last_core_seq16/has_core_seq16 set by apply_pos_full only (runtime-local, not BLE/persisted).

bool NodeTable::upsert_remote(uint64_t node_id,
                              bool pos_valid,
                              int32_t lat_e7,
                              int32_t lon_e7,
                              uint16_t pos_age_s,
                              int8_t last_rx_rssi,
                              uint16_t last_seq,
                              uint32_t now_ms) {
  const int existing = find_entry_index(node_id);
  if (existing >= 0) {
    NodeEntry& entry = entries_[static_cast<size_t>(existing)];
    if (!entry.is_self) {
      const Seq16Order order = seq16_order(last_seq, entry.last_seq);
      switch (order) {
        case Seq16Order::Newer:
          /* Accepted packet: update lastRxAt + link metrics; position only when pos_valid (v0.2: Alive does not overwrite position). */
          entry.last_rx_rssi = last_rx_rssi;
          entry.last_seq = last_seq;
          entry.last_seen_ms = now_ms;
          if (pos_valid) {
            entry.pos_valid = true;
            entry.lat_e7 = lat_e7;
            entry.lon_e7 = lon_e7;
            entry.pos_age_s = pos_age_s;
            entry.last_core_seq16 = last_seq;
            entry.has_core_seq16  = true;
          }
          /* else: Alive path — do not overwrite position (packet_truth_table_v02). */
          break;
        case Seq16Order::Same:
          /* Duplicate: refresh lastRxAt and link metrics only; MUST NOT overwrite position/telemetry. */
          entry.last_seen_ms = now_ms;
          entry.last_rx_rssi = last_rx_rssi;
          break;
        case Seq16Order::Older:
          /* Out-of-order: do not overwrite position/telemetry; update lastRxAt + link metrics only. */
          entry.last_seen_ms = now_ms;
          entry.last_rx_rssi = last_rx_rssi;
          break;
      }
    }
    set_dirty();
    return true;
  }

  int index = find_free_index();
  if (index < 0) {
    if (!evict_oldest_grey(now_ms)) {
      return false;
    }
    index = find_free_index();
    if (index < 0) {
      return false;
    }
  }

  NodeEntry& entry = entries_[static_cast<size_t>(index)];
  entry = NodeEntry{};
  entry.node_id = node_id;
  entry.short_id = compute_short_id(node_id);
  entry.is_self = false;
  entry.pos_valid = pos_valid;
  entry.lat_e7 = pos_valid ? lat_e7 : 0;
  entry.lon_e7 = pos_valid ? lon_e7 : 0;
  entry.pos_age_s = pos_valid ? pos_age_s : 0;
  entry.last_rx_rssi = last_rx_rssi;
  entry.last_seq = last_seq;
  entry.last_seen_ms = now_ms;
  entry.in_use = true;
  // Track last Core seq16 for Tail-1 match check.
  if (pos_valid) {
    entry.last_core_seq16 = last_seq;
    entry.has_core_seq16  = true;
  }
  size_++;
  recompute_collisions();
  set_dirty();
  return true;
}

// apply_pos_full: v0.2 Node_Pos_Full (#435). Single-packet apply; last_core_seq16 := seq16.
// Pos_Quality → NodeTable: canonical fields fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small
// land in existing storage per nodetable_master_field_table_v0 §2 (pos_fix_type/pos_accuracy_bucket
// stubs; pos_sats→sats, pos_flags_small→pos_flags). Packing: pos_flags = [0:3] pos_flags_small,
// [4:6] fix_type, [7] pos_accuracy_bucket bit0; sats = [0:5] pos_sats, [6:7] pos_accuracy_bucket bits 1–2.
bool NodeTable::apply_pos_full(uint64_t node_id,
                               uint16_t seq16,
                               int32_t lat_e7, int32_t lon_e7,
                               uint8_t fix_type, uint8_t pos_sats,
                               uint8_t pos_accuracy_bucket, uint8_t pos_flags_small,
                               int8_t rssi_dbm,
                               uint32_t now_ms) {
  int idx = find_entry_index(node_id);
  if (idx < 0) {
    int free_idx = find_free_index();
    if (free_idx < 0) {
      if (!evict_oldest_grey(now_ms)) {
        return false;
      }
      free_idx = find_free_index();
      if (free_idx < 0) {
        return false;
      }
    }
    NodeEntry& new_entry = entries_[static_cast<size_t>(free_idx)];
    new_entry = NodeEntry{};
    new_entry.node_id = node_id;
    new_entry.short_id = compute_short_id(node_id);
    new_entry.is_self = false;
    new_entry.in_use = true;
    new_entry.last_seen_ms = now_ms;
    new_entry.last_rx_rssi = rssi_dbm;
    size_++;
    recompute_collisions();
    set_dirty();
    idx = free_idx;
  }

  NodeEntry& entry = entries_[static_cast<size_t>(idx)];
  const Seq16Order order = seq16_order(seq16, entry.last_seq);
  if (order == Seq16Order::Same || order == Seq16Order::Older) {
    entry.last_seen_ms = now_ms;
    entry.last_rx_rssi = rssi_dbm;
    set_dirty();
    return true;
  }

  entry.pos_valid = true;
  entry.lat_e7 = lat_e7;
  entry.lon_e7 = lon_e7;
  entry.pos_age_s = 0;
  entry.last_seq = seq16;
  entry.last_core_seq16 = seq16;
  entry.has_core_seq16 = true;
  entry.has_pos_flags = true;
  entry.has_sats = true;
  entry.pos_flags = (pos_flags_small & 0x0Fu) | ((fix_type & 0x07u) << 4) | ((pos_accuracy_bucket & 0x01u) << 7);
  entry.sats = (pos_sats & 0x3Fu) | ((pos_accuracy_bucket & 0x06u) << 5);
  entry.last_seen_ms = now_ms;
  entry.last_rx_rssi = rssi_dbm;
  set_dirty();
  return true;
}

// apply_status: v0.2 Node_Status (#435). Full snapshot; does not update position.
bool NodeTable::apply_status(uint64_t node_id,
                             uint16_t seq16,
                             uint8_t battery_percent, uint8_t battery_est_rem_time,
                             uint8_t tx_power_ch_throttle, uint8_t uptime10m,
                             uint8_t role_id, uint8_t max_silence_10s,
                             uint16_t hw_profile_id, uint16_t fw_version_id,
                             int8_t rssi_dbm,
                             uint32_t now_ms) {
  (void)tx_power_ch_throttle;
  (void)role_id;
  (void)battery_est_rem_time;
  int idx = find_entry_index(node_id);
  if (idx < 0) {
    int free_idx = find_free_index();
    if (free_idx < 0) {
      if (!evict_oldest_grey(now_ms)) {
        return false;
      }
      free_idx = find_free_index();
      if (free_idx < 0) {
        return false;
      }
    }
    NodeEntry& new_entry = entries_[static_cast<size_t>(free_idx)];
    new_entry = NodeEntry{};
    new_entry.node_id = node_id;
    new_entry.short_id = compute_short_id(node_id);
    new_entry.is_self = false;
    new_entry.in_use = true;
    new_entry.last_seen_ms = now_ms;
    new_entry.last_rx_rssi = rssi_dbm;
    size_++;
    recompute_collisions();
    set_dirty();
    idx = free_idx;
  }

  NodeEntry& entry = entries_[static_cast<size_t>(idx)];
  const Seq16Order order = seq16_order(seq16, entry.last_seq);
  if (order == Seq16Order::Same || order == Seq16Order::Older) {
    entry.last_seen_ms = now_ms;
    entry.last_rx_rssi = rssi_dbm;
    set_dirty();
    return true;
  }

  entry.last_seq = seq16;
  entry.has_battery = true;
  entry.battery_percent = battery_percent;
  entry.has_uptime = true;
  entry.uptime_sec = static_cast<uint32_t>(uptime10m) * 600u;
  entry.has_max_silence = true;
  entry.max_silence_10s = max_silence_10s;
  entry.has_hw_profile = true;
  entry.hw_profile_id = hw_profile_id;
  entry.has_fw_version = true;
  entry.fw_version_id = fw_version_id;
  entry.last_seen_ms = now_ms;
  entry.last_rx_rssi = rssi_dbm;
  set_dirty();
  return true;
}

size_t NodeTable::size() const {
  return size_;
}

size_t NodeTable::get_page(uint32_t now_ms,
                           size_t page_index,
                           size_t page_size,
                           uint8_t* out_buffer,
                           size_t out_capacity) const {
  if (!out_buffer || out_capacity < kRecordBytes || page_size == 0) {
    return 0;
  }

  std::array<size_t, kMaxNodes> indices{};
  const size_t total = build_ordered_indices(indices);
  const size_t start = page_index * page_size;
  if (start >= total) {
    return 0;
  }

  size_t remaining = total - start;
  size_t entries_to_write = remaining < page_size ? remaining : page_size;
  const size_t max_by_capacity = out_capacity / kRecordBytes;
  if (entries_to_write > max_by_capacity) {
    entries_to_write = max_by_capacity;
  }
  if (entries_to_write == 0) {
    return 0;
  }

  size_t offset = 0;
  for (size_t i = 0; i < entries_to_write; ++i) {
    const NodeEntry& entry = entries_[indices[start + i]];
    const bool grey = is_grey(entry, now_ms);
    const uint32_t age_ms = now_ms >= entry.last_seen_ms ? (now_ms - entry.last_seen_ms) : 0;
    const uint16_t age_s = clamp_u16(age_ms / 1000);
    uint8_t flags = 0;
    if (entry.is_self) {
      flags |= 0x01;
    }
    if (entry.pos_valid) {
      flags |= 0x02;
    }
    if (grey) {
      flags |= 0x04;
    }
    if (entry.short_id_collision) {
      flags |= 0x08;
    }

    write_u64_le(out_buffer + offset, entry.node_id);
    write_u16_le(out_buffer + offset + 8, entry.short_id);
    out_buffer[offset + 10] = flags;
    write_u16_le(out_buffer + offset + 11, age_s);
    write_u32_le(out_buffer + offset + 13, static_cast<uint32_t>(entry.pos_valid ? entry.lat_e7 : 0));
    write_u32_le(out_buffer + offset + 17, static_cast<uint32_t>(entry.pos_valid ? entry.lon_e7 : 0));
    write_u16_le(out_buffer + offset + 21, entry.pos_valid ? entry.pos_age_s : 0);
    out_buffer[offset + 23] = static_cast<uint8_t>(entry.last_rx_rssi);
    // #419: last_seq not in BLE per canon; offset 24 = snr_last (127 = NA), 25 = reserved.
    out_buffer[offset + 24] = static_cast<uint8_t>(entry.snr_last);
    out_buffer[offset + 25] = 0;

    offset += kRecordBytes;
  }

  return offset;
}

size_t NodeTable::get_peer_dump_line(uint32_t now_ms, size_t peer_index, char* buf, size_t cap) const {
  if (!buf || cap == 0) {
    return 0;
  }
  std::array<size_t, kMaxNodes> indices{};
  const size_t total = build_ordered_indices(indices);
  size_t peer_count = 0;
  size_t first_peer_offset = 0;
  if (self_index_ >= 0 && total > 0) {
    first_peer_offset = 1;
    peer_count = total - 1;
  } else {
    peer_count = total;
  }
  if (peer_index >= peer_count) {
    return 0;
  }
  const NodeEntry& entry = entries_[indices[first_peer_offset + peer_index]];
  const bool grey = is_grey(entry, now_ms);
  const uint32_t age_ms = now_ms >= entry.last_seen_ms ? (now_ms - entry.last_seen_ms) : 0;
  const uint16_t age_s = clamp_u16(age_ms / 1000);
  const int len = std::snprintf(buf, cap,
                                "peer shortId=%u ageS=%u grey=%u seq=%u rssi=%d posAgeS=%u",
                                static_cast<unsigned>(entry.short_id),
                                static_cast<unsigned>(age_s),
                                grey ? 1u : 0u,
                                static_cast<unsigned>(entry.last_seq),
                                static_cast<int>(entry.last_rx_rssi),
                                static_cast<unsigned>(entry.pos_valid ? entry.pos_age_s : 0));
  return len > 0 && static_cast<size_t>(len) < cap ? static_cast<size_t>(len) : 0;
}

uint16_t NodeTable::create_snapshot(uint32_t now_ms) {
  snapshot_time_ms_ = now_ms;
  snapshot_id_ = static_cast<uint16_t>(snapshot_id_ + 1u);
  snapshot_count_ = build_ordered_indices(snapshot_indices_);
  return snapshot_id_;
}

size_t NodeTable::get_snapshot_page(uint16_t snapshot_id,
                                    size_t page_index,
                                    size_t page_size,
                                    uint8_t* out_buffer,
                                    size_t out_capacity) const {
  if (snapshot_id != snapshot_id_) {
    return 0;
  }
  if (!out_buffer || out_capacity < kRecordBytes || page_size == 0) {
    return 0;
  }

  const size_t start = page_index * page_size;
  if (start >= snapshot_count_) {
    return 0;
  }

  size_t remaining = snapshot_count_ - start;
  size_t entries_to_write = remaining < page_size ? remaining : page_size;
  const size_t max_by_capacity = out_capacity / kRecordBytes;
  if (entries_to_write > max_by_capacity) {
    entries_to_write = max_by_capacity;
  }
  if (entries_to_write == 0) {
    return 0;
  }

  size_t offset = 0;
  for (size_t i = 0; i < entries_to_write; ++i) {
    const NodeEntry& entry = entries_[snapshot_indices_[start + i]];
    const bool grey = is_grey(entry, snapshot_time_ms_);
    const uint32_t age_ms =
        snapshot_time_ms_ >= entry.last_seen_ms ? (snapshot_time_ms_ - entry.last_seen_ms) : 0;
    const uint16_t age_s = clamp_u16(age_ms / 1000);
    uint8_t flags = 0;
    if (entry.is_self) {
      flags |= 0x01;
    }
    if (entry.pos_valid) {
      flags |= 0x02;
    }
    if (grey) {
      flags |= 0x04;
    }
    if (entry.short_id_collision) {
      flags |= 0x08;
    }

    write_u64_le(out_buffer + offset, entry.node_id);
    write_u16_le(out_buffer + offset + 8, entry.short_id);
    out_buffer[offset + 10] = flags;
    write_u16_le(out_buffer + offset + 11, age_s);
    write_u32_le(out_buffer + offset + 13, static_cast<uint32_t>(entry.pos_valid ? entry.lat_e7 : 0));
    write_u32_le(out_buffer + offset + 17, static_cast<uint32_t>(entry.pos_valid ? entry.lon_e7 : 0));
    write_u16_le(out_buffer + offset + 21, entry.pos_valid ? entry.pos_age_s : 0);
    out_buffer[offset + 23] = static_cast<uint8_t>(entry.last_rx_rssi);
    out_buffer[offset + 24] = static_cast<uint8_t>(entry.snr_last);
    out_buffer[offset + 25] = 0;

    offset += kRecordBytes;
  }

  return offset;
}

uint32_t NodeTable::get_snapshot_time_ms(uint16_t snapshot_id) const {
  if (snapshot_id != snapshot_id_) {
    return 0;
  }
  return snapshot_time_ms_;
}

size_t NodeTable::get_snapshot_page_entries(uint16_t snapshot_id,
                                           size_t page_index,
                                           size_t page_size,
                                           NodeEntry* out,
                                           size_t max_count) const {
  if (snapshot_id != snapshot_id_ || !out || max_count == 0 || page_size == 0) {
    return 0;
  }
  const size_t start = page_index * page_size;
  if (start >= snapshot_count_) {
    return 0;
  }
  size_t remaining = snapshot_count_ - start;
  size_t n = remaining < page_size ? remaining : page_size;
  if (n > max_count) {
    n = max_count;
  }
  for (size_t i = 0; i < n; ++i) {
    out[i] = entries_[snapshot_indices_[start + i]];
  }
  return n;
}

bool NodeTable::find_entry_by_node_id(uint64_t node_id, NodeEntry* out) const {
  if (!out) {
    return false;
  }
  const int idx = find_entry_index(node_id);
  if (idx < 0) {
    return false;
  }
  *out = entries_[static_cast<size_t>(idx)];
  return true;
}

void NodeTable::for_each_used_entry(std::function<void(const NodeEntry&)> fn) const {
  for (size_t i = 0; i < entries_.size(); ++i) {
    if (entries_[i].in_use) {
      fn(entries_[i]);
    }
  }
}

void NodeTable::restore_from_entries(const NodeEntry* entries, size_t count) {
  if (!entries || count > kMaxNodes) {
    return;
  }
  for (size_t i = 0; i < entries_.size(); ++i) {
    entries_[i].in_use = false;
  }
  size_ = 0;
  self_index_ = -1;
  for (size_t c = 0; c < count; ++c) {
    const int idx = find_free_index();
    if (idx < 0) {
      break;
    }
    entries_[static_cast<size_t>(idx)] = entries[c];
    entries_[static_cast<size_t>(idx)].in_use = true;
    if (entries[c].is_self) {
      self_index_ = idx;
    }
    size_++;
  }
  recompute_collisions();
  // Do not set dirty; restore is load, not user mutation.
}

#if defined(NAVIGA_TEST)
bool NodeTable::find_entry_for_test(uint64_t node_id, NodeEntry* out) const {
  const int idx = find_entry_index(node_id);
  if (idx < 0 || !out) {
    return false;
  }
  *out = entries_[static_cast<size_t>(idx)];
  return true;
}
#endif

int NodeTable::find_entry_index(uint64_t node_id) const {
  for (size_t i = 0; i < entries_.size(); ++i) {
    if (entries_[i].in_use && entries_[i].node_id == node_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int NodeTable::find_free_index() const {
  for (size_t i = 0; i < entries_.size(); ++i) {
    if (!entries_[i].in_use) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool NodeTable::evict_oldest_grey(uint32_t now_ms) {
  uint32_t oldest = UINT32_MAX;
  int oldest_index = -1;
  for (size_t i = 0; i < entries_.size(); ++i) {
    const NodeEntry& entry = entries_[i];
    if (!entry.in_use || entry.is_self) {
      continue;
    }
    if (!is_grey(entry, now_ms)) {
      continue;
    }
    if (entry.last_seen_ms < oldest) {
      oldest = entry.last_seen_ms;
      oldest_index = static_cast<int>(i);
    }
  }

  if (oldest_index < 0) {
    return false;
  }
  entries_[static_cast<size_t>(oldest_index)] = NodeEntry{};
  size_--;
  recompute_collisions();
  return true;
}

void NodeTable::recompute_collisions() {
  for (auto& entry : entries_) {
    if (entry.in_use) {
      entry.short_id_collision = false;
    }
  }

  for (size_t i = 0; i < entries_.size(); ++i) {
    if (!entries_[i].in_use) {
      continue;
    }
    for (size_t j = i + 1; j < entries_.size(); ++j) {
      if (!entries_[j].in_use) {
        continue;
      }
      if (entries_[i].short_id == entries_[j].short_id) {
        entries_[i].short_id_collision = true;
        entries_[j].short_id_collision = true;
      }
    }
  }
}

bool NodeTable::is_grey(const NodeEntry& entry, uint32_t now_ms) const {
  if (expected_interval_s_ == 0) {
    return false;
  }
  const uint32_t age_ms = now_ms >= entry.last_seen_ms ? (now_ms - entry.last_seen_ms) : 0;
  const uint32_t age_s = age_ms / 1000;
  const uint32_t threshold_s =
      static_cast<uint32_t>(expected_interval_s_) + static_cast<uint32_t>(grace_s_);
  return age_s > threshold_s;
}

uint16_t NodeTable::compute_grace_s(uint16_t expected_interval_s) {
  const uint32_t rounded = (static_cast<uint32_t>(expected_interval_s) * 25u + 50u) / 100u;
  const uint16_t grace = static_cast<uint16_t>(rounded);
  return grace < 2 ? 2 : grace;
}

size_t NodeTable::build_ordered_indices(std::array<size_t, kMaxNodes>& out_indices) const {
  size_t count = 0;
  if (self_index_ >= 0 && entries_[static_cast<size_t>(self_index_)].in_use) {
    out_indices[count++] = static_cast<size_t>(self_index_);
  }

  for (size_t i = 0; i < entries_.size(); ++i) {
    if (!entries_[i].in_use) {
      continue;
    }
    if (static_cast<int>(i) == self_index_) {
      continue;
    }
    out_indices[count++] = i;
  }

  if (count <= 1) {
    return count;
  }

  auto compare = [&](size_t a, size_t b) {
    return entries_[a].node_id < entries_[b].node_id;
  };
  if (self_index_ >= 0) {
    std::sort(out_indices.begin() + 1, out_indices.begin() + static_cast<long>(count), compare);
  } else {
    std::sort(out_indices.begin(), out_indices.begin() + static_cast<long>(count), compare);
  }
  return count;
}

} // namespace domain
} // namespace naviga
