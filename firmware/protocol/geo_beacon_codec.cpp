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

void write_u64_le(uint8_t* dst, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    dst[i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
  }
}

void write_i32_le(uint8_t* dst, int32_t value) {
  write_u32_le(dst, static_cast<uint32_t>(value));
}

uint16_t read_u16_le(const uint8_t* src) {
  return static_cast<uint16_t>(src[0]) | (static_cast<uint16_t>(src[1]) << 8);
}

uint32_t read_u32_le(const uint8_t* src) {
  return static_cast<uint32_t>(src[0]) | (static_cast<uint32_t>(src[1]) << 8) |
         (static_cast<uint32_t>(src[2]) << 16) | (static_cast<uint32_t>(src[3]) << 24);
}

uint64_t read_u64_le(const uint8_t* src) {
  uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v |= static_cast<uint64_t>(src[i]) << (8 * i);
  }
  return v;
}

int32_t read_i32_le(const uint8_t* src) {
  return static_cast<int32_t>(read_u32_le(src));
}

bool in_range_i32(int32_t value, int32_t min_value, int32_t max_value) {
  return value >= min_value && value <= max_value;
}

}  // namespace

// Core: payloadVersion(1) | nodeId(8) | seq16(2) | positionLat(4) | positionLon(4) = 19 B.
size_t encode_core(const GeoBeaconFields& fields, ByteSpan out) {
  if (!out.data || out.size < kCorePayloadSize) {
    return 0;
  }
  out.data[0] = kBeaconVersion;
  write_u64_le(out.data + 1, fields.node_id);
  write_u16_le(out.data + 9, fields.seq);
  write_i32_le(out.data + 11, fields.lat_e7);
  write_i32_le(out.data + 15, fields.lon_e7);
  return kCorePayloadSize;
}

// Alive: payloadVersion(1) | nodeId(8) | seq16(2) = 11 B.
size_t encode_alive(uint64_t node_id, uint16_t seq, ByteSpan out) {
  if (!out.data || out.size < kAlivePayloadSize) {
    return 0;
  }
  out.data[0] = kBeaconVersion;
  write_u64_le(out.data + 1, node_id);
  write_u16_le(out.data + 9, seq);
  return kAlivePayloadSize;
}

DecodeError decode_core(ConstByteSpan in, GeoBeaconFields* out) {
  if (!in.data || !out || in.size != kCorePayloadSize) {
    return DecodeError::ShortBuffer;
  }
  if (in.data[0] != kBeaconVersion) {
    return DecodeError::BadVersion;
  }
  out->node_id = read_u64_le(in.data + 1);
  out->seq = read_u16_le(in.data + 9);
  out->lat_e7 = read_i32_le(in.data + 11);
  out->lon_e7 = read_i32_le(in.data + 15);
  out->pos_valid = 1;
  out->pos_age_s = 0;
  if (!in_range_i32(out->lat_e7, kLatMin, kLatMax) ||
      !in_range_i32(out->lon_e7, kLonMin, kLonMax)) {
    return DecodeError::InvalidRange;
  }
  return DecodeError::Ok;
}

DecodeError decode_alive(ConstByteSpan in, GeoBeaconFields* out) {
  if (!in.data || !out || in.size != kAlivePayloadSize) {
    return DecodeError::ShortBuffer;
  }
  if (in.data[0] != kBeaconVersion) {
    return DecodeError::BadVersion;
  }
  out->node_id = read_u64_le(in.data + 1);
  out->seq = read_u16_le(in.data + 9);
  out->pos_valid = 0;
  out->lat_e7 = 0;
  out->lon_e7 = 0;
  out->pos_age_s = 0;
  return DecodeError::Ok;
}

DecodeError decode_beacon(ConstByteSpan in, GeoBeaconFields* out) {
  if (!in.data || !out) {
    return DecodeError::ShortBuffer;
  }
  if (in.size == kCorePayloadSize) {
    return decode_core(in, out);
  }
  if (in.size == kAlivePayloadSize) {
    return decode_alive(in, out);
  }
  return DecodeError::BadMsgType;
}

}  // namespace protocol
}  // namespace naviga
