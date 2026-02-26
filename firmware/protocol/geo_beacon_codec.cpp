#include "geo_beacon_codec.h"

namespace naviga {
namespace protocol {

namespace {

constexpr int32_t kLatMin = -900000000;
constexpr int32_t kLatMax = 900000000;
constexpr int32_t kLonMin = -1800000000;
constexpr int32_t kLonMax = 1800000000;

void write_u16_le(uint8_t* dst, uint16_t value) {
  dst[0] = static_cast<uint8_t>(value & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void write_u32_le(uint8_t* dst, uint32_t value) {
  dst[0] = static_cast<uint8_t>(value & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  dst[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  dst[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void write_nodeid48_le(uint8_t* dst, uint64_t value) {
  // On-air NodeID48: lower 48 bits of domain uint64, written as 6-byte LE.
  // Upper 16 bits are always 0x0000 (domain invariant); they are not transmitted.
  dst[0] = static_cast<uint8_t>(value & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  dst[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  dst[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
  dst[4] = static_cast<uint8_t>((value >> 32) & 0xFF);
  dst[5] = static_cast<uint8_t>((value >> 40) & 0xFF);
}

uint16_t read_u16_le(const uint8_t* src) {
  return static_cast<uint16_t>(src[0]) |
         static_cast<uint16_t>(src[1]) << 8;
}

uint32_t read_u32_le(const uint8_t* src) {
  return static_cast<uint32_t>(src[0]) |
         static_cast<uint32_t>(src[1]) << 8 |
         static_cast<uint32_t>(src[2]) << 16 |
         static_cast<uint32_t>(src[3]) << 24;
}

uint64_t read_nodeid48_le(const uint8_t* src) {
  // Read 6-byte LE NodeID48 and expand to domain uint64 (upper 16 bits = 0x0000).
  return static_cast<uint64_t>(src[0]) |
         static_cast<uint64_t>(src[1]) << 8 |
         static_cast<uint64_t>(src[2]) << 16 |
         static_cast<uint64_t>(src[3]) << 24 |
         static_cast<uint64_t>(src[4]) << 32 |
         static_cast<uint64_t>(src[5]) << 40;
}

void write_i32_le(uint8_t* dst, int32_t value) {
  write_u32_le(dst, static_cast<uint32_t>(value));
}

int32_t read_i32_le(const uint8_t* src) {
  return static_cast<int32_t>(read_u32_le(src));
}

bool in_range_i32(int32_t value, int32_t min_value, int32_t max_value) {
  return value >= min_value && value <= max_value;
}

} // namespace

size_t encode_geo_beacon(const GeoBeaconFields& fields, ByteSpan out) {
  if (!out.data || out.size < kGeoBeaconSize) {
    return 0;
  }

  // Layout (22 bytes): [0] msg_type [1] ver [2] rsvd [3..8] nodeId48 LE
  //                    [9] pos_valid [10..13] lat_e7 [14..17] lon_e7
  //                    [18..19] pos_age_s [20..21] seq
  out.data[0] = kGeoBeaconMsgType;
  out.data[1] = kGeoBeaconVersion;
  out.data[2] = 0;
  write_nodeid48_le(out.data + 3, fields.node_id);
  out.data[9] = fields.pos_valid;
  write_i32_le(out.data + 10, fields.lat_e7);
  write_i32_le(out.data + 14, fields.lon_e7);
  write_u16_le(out.data + 18, fields.pos_age_s);
  write_u16_le(out.data + 20, fields.seq);

  return kGeoBeaconSize;
}

DecodeError decode_geo_beacon(ConstByteSpan in, GeoBeaconFields* out) {
  if (!in.data || !out || in.size < kGeoBeaconSize) {
    return DecodeError::ShortBuffer;
  }

  if (in.data[0] != kGeoBeaconMsgType) {
    return DecodeError::BadMsgType;
  }

  if (in.data[1] != kGeoBeaconVersion) {
    return DecodeError::BadVersion;
  }

  out->node_id = read_nodeid48_le(in.data + 3);
  out->pos_valid = in.data[9];
  out->lat_e7 = read_i32_le(in.data + 10);
  out->lon_e7 = read_i32_le(in.data + 14);
  out->pos_age_s = read_u16_le(in.data + 18);
  out->seq = read_u16_le(in.data + 20);

  if (out->pos_valid != 0) {
    if (!in_range_i32(out->lat_e7, kLatMin, kLatMax) ||
        !in_range_i32(out->lon_e7, kLonMin, kLonMax)) {
      return DecodeError::InvalidRange;
    }
  }

  return DecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
