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
  dst[0] = static_cast<uint8_t>(value & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  dst[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  dst[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
  dst[4] = static_cast<uint8_t>((value >> 32) & 0xFF);
  dst[5] = static_cast<uint8_t>((value >> 40) & 0xFF);
  dst[6] = static_cast<uint8_t>((value >> 48) & 0xFF);
  dst[7] = static_cast<uint8_t>((value >> 56) & 0xFF);
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

uint64_t read_u64_le(const uint8_t* src) {
  return static_cast<uint64_t>(src[0]) |
         static_cast<uint64_t>(src[1]) << 8 |
         static_cast<uint64_t>(src[2]) << 16 |
         static_cast<uint64_t>(src[3]) << 24 |
         static_cast<uint64_t>(src[4]) << 32 |
         static_cast<uint64_t>(src[5]) << 40 |
         static_cast<uint64_t>(src[6]) << 48 |
         static_cast<uint64_t>(src[7]) << 56;
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

  out.data[0] = kGeoBeaconMsgType;
  out.data[1] = kGeoBeaconVersion;
  out.data[2] = 0;
  write_u64_le(out.data + 3, fields.node_id);
  out.data[11] = fields.pos_valid;
  write_i32_le(out.data + 12, fields.lat_e7);
  write_i32_le(out.data + 16, fields.lon_e7);
  write_u16_le(out.data + 20, fields.pos_age_s);
  write_u16_le(out.data + 22, fields.seq);

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

  out->node_id = read_u64_le(in.data + 3);
  out->pos_valid = in.data[11];
  out->lat_e7 = read_i32_le(in.data + 12);
  out->lon_e7 = read_i32_le(in.data + 16);
  out->pos_age_s = read_u16_le(in.data + 20);
  out->seq = read_u16_le(in.data + 22);

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
