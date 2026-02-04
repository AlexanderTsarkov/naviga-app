#include "app/node_table.h"

namespace naviga {

namespace {

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

void serialize_entry_le(const NodeEntry& entry, uint8_t* out) {
  // Little-endian fixed-size layout (23 bytes):
  // full_id (u64) | short_id (u16) | lat_e7 (i32) | lon_e7 (i32)
  // | last_seen_ms (u32) | flags (u8)
  write_u64_le(out, entry.full_id);
  write_u16_le(out + 8, entry.short_id);
  write_u32_le(out + 10, static_cast<uint32_t>(entry.lat_e7));
  write_u32_le(out + 14, static_cast<uint32_t>(entry.lon_e7));
  write_u32_le(out + 18, entry.last_seen_ms);
  out[22] = entry.flags;
}

} // namespace

void NodeTable::init_self(uint64_t full_id, uint16_t short_id, uint32_t now_ms) {
  self_.full_id = full_id;
  self_.short_id = short_id;
  self_.lat_e7 = 0;
  self_.lon_e7 = 0;
  self_.last_seen_ms = now_ms;
  self_.flags = 0;
  has_self_ = true;
}

void NodeTable::update_self_position(int32_t lat_e7, int32_t lon_e7, uint32_t now_ms) {
  if (!has_self_) {
    return;
  }
  self_.lat_e7 = lat_e7;
  self_.lon_e7 = lon_e7;
  self_.last_seen_ms = now_ms;
}

size_t NodeTable::size() const {
  return has_self_ ? 1 : 0;
}

size_t NodeTable::get_page(size_t page_index,
                            size_t page_size,
                            uint8_t* out_buffer,
                            size_t out_capacity) const {
  if (!has_self_ || !out_buffer || out_capacity < kEntryBytes || page_size == 0) {
    return 0;
  }
  const size_t total = size();
  const size_t start = page_index * page_size;
  if (start >= total) {
    return 0;
  }

  size_t remaining = total - start;
  size_t entries_to_write = remaining < page_size ? remaining : page_size;
  const size_t max_entries_by_capacity = out_capacity / kEntryBytes;
  if (entries_to_write > max_entries_by_capacity) {
    entries_to_write = max_entries_by_capacity;
  }
  if (entries_to_write == 0) {
    return 0;
  }

  // Only self entry exists for now.
  if (start == 0) {
    serialize_entry_le(self_, out_buffer);
    return kEntryBytes;
  }

  return 0;
}

} // namespace naviga
