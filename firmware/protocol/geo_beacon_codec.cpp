#include "geo_beacon_codec.h"

#include <cmath>

namespace naviga {
namespace protocol {

namespace {

constexpr double kMaxU24 = 16777215.0;  // 2^24 - 1

// Clamp helpers
double clamp_lat(double v) {
  if (v < -90.0) return -90.0;
  if (v > 90.0) return 90.0;
  return v;
}

double clamp_lon(double v) {
  if (v < -180.0) return -180.0;
  if (v > 180.0) return 180.0;
  return v;
}

uint32_t lat_to_u24(double lat_deg) {
  const double clamped = clamp_lat(lat_deg);
  return static_cast<uint32_t>(std::round((clamped + 90.0) / 180.0 * kMaxU24));
}

uint32_t lon_to_u24(double lon_deg) {
  const double clamped = clamp_lon(lon_deg);
  return static_cast<uint32_t>(std::round((clamped + 180.0) / 360.0 * kMaxU24));
}

double u24_to_lat(uint32_t v) {
  return static_cast<double>(v) / kMaxU24 * 180.0 - 90.0;
}

double u24_to_lon(uint32_t v) {
  return static_cast<double>(v) / kMaxU24 * 360.0 - 180.0;
}

void write_u16_le(uint8_t* dst, uint16_t value) {
  dst[0] = static_cast<uint8_t>(value & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void write_u24_le(uint8_t* dst, uint32_t value) {
  dst[0] = static_cast<uint8_t>(value & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  dst[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
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

uint32_t read_u24_le(const uint8_t* src) {
  return static_cast<uint32_t>(src[0]) |
         static_cast<uint32_t>(src[1]) << 8 |
         static_cast<uint32_t>(src[2]) << 16;
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

} // namespace

size_t encode_geo_beacon(const GeoBeaconFields& fields, ByteSpan out) {
  if (!out.data || out.size < kGeoBeaconSize) {
    return 0;
  }

  // BeaconCore packed24 v0 payload layout (15 bytes):
  //   [0]     payloadVersion = 0x00
  //   [1..6]  nodeId48 LE
  //   [7..8]  seq16 LE
  //   [9..11] lat_u24 LE
  //   [12..14] lon_u24 LE
  out.data[0] = kGeoBeaconPayloadVersion;
  write_nodeid48_le(out.data + 1, fields.node_id);
  write_u16_le(out.data + 7, fields.seq);
  write_u24_le(out.data + 9, lat_to_u24(fields.lat_deg));
  write_u24_le(out.data + 12, lon_to_u24(fields.lon_deg));

  return kGeoBeaconSize;
}

DecodeError decode_geo_beacon(ConstByteSpan in, GeoBeaconFields* out) {
  if (!in.data || !out || in.size < kGeoBeaconSize) {
    return DecodeError::ShortBuffer;
  }

  if (in.data[0] != kGeoBeaconPayloadVersion) {
    return DecodeError::BadPayloadVersion;
  }

  out->node_id = read_nodeid48_le(in.data + 1);
  out->seq = read_u16_le(in.data + 7);

  const uint32_t lat_u24 = read_u24_le(in.data + 9);
  const uint32_t lon_u24 = read_u24_le(in.data + 12);

  // Validate packed24 values are within [0, 16777215].
  if (lat_u24 > static_cast<uint32_t>(kMaxU24) ||
      lon_u24 > static_cast<uint32_t>(kMaxU24)) {
    return DecodeError::InvalidRange;
  }

  out->lat_deg = u24_to_lat(lat_u24);
  out->lon_deg = u24_to_lon(lon_u24);
  out->pos_valid = 1;  // BeaconCore is always position-bearing (§3.1).

  return DecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
