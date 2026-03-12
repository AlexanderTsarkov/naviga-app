#include "pos_full_codec.h"

#include <cmath>

namespace naviga {
namespace protocol {

namespace pos_full_detail {

constexpr double kMaxU24 = 16777215.0;

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
  return static_cast<uint32_t>(std::round((clamp_lat(lat_deg) + 90.0) / 180.0 * kMaxU24));
}
uint32_t lon_to_u24(double lon_deg) {
  return static_cast<uint32_t>(std::round((clamp_lon(lon_deg) + 180.0) / 360.0 * kMaxU24));
}
double u24_to_lat(uint32_t v) {
  return static_cast<double>(v) / kMaxU24 * 180.0 - 90.0;
}
double u24_to_lon(uint32_t v) {
  return static_cast<double>(v) / kMaxU24 * 360.0 - 180.0;
}

} // namespace pos_full_detail

size_t encode_pos_full_frame(const PosFullFields& fields, uint8_t* out, size_t out_cap) {
  if (!out || out_cap < kPosFullFrameSize) {
    return 0;
  }
  PacketHeader hdr;
  hdr.msg_type = MsgType::BeaconPosFull;
  hdr.reserved = 0;
  hdr.payload_len = static_cast<uint8_t>(kPosFullPayloadSize);
  if (!encode_header(hdr, out, out_cap)) {
    return 0;
  }
  uint8_t* p = out + kHeaderSize;
  p[0] = kPosFullPayloadVersion;
  wire::write_nodeid48_le(p + 1, fields.node_id);
  wire::write_u16_le(p + 7, fields.seq16);
  wire::write_u24_le(p + 9, pos_full_detail::lat_to_u24(fields.lat_deg));
  wire::write_u24_le(p + 12, pos_full_detail::lon_to_u24(fields.lon_deg));
  pack_pos_quality_le(fields.fix_type, fields.pos_sats,
                      fields.pos_accuracy_bucket, fields.pos_flags_small,
                      p + 15);
  return kPosFullFrameSize;
}

PosFullDecodeError decode_pos_full_payload(const uint8_t* payload, size_t payload_len,
                                           PosFullFields* out) {
  if (!payload || !out || payload_len < kPosFullPayloadSize) {
    return PosFullDecodeError::ShortBuffer;
  }
  if (payload[0] != kPosFullPayloadVersion) {
    return PosFullDecodeError::BadPayloadVersion;
  }
  out->node_id = wire::read_nodeid48_le(payload + 1);
  out->seq16 = wire::read_u16_le(payload + 7);
  const uint32_t lat_u24 = wire::read_u24_le(payload + 9);
  const uint32_t lon_u24 = wire::read_u24_le(payload + 12);
  if (lat_u24 > static_cast<uint32_t>(pos_full_detail::kMaxU24) || lon_u24 > static_cast<uint32_t>(pos_full_detail::kMaxU24)) {
    return PosFullDecodeError::InvalidRange;
  }
  out->lat_deg = pos_full_detail::u24_to_lat(lat_u24);
  out->lon_deg = pos_full_detail::u24_to_lon(lon_u24);
  unpack_pos_quality_le(payload + 15,
                       &out->fix_type, &out->pos_sats,
                       &out->pos_accuracy_bucket, &out->pos_flags_small);
  return PosFullDecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
