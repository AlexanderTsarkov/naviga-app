#include "geo_beacon_codec.h"
#include "wire_helpers.h"

#include <cmath>

namespace naviga {
namespace protocol {

namespace {

constexpr double kMaxU24 = 16777215.0;  // 2^24 - 1

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

void write_u24_le(uint8_t* dst, uint32_t value) {
  dst[0] = static_cast<uint8_t>(value & 0xFFu);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
  dst[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
}

uint32_t read_u24_le(const uint8_t* src) {
  return static_cast<uint32_t>(src[0]) |
         (static_cast<uint32_t>(src[1]) << 8) |
         (static_cast<uint32_t>(src[2]) << 16);
}

} // namespace

size_t encode_geo_beacon(const GeoBeaconFields& fields, ByteSpan out) {
  if (!out.data || out.size < kGeoBeaconFrameSize) {
    return 0;
  }
  // BeaconCore is position-bearing only.
  if (fields.pos_valid == 0) {
    return 0;
  }

  // Write 2-byte frame header: msg_type=0x01, reserved=0, payload_len=15.
  PacketHeader hdr;
  hdr.msg_type    = MsgType::BeaconCore;
  hdr.reserved    = 0;
  hdr.payload_len = static_cast<uint8_t>(kGeoBeaconSize);
  if (!encode_header(hdr, out.data, out.size)) {
    return 0;
  }

  // Write BeaconCore payload starting at byte 2.
  uint8_t* p = out.data + kHeaderSize;
  p[0] = kGeoBeaconPayloadVersion;
  wire::write_nodeid48_le(p + 1, fields.node_id);
  wire::write_u16_le(p + 7, fields.seq);
  write_u24_le(p + 9, lat_to_u24(fields.lat_deg));
  write_u24_le(p + 12, lon_to_u24(fields.lon_deg));

  return kGeoBeaconFrameSize;
}

DecodeError decode_geo_beacon(ConstByteSpan in, GeoBeaconFields* out) {
  if (!in.data || !out || in.size < kGeoBeaconSize) {
    return DecodeError::ShortBuffer;
  }
  if (in.data[0] != kGeoBeaconPayloadVersion) {
    return DecodeError::BadPayloadVersion;
  }

  out->node_id = wire::read_nodeid48_le(in.data + 1);
  out->seq     = wire::read_u16_le(in.data + 7);

  const uint32_t lat_u24 = read_u24_le(in.data + 9);
  const uint32_t lon_u24 = read_u24_le(in.data + 12);

  if (lat_u24 > static_cast<uint32_t>(kMaxU24) ||
      lon_u24 > static_cast<uint32_t>(kMaxU24)) {
    return DecodeError::InvalidRange;
  }

  out->lat_deg   = u24_to_lat(lat_u24);
  out->lon_deg   = u24_to_lon(lon_u24);
  out->pos_valid = 1;  // BeaconCore is always position-bearing (§3.1).

  return DecodeError::Ok;
}

DecodeError decode_geo_beacon_frame(ConstByteSpan in, GeoBeaconFields* out) {
  if (!in.data || !out || in.size < kGeoBeaconFrameSize) {
    return DecodeError::ShortBuffer;
  }

  PacketHeader hdr;
  if (!decode_header(in.data, in.size, &hdr)) {
    return DecodeError::BadMsgType;
  }
  if (hdr.msg_type != MsgType::BeaconCore) {
    return DecodeError::BadMsgType;
  }
  if (!validate_header(hdr, in.size - kHeaderSize)) {
    return DecodeError::PayloadLenMismatch;
  }

  const ConstByteSpan payload{in.data + kHeaderSize, in.size - kHeaderSize};
  return decode_geo_beacon(payload, out);
}

} // namespace protocol
} // namespace naviga
