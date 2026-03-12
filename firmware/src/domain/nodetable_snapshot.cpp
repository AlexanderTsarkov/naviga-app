#include "domain/nodetable_snapshot.h"
#include "domain/node_table.h"

#include <cstring>

namespace naviga {
namespace domain {

namespace {

constexpr uint8_t kSnapshotMagic0 = 'N';
constexpr uint8_t kSnapshotMagic1 = 'T';
/** Snapshot format version. v3 = 35-byte (#418); v4 = 68-byte (+ node_name, #419). */
constexpr uint8_t kSnapshotVersion = 4;
constexpr uint8_t kSnapshotVersionV3 = 3;

void put_u16_le(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v & 0xFF);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}
void put_u32_le(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v & 0xFF);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
  p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
  p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}
void put_u64_le(uint8_t* p, uint64_t v) {
  for (int i = 0; i < 8; ++i) {
    p[i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFF);
  }
}
uint16_t get_u16_le(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}
uint32_t get_u32_le(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
uint64_t get_u64_le(const uint8_t* p) {
  uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v |= static_cast<uint64_t>(p[i]) << (8 * i);
  }
  return v;
}

/** Persisted subset (#418 v3 + #419 node_name). Does NOT persist: last_seen_ms, is_self, short_id_collision, last_rx_rssi, last_seq, snr_last, legacy ref* (canon §7). */
size_t pack_record(const NodeEntry& e, uint8_t* out) {
  put_u64_le(out + 0, e.node_id);
  put_u16_le(out + 8, e.short_id);
  out[10] = e.pos_valid ? 0x01u : 0x00u;
  put_u32_le(out + 11, static_cast<uint32_t>(e.pos_valid ? e.lat_e7 : 0));
  put_u32_le(out + 15, static_cast<uint32_t>(e.pos_valid ? e.lon_e7 : 0));
  put_u16_le(out + 19, e.pos_valid ? e.pos_age_s : 0);
  uint8_t flags2 = 0;
  if (e.has_pos_flags) flags2 |= 0x01;
  if (e.has_sats) flags2 |= 0x02;
  out[21] = flags2;
  out[22] = e.pos_flags;
  out[23] = e.sats;
  uint8_t flags3 = 0;
  if (e.has_battery) flags3 |= 0x01;
  if (e.has_uptime) flags3 |= 0x02;
  if (e.has_max_silence) flags3 |= 0x04;
  if (e.has_hw_profile) flags3 |= 0x08;
  if (e.has_fw_version) flags3 |= 0x10;
  out[24] = flags3;
  out[25] = e.battery_percent;
  put_u32_le(out + 26, e.uptime_sec);
  out[30] = e.max_silence_10s;
  put_u16_le(out + 31, e.hw_profile_id);
  put_u16_le(out + 33, e.fw_version_id);
  // #419: node_name (1 byte length + 32 bytes; canonical single name field).
  const size_t name_len = std::min(strnlen(e.node_name, kNodeTableNodeNameMaxLen), kNodeTableNodeNameMaxLen);
  out[35] = static_cast<uint8_t>(name_len);
  std::memcpy(out + 36, e.node_name, name_len);
  if (name_len < kNodeTableNodeNameMaxLen) {
    std::memset(out + 36 + name_len, 0, kNodeTableNodeNameMaxLen - name_len);
  }
  return kNodeTableSnapshotRecordBytes;
}

/** Unpack to NodeEntry (v3: 35 bytes, or v4: 68 bytes with node_name). Sets derived/runtime/legacy to 0/false; caller sets is_self. */
void unpack_record(const uint8_t* in, NodeEntry* e, size_t record_bytes) {
  e->node_id = get_u64_le(in + 0);
  e->short_id = get_u16_le(in + 8);
  e->pos_valid = (in[10] & 0x01) != 0;
  e->lat_e7 = static_cast<int32_t>(get_u32_le(in + 11));
  e->lon_e7 = static_cast<int32_t>(get_u32_le(in + 15));
  e->pos_age_s = get_u16_le(in + 19);
  const uint8_t flags2 = in[21];
  e->has_pos_flags = (flags2 & 0x01) != 0;
  e->has_sats = (flags2 & 0x02) != 0;
  e->pos_flags = in[22];
  e->sats = in[23];
  const uint8_t flags3 = in[24];
  e->has_battery = (flags3 & 0x01) != 0;
  e->has_uptime = (flags3 & 0x02) != 0;
  e->has_max_silence = (flags3 & 0x04) != 0;
  e->has_hw_profile = (flags3 & 0x08) != 0;
  e->has_fw_version = (flags3 & 0x10) != 0;
  e->battery_percent = in[25];
  e->uptime_sec = get_u32_le(in + 26);
  e->max_silence_10s = in[30];
  e->hw_profile_id = get_u16_le(in + 31);
  e->fw_version_id = get_u16_le(in + 33);
  if (record_bytes >= kNodeTableSnapshotRecordBytes) {
    const size_t name_len = std::min<size_t>(in[35], kNodeTableNodeNameMaxLen);
    std::memcpy(e->node_name, in + 36, name_len);
    e->node_name[name_len] = '\0';
  } else {
    e->node_name[0] = '\0';
  }
  e->last_seen_ms = 0;
  e->is_self = false;
  e->short_id_collision = false;
  e->last_rx_rssi = 0;
  e->snr_last = kSnrLastNa;
  e->last_seq = 0;
  e->last_core_seq16 = 0;
  e->has_core_seq16 = false;
  e->in_use = true;
}

}  // namespace

size_t build_nodetable_snapshot(const NodeTable& table,
                                uint8_t* out,
                                size_t out_cap) {
  constexpr size_t kHeaderBytes = 5;
  if (!out || out_cap < kHeaderBytes + kNodeTableSnapshotRecordBytes) {
    return 0;
  }
  out[0] = kSnapshotMagic0;
  out[1] = kSnapshotMagic1;
  out[2] = kSnapshotVersion;
  size_t count = 0;
  size_t offset = kHeaderBytes;
  table.for_each_used_entry([&](const NodeEntry& e) {
    if (count < NodeTable::kMaxNodes && offset + kNodeTableSnapshotRecordBytes <= out_cap) {
      pack_record(e, out + offset);
      offset += kNodeTableSnapshotRecordBytes;
      count++;
    }
  });
  put_u16_le(out + 3, static_cast<uint16_t>(count));
  return offset;
}

size_t restore_from_nodetable_snapshot(const uint8_t* data,
                                       size_t len,
                                       uint64_t self_node_id,
                                       NodeEntry* out_entries,
                                       size_t max_entries) {
  constexpr size_t kHeaderBytes = 5;
  if (!data || len < kHeaderBytes || !out_entries || max_entries == 0) {
    return 0;
  }
  if (data[0] != kSnapshotMagic0 || data[1] != kSnapshotMagic1) {
    return 0;
  }
  const uint8_t version = data[2];
  size_t record_bytes = 0;
  if (version == kSnapshotVersionV3) {
    record_bytes = kNodeTableSnapshotRecordBytesV3;
  } else if (version == kSnapshotVersion) {
    record_bytes = kNodeTableSnapshotRecordBytes;
  } else {
    return 0;
  }
  const uint16_t count = get_u16_le(data + 3);
  if (static_cast<size_t>(count) * record_bytes + kHeaderBytes > len) {
    return 0;
  }
  size_t n = 0;
  for (uint16_t i = 0; i < count && n < max_entries; ++i) {
    const uint8_t* rec = data + kHeaderBytes + i * record_bytes;
    unpack_record(rec, out_entries + n, record_bytes);
    out_entries[n].is_self = (out_entries[n].node_id == self_node_id);
    n++;
  }
  return n;
}

}  // namespace domain
}  // namespace naviga
