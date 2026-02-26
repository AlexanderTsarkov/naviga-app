#include "alive_codec.h"
#include "wire_helpers.h"

namespace naviga {
namespace protocol {

size_t encode_alive_frame(const AliveFields& fields, uint8_t* out, size_t out_cap) {
  const size_t payload_len = fields.has_status ? kAlivePayloadMax : kAlivePayloadMin;
  const size_t frame_size  = kHeaderSize + payload_len;

  if (!out || out_cap < frame_size) {
    return 0;
  }

  // Write 2-byte frame header.
  PacketHeader hdr;
  hdr.msg_type    = MsgType::BeaconAlive;
  hdr.reserved    = 0;
  hdr.payload_len = static_cast<uint8_t>(payload_len);
  if (!encode_header(hdr, out, out_cap)) {
    return 0;
  }

  // Write payload starting at byte 2.
  uint8_t* p = out + kHeaderSize;
  p[0] = kAlivePayloadVersion;
  wire::write_nodeid48_le(p + 1, fields.node_id);
  wire::write_u16_le(p + 7, fields.seq);
  if (fields.has_status) {
    p[9] = fields.alive_status;
  }

  return frame_size;
}

AliveDecodeError decode_alive_payload(const uint8_t* payload, size_t payload_len,
                                      AliveFields* out) {
  if (!payload || !out || payload_len < kAlivePayloadMin) {
    return AliveDecodeError::ShortBuffer;
  }
  if (payload[0] != kAlivePayloadVersion) {
    return AliveDecodeError::BadPayloadVersion;
  }

  out->node_id      = wire::read_nodeid48_le(payload + 1);
  out->seq          = wire::read_u16_le(payload + 7);
  out->has_status   = (payload_len >= kAlivePayloadMax);
  out->alive_status = out->has_status ? payload[9] : 0x00u;

  return AliveDecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
