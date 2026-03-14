#include "ble_node_table_bridge.h"

#include "platform/ble_transport_core.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace naviga {
namespace protocol {

namespace {

constexpr size_t kMaxDeviceInfoLen = 256;
constexpr size_t kPageHeaderBytes = 10;

/** Simple hash for change detection; not cryptographic. */
uint32_t entry_hash(const domain::NodeEntry& e) {
  uint32_t h = static_cast<uint32_t>(e.node_id) ^ static_cast<uint32_t>(e.node_id >> 32);
  h ^= static_cast<uint32_t>(e.short_id) << 8;
  h ^= static_cast<uint32_t>(e.lat_e7);
  h ^= static_cast<uint32_t>(e.lon_e7) * 31u;
  h ^= e.pos_age_s * 7u;
  h ^= e.last_seen_ms;
  h ^= static_cast<uint8_t>(e.last_rx_rssi) << 16;
  return h;
}

void write_u16_le(uint8_t* out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void write_u32_le_at(uint8_t* out, size_t offset, uint32_t value) {
  out[offset + 0] = static_cast<uint8_t>(value & 0xFF);
  out[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  out[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  out[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void write_u64_le_at(uint8_t* out, size_t offset, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out[offset + i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
  }
}

/** S04 #464: Pack one NodeEntry to canon BLE record (72 bytes). Excludes last_seq, last_seen_ms, etc. */
size_t pack_ble_record(const domain::NodeEntry& e,
                       uint32_t snapshot_time_ms,
                       const domain::NodeTable& table,
                       uint8_t* out) {
  if (!out) {
    return 0;
  }
  const uint32_t age_ms =
      snapshot_time_ms >= e.last_seen_ms ? (snapshot_time_ms - e.last_seen_ms) : 0;
  const uint32_t age_s32 = age_ms / 1000;
  const uint16_t age_s = age_s32 > 65535u ? 65535 : static_cast<uint16_t>(age_s32);
  const bool is_stale = table.is_stale(e, snapshot_time_ms);

  write_u64_le_at(out, 0, e.node_id);
  write_u16_le(out + 8, e.short_id);
  uint8_t flags1 = 0;
  if (e.is_self) flags1 |= 0x01;
  if (e.short_id_collision) flags1 |= 0x02;
  if (e.pos_valid) flags1 |= 0x04;
  if (is_stale) flags1 |= 0x08;
  out[10] = flags1;
  write_u16_le(out + 11, age_s);
  write_u32_le_at(out, 13, static_cast<uint32_t>(e.pos_valid ? e.lat_e7 : 0));
  write_u32_le_at(out, 17, static_cast<uint32_t>(e.pos_valid ? e.lon_e7 : 0));
  write_u16_le(out + 21, e.pos_valid ? e.pos_age_s : 0);
  uint8_t flags2 = 0;
  if (e.has_pos_flags) flags2 |= 0x01;
  if (e.has_sats) flags2 |= 0x02;
  out[23] = flags2;
  out[24] = e.pos_flags;
  out[25] = e.sats;
  uint8_t flags3 = 0;
  if (e.has_battery) flags3 |= 0x01;
  if (e.has_uptime) flags3 |= 0x02;
  if (e.has_max_silence) flags3 |= 0x04;
  if (e.has_hw_profile) flags3 |= 0x08;
  if (e.has_fw_version) flags3 |= 0x10;
  out[26] = flags3;
  out[27] = e.battery_percent;
  write_u32_le_at(out, 28, e.uptime_sec);
  out[32] = e.max_silence_10s;
  write_u16_le(out + 33, e.hw_profile_id);
  write_u16_le(out + 35, e.fw_version_id);
  out[37] = static_cast<uint8_t>(e.last_rx_rssi);
  out[38] = static_cast<uint8_t>(e.snr_last);
  const size_t name_len =
      std::min(strnlen(e.node_name, domain::kNodeTableNodeNameMaxLen), domain::kNodeTableNodeNameMaxLen);
  out[39] = static_cast<uint8_t>(name_len);
  std::memcpy(out + 40, e.node_name, name_len);
  if (name_len < domain::kNodeTableNodeNameMaxLen) {
    std::memset(out + 40 + name_len, 0, domain::kNodeTableNodeNameMaxLen - name_len);
  }
  return BleNodeTableBridge::kRecordBytesBle;
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

void append_u8(uint8_t* buffer, size_t& offset, size_t capacity, uint8_t value) {
  if (offset < capacity) {
    buffer[offset++] = value;
  }
}

void append_u16_le(uint8_t* buffer, size_t& offset, size_t capacity, uint16_t value) {
  if (offset + 1 < capacity) {
    buffer[offset++] = static_cast<uint8_t>(value & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((value >> 8) & 0xFF);
  }
}

void append_u32_le(uint8_t* buffer, size_t& offset, size_t capacity, uint32_t value) {
  if (offset + 3 < capacity) {
    buffer[offset++] = static_cast<uint8_t>(value & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buffer[offset++] = static_cast<uint8_t>((value >> 24) & 0xFF);
  }
}

void append_u64_le(uint8_t* buffer, size_t& offset, size_t capacity, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    if (offset < capacity) {
      buffer[offset++] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
    }
  }
}

void append_str(uint8_t* buffer, size_t& offset, size_t capacity, const char* value) {
  if (!value) {
    append_u8(buffer, offset, capacity, 0);
    return;
  }
  const size_t len = std::min<size_t>(255, std::strlen(value));
  append_u8(buffer, offset, capacity, static_cast<uint8_t>(len));
  const size_t copy_len = std::min(len, capacity - offset);
  if (copy_len > 0) {
    std::memcpy(buffer + offset, value, copy_len);
    offset += copy_len;
  }
}

} // namespace

bool BleNodeTableBridge::update_device_info(const DeviceInfoModel& model,
                                            IBleTransport& transport) const {
  std::array<uint8_t, kMaxDeviceInfoLen> buffer{};
  size_t offset = 0;

  append_u8(buffer.data(), offset, buffer.size(), model.format_ver);
  append_u8(buffer.data(), offset, buffer.size(), model.ble_schema_ver);
  if (model.include_radio_proto_ver) {
    append_u8(buffer.data(), offset, buffer.size(), model.radio_proto_ver);
  }
  append_u64_le(buffer.data(), offset, buffer.size(), model.node_id);
  append_u16_le(buffer.data(), offset, buffer.size(), model.short_id);
  append_u8(buffer.data(), offset, buffer.size(), model.device_type);
  append_str(buffer.data(), offset, buffer.size(), model.firmware_version);
  append_str(buffer.data(), offset, buffer.size(), model.radio_module_model);
  append_u16_le(buffer.data(), offset, buffer.size(), model.band_id);
  append_u16_le(buffer.data(), offset, buffer.size(), model.power_min);
  append_u16_le(buffer.data(), offset, buffer.size(), model.power_max);
  append_u16_le(buffer.data(), offset, buffer.size(), model.channel_min);
  append_u16_le(buffer.data(), offset, buffer.size(), model.channel_max);
  append_u8(buffer.data(), offset, buffer.size(), model.network_mode);
  append_u16_le(buffer.data(), offset, buffer.size(), model.channel_id);
  append_u16_le(buffer.data(), offset, buffer.size(), model.public_channel_id);
  append_u16_le(buffer.data(), offset, buffer.size(), model.private_channel_min);
  append_u16_le(buffer.data(), offset, buffer.size(), model.private_channel_max);
  append_u32_le(buffer.data(), offset, buffer.size(), model.capabilities);

  transport.set_device_info(buffer.data(), offset);
  return offset > 0;
}

bool BleNodeTableBridge::update_node_table(uint32_t now_ms,
                                           domain::NodeTable& table,
                                           IBleTransport& transport) const {
  uint16_t req_snapshot_id = 0;
  uint16_t req_page_index = 0;
  if (!transport.get_node_table_request(&req_snapshot_id, &req_page_index)) {
    req_snapshot_id = 0;
    req_page_index = 0;
  }

  uint16_t snapshot_id = req_snapshot_id;
  size_t total_nodes = table.size();
  size_t page_count = total_nodes == 0 ? 0 : (total_nodes + kPageSize - 1) / kPageSize;
  size_t page_index = req_page_index;

  bool snapshot_reset = false;
  if (snapshot_id == 0) {
    snapshot_id = table.create_snapshot(now_ms);
    total_nodes = table.size();
    page_count = total_nodes == 0 ? 0 : (total_nodes + kPageSize - 1) / kPageSize;
    page_index = 0;
    snapshot_reset = true;
  }

  std::array<domain::NodeEntry, kPageSize> entries{};
  size_t n_entries = 0;
  if (total_nodes > 0 && page_index < page_count) {
    n_entries = table.get_snapshot_page_entries(
        snapshot_id, page_index, kPageSize, entries.data(), entries.size());
    if (n_entries == 0 && !snapshot_reset) {
      snapshot_id = table.create_snapshot(now_ms);
      total_nodes = table.size();
      page_count = total_nodes == 0 ? 0 : (total_nodes + kPageSize - 1) / kPageSize;
      page_index = 0;
      n_entries = table.get_snapshot_page_entries(
          snapshot_id, 0, kPageSize, entries.data(), entries.size());
    }
  }

  const uint32_t snapshot_time_ms = table.get_snapshot_time_ms(snapshot_id);

  std::array<uint8_t, kPageHeaderBytes + kRecordBytesBle * kPageSize> buffer{};
  write_u16_le(buffer.data(), snapshot_id);
  write_u16_le(buffer.data() + 2, static_cast<uint16_t>(total_nodes));
  write_u16_le(buffer.data() + 4, static_cast<uint16_t>(page_index));
  buffer[6] = kPageSize;
  write_u16_le(buffer.data() + 7, static_cast<uint16_t>(page_count));
  buffer[9] = kRecordFormatVer;

  size_t record_bytes = 0;
  for (size_t i = 0; i < n_entries; ++i) {
    pack_ble_record(entries[i], snapshot_time_ms, table,
                   buffer.data() + kPageHeaderBytes + i * kRecordBytesBle);
    record_bytes += kRecordBytesBle;
  }

  const size_t payload_len = kPageHeaderBytes + record_bytes;
  transport.set_node_table_response(buffer.data(), payload_len);
  return true;
}

void BleNodeTableBridge::update_targeted_read(uint32_t now_ms,
                                              domain::NodeTable& table,
                                              IBleTransport& transport) const {
  uint64_t node_id = 0;
  if (!transport.get_targeted_read_request(&node_id)) {
    transport.set_targeted_read_response(nullptr, 0);
    return;
  }
  domain::NodeEntry entry{};
  if (!table.find_entry_by_node_id(node_id, &entry)) {
    transport.set_targeted_read_response(nullptr, 0);
    return;
  }
  std::array<uint8_t, kRecordBytesBle> buf{};
  pack_ble_record(entry, now_ms, table, buf.data());
  transport.set_targeted_read_response(buf.data(), kRecordBytesBle);
}

bool BleNodeTableBridge::update_all(uint32_t now_ms,
                                    const DeviceInfoModel& model,
                                    domain::NodeTable& table,
                                    IBleTransport& transport) const {
  const bool ok_info = update_device_info(model, transport);
  const bool ok_table = update_node_table(now_ms, table, transport);
  return ok_info && ok_table;
}

void BleNodeTableBridge::update_subscription_batch(uint32_t now_ms,
                                                   domain::NodeTable& table,
                                                   IBleTransport& transport) {
  const bool window_elapsed =
      (last_emit_ms_ != 0 && (now_ms - last_emit_ms_) >= kCoalescingWindowMs) ||
      (last_emit_ms_ == 0);

  if (window_elapsed) {
    if (pending_count_ > 0) {
      const uint32_t snapshot_time_ms = now_ms;
      std::array<uint8_t, BleTransportCore::kMaxSubscriptionBatchLen> buf{};
      buf[0] = static_cast<uint8_t>(pending_count_);
      size_t offset = 1;
      for (size_t i = 0; i < pending_count_ && offset + kRecordBytesBle <= buf.size(); ++i) {
        domain::NodeEntry entry{};
        if (!table.find_entry_by_node_id(pending_ids_[i], &entry)) {
          continue;
        }
        pack_ble_record(entry, snapshot_time_ms, table, buf.data() + offset);
        offset += kRecordBytesBle;
      }
      const size_t payload_len = 1 + pending_count_ * kRecordBytesBle;
      transport.set_subscription_update_payload(buf.data(), payload_len);
      transport.send_subscription_update();
    }
    prev_count_ = 0;
    table.for_each_used_entry([this](const domain::NodeEntry& e) {
      if (prev_count_ < prev_.size()) {
        prev_[prev_count_].node_id = e.node_id;
        prev_[prev_count_].hash = entry_hash(e);
        ++prev_count_;
      }
    });
    pending_count_ = 0;
    last_emit_ms_ = now_ms;
    return;
  }

  table.for_each_used_entry([this, &table, now_ms](const domain::NodeEntry& e) {
    if (pending_count_ >= kMaxBatchRecords) {
      return;
    }
    const uint32_t h = entry_hash(e);
    bool found_prev = false;
    bool changed = true;
    for (size_t i = 0; i < prev_count_; ++i) {
      if (prev_[i].node_id == e.node_id) {
        found_prev = true;
        changed = (prev_[i].hash != h);
        break;
      }
    }
    if (!changed) {
      return;
    }
    for (size_t i = 0; i < pending_count_; ++i) {
      if (pending_ids_[i] == e.node_id) {
        return;
      }
    }
    pending_ids_[pending_count_++] = e.node_id;
  });
}

} // namespace protocol
} // namespace naviga
