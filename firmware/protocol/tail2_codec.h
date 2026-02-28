#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"
#include "wire_helpers.h"

namespace naviga {
namespace protocol {

/**
 * Node_OOTB_Operational packet v0 (legacy wire enum: BEACON_TAIL2, msg_type=0x04).
 *
 * Carries dynamic runtime fields only. Static/config fields moved to
 * Node_OOTB_Informative (msg_type=0x05, info_codec.h).
 *
 * On-air payload layout (Common prefix + Useful payload):
 *   [0]      payloadVersion = 0x00          (Common prefix byte 0)
 *   [1..6]   nodeId48 LE                    (Common prefix bytes 1–6)
 *   [7..8]   seq16 LE                       (Common prefix bytes 7–8: global per-node counter)
 *   [9]      batteryPercent (optional) uint8; 0xFF = not present
 *   [10..13] uptimeSec      (optional) uint32 LE; 0xFFFFFFFF = not present
 *   txPower: planned field (S03, not yet implemented)
 *
 * Minimum: 9 bytes (Common only). Maximum (all optional): 14 bytes.
 * On-air with 2-byte header: 11 bytes min, 16 bytes max.
 *
 * Spec: docs/product/areas/nodetable/contract/tail2_packet_encoding_v0.md, issue #307/#314.
 */
struct Tail2Fields {
  uint64_t node_id           = 0;
  uint16_t seq16             = 0;  ///< Common prefix: global per-node counter.

  bool    has_battery        = false;
  uint8_t battery_percent    = 0xFF; ///< 0xFF = not present.

  bool     has_uptime        = false;
  uint32_t uptime_sec        = 0xFFFFFFFFu; ///< 0xFFFFFFFF = not present.
};

/** Tail-2 (Operational) payload version byte. */
constexpr uint8_t kTail2PayloadVersion = 0x00;

/** Minimum Tail-2 payload size (Common 9 B only). */
constexpr size_t kTail2PayloadMin = 9;

/** Maximum Tail-2 payload size (all optional fields present). */
constexpr size_t kTail2PayloadMax = 14;

/** Minimum on-air Tail-2 frame size (header + min payload). */
constexpr size_t kTail2FrameMin = kHeaderSize + kTail2PayloadMin;

/** Maximum on-air Tail-2 frame size (header + max payload). */
constexpr size_t kTail2FrameMax = kHeaderSize + kTail2PayloadMax;

/** Payload offsets for optional fields (relative to start of payload). */
constexpr size_t kTail2OffsetSeq16   = 7;   ///< Common: global seq16.
constexpr size_t kTail2OffsetBattery = 9;   ///< batteryPercent.
constexpr size_t kTail2OffsetUptime  = 10;  ///< uptimeSec (4 B LE).

constexpr size_t kTail2SizeWithBattery = 10;
constexpr size_t kTail2SizeWithUptime  = 14;

enum class Tail2DecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
  BadPayloadLen,  ///< payload_len < 9 or > 14.
};

/**
 * Encode a complete on-air Tail-2 (Operational) frame: 2-byte header + payload.
 *
 * Fields are included up to and including the last field where has_* is true.
 *
 * @param fields  Tail-2 fields to encode.
 * @param out     Destination buffer; MUST have at least kTail2FrameMin bytes.
 * @param out_cap Capacity of \a out.
 * @return Number of bytes written, or 0 on error.
 */
inline size_t encode_tail2_frame(const Tail2Fields& fields, uint8_t* out, size_t out_cap) {
  size_t payload_len = kTail2PayloadMin;
  if (fields.has_uptime)   { payload_len = kTail2SizeWithUptime;  }
  else if (fields.has_battery) { payload_len = kTail2SizeWithBattery; }

  const size_t frame_size = kHeaderSize + payload_len;
  if (!out || out_cap < frame_size) {
    return 0;
  }

  PacketHeader hdr;
  hdr.msg_type    = MsgType::BeaconTail2;
  hdr.reserved    = 0;
  hdr.payload_len = static_cast<uint8_t>(payload_len);
  if (!encode_header(hdr, out, out_cap)) {
    return 0;
  }

  uint8_t* p = out + kHeaderSize;
  p[0] = kTail2PayloadVersion;
  wire::write_nodeid48_le(p + 1, fields.node_id);
  wire::write_u16_le(p + kTail2OffsetSeq16, fields.seq16);

  if (payload_len >= kTail2SizeWithBattery) {
    p[kTail2OffsetBattery] = fields.battery_percent;
  }
  if (payload_len >= kTail2SizeWithUptime) {
    p[kTail2OffsetUptime + 0] = static_cast<uint8_t>(fields.uptime_sec & 0xFFu);
    p[kTail2OffsetUptime + 1] = static_cast<uint8_t>((fields.uptime_sec >> 8) & 0xFFu);
    p[kTail2OffsetUptime + 2] = static_cast<uint8_t>((fields.uptime_sec >> 16) & 0xFFu);
    p[kTail2OffsetUptime + 3] = static_cast<uint8_t>((fields.uptime_sec >> 24) & 0xFFu);
  }

  return frame_size;
}

/**
 * Decode a Tail-2 (Operational) payload (without the 2-byte header).
 *
 * The caller MUST have already stripped the 2-byte frame header and verified
 * msg_type == MsgType::BeaconTail2 before calling this function.
 *
 * Optional fields are decoded based on payload_len. Valid range: [9..14].
 *
 * @param payload     Pointer to payload bytes (after the header).
 * @param payload_len Number of payload bytes.
 * @param out         Output fields.
 * @return Tail2DecodeError::Ok on success.
 */
inline Tail2DecodeError decode_tail2_payload(const uint8_t* payload, size_t payload_len,
                                             Tail2Fields* out) {
  if (!payload || !out || payload_len < kTail2PayloadMin) {
    return Tail2DecodeError::ShortBuffer;
  }
  if (payload_len > kTail2PayloadMax) {
    return Tail2DecodeError::BadPayloadLen;
  }
  if (payload[0] != kTail2PayloadVersion) {
    return Tail2DecodeError::BadPayloadVersion;
  }

  out->node_id = wire::read_nodeid48_le(payload + 1);
  out->seq16   = wire::read_u16_le(payload + kTail2OffsetSeq16);

  out->has_battery     = (payload_len >= kTail2SizeWithBattery);
  out->battery_percent = out->has_battery ? payload[kTail2OffsetBattery] : 0xFFu;

  out->has_uptime  = (payload_len >= kTail2SizeWithUptime);
  out->uptime_sec  = out->has_uptime
                       ? (static_cast<uint32_t>(payload[kTail2OffsetUptime + 0])        |
                          (static_cast<uint32_t>(payload[kTail2OffsetUptime + 1]) << 8)  |
                          (static_cast<uint32_t>(payload[kTail2OffsetUptime + 2]) << 16) |
                          (static_cast<uint32_t>(payload[kTail2OffsetUptime + 3]) << 24))
                       : 0xFFFFFFFFu;

  return Tail2DecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
