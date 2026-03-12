#include "status_codec.h"

namespace naviga {
namespace protocol {

size_t encode_status_frame(const StatusFields& fields, uint8_t* out, size_t out_cap) {
  if (!out || out_cap < kStatusFrameSize) {
    return 0;
  }
  PacketHeader hdr;
  hdr.msg_type = MsgType::BeaconStatus;
  hdr.reserved = 0;
  hdr.payload_len = static_cast<uint8_t>(kStatusPayloadSize);
  if (!encode_header(hdr, out, out_cap)) {
    return 0;
  }
  uint8_t* p = out + kHeaderSize;
  p[0] = kStatusPayloadVersion;
  wire::write_nodeid48_le(p + 1, fields.node_id);
  wire::write_u16_le(p + 7, fields.seq16);
  p[9] = fields.battery_percent;
  p[10] = fields.battery_est_rem_time;
  p[11] = fields.tx_power_ch_throttle;
  p[12] = fields.uptime10m;
  p[13] = fields.role_id;
  p[14] = fields.max_silence_10s;
  wire::write_u16_le(p + 15, fields.hw_profile_id);
  wire::write_u16_le(p + 17, fields.fw_version_id);
  return kStatusFrameSize;
}

StatusDecodeError decode_status_payload(const uint8_t* payload, size_t payload_len,
                                        StatusFields* out) {
  if (!payload || !out || payload_len < kStatusPayloadSize) {
    return StatusDecodeError::ShortBuffer;
  }
  if (payload[0] != kStatusPayloadVersion) {
    return StatusDecodeError::BadPayloadVersion;
  }
  out->node_id = wire::read_nodeid48_le(payload + 1);
  out->seq16 = wire::read_u16_le(payload + 7);
  out->battery_percent = payload[9];
  out->battery_est_rem_time = payload[10];
  out->tx_power_ch_throttle = payload[11];
  out->uptime10m = payload[12];
  out->role_id = payload[13];
  out->max_silence_10s = payload[14];
  out->hw_profile_id = wire::read_u16_le(payload + 15);
  out->fw_version_id = wire::read_u16_le(payload + 17);
  return StatusDecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
