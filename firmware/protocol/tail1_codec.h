#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"
#include "wire_helpers.h"

namespace naviga {
namespace protocol {

/**
 * BeaconTail-1 packet v0 — payload fields.
 *
 * On-air payload layout (Common prefix + Functional payload):
 *   [0]      payloadVersion = 0x00          (Common prefix byte 0)
 *   [1..6]   nodeId48 LE                    (Common prefix bytes 1–6)
 *   [7..8]   ref_core_seq16 LE              (Functional payload: Core linkage key —
 *                                            back-reference to the seq16 of the Core
 *                                            sample this Tail-1 qualifies; NOT a
 *                                            per-frame freshness counter)
 *   [9]      posFlags  (optional; 0x00 = not present)
 *   [10]     sats      (optional; 0x00 = not present)
 *
 * Minimum: 9 bytes. With posFlags + sats: 11 bytes.
 * On-air with 2-byte header: 11 bytes min, 13 bytes max.
 *
 * Spec: docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md, issue #307.
 */
struct Tail1Fields {
  uint64_t node_id         = 0;
  uint16_t ref_core_seq16  = 0;  ///< Core linkage key: seq16 of the Core this Tail-1 qualifies.
  bool     has_pos_flags   = false;
  uint8_t  pos_flags       = 0x00;  ///< 0x00 = not present.
  bool     has_sats        = false;
  uint8_t  sats            = 0x00;  ///< 0x00 = not present.
};

/** Tail-1 payload version byte. */
constexpr uint8_t kTail1PayloadVersion = 0x00;

/** Minimum Tail-1 payload size (payloadVersion + nodeId48 + ref_core_seq16). */
constexpr size_t kTail1PayloadMin = 9;

/** Maximum Tail-1 payload size (with posFlags + sats). */
constexpr size_t kTail1PayloadMax = 11;

/** Minimum on-air Tail-1 frame size (header + min payload). */
constexpr size_t kTail1FrameMin = kHeaderSize + kTail1PayloadMin;

/** Maximum on-air Tail-1 frame size (header + max payload). */
constexpr size_t kTail1FrameMax = kHeaderSize + kTail1PayloadMax;

enum class Tail1DecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
  BadPayloadLen,  ///< payload_len not in {9, 11}.
};

/**
 * Encode a complete on-air Tail-1 frame: 2-byte header + payload.
 *
 * @param fields  Tail-1 fields to encode.
 * @param out     Destination buffer; MUST have at least kTail1FrameMin bytes.
 * @param out_cap Capacity of \a out.
 * @return Number of bytes written, or 0 on error.
 */
inline size_t encode_tail1_frame(const Tail1Fields& fields, uint8_t* out, size_t out_cap) {
  const bool with_optional = fields.has_pos_flags || fields.has_sats;
  const size_t payload_len = with_optional ? kTail1PayloadMax : kTail1PayloadMin;
  const size_t frame_size  = kHeaderSize + payload_len;

  if (!out || out_cap < frame_size) {
    return 0;
  }

  PacketHeader hdr;
  hdr.msg_type    = MsgType::BeaconTail1;
  hdr.reserved    = 0;
  hdr.payload_len = static_cast<uint8_t>(payload_len);
  if (!encode_header(hdr, out, out_cap)) {
    return 0;
  }

  uint8_t* p = out + kHeaderSize;
  p[0] = kTail1PayloadVersion;
  wire::write_nodeid48_le(p + 1, fields.node_id);
  wire::write_u16_le(p + 7, fields.ref_core_seq16);
  if (with_optional) {
    p[9]  = fields.pos_flags;
    p[10] = fields.sats;
  }

  return frame_size;
}

/**
 * Decode a Tail-1 payload (without the 2-byte header).
 *
 * The caller MUST have already stripped the 2-byte frame header and verified
 * msg_type == MsgType::BeaconTail1 before calling this function.
 *
 * Valid payload_len values: 9 (base) or 11 (with posFlags + sats).
 * Any other length is rejected per tail1_packet_encoding_v0.md §4.3.
 *
 * @param payload     Pointer to payload bytes (after the header).
 * @param payload_len Number of payload bytes.
 * @param out         Output fields.
 * @return Tail1DecodeError::Ok on success.
 */
inline Tail1DecodeError decode_tail1_payload(const uint8_t* payload, size_t payload_len,
                                             Tail1Fields* out) {
  if (!payload || !out || payload_len < kTail1PayloadMin) {
    return Tail1DecodeError::ShortBuffer;
  }
  if (payload_len != kTail1PayloadMin && payload_len != kTail1PayloadMax) {
    return Tail1DecodeError::BadPayloadLen;
  }
  if (payload[0] != kTail1PayloadVersion) {
    return Tail1DecodeError::BadPayloadVersion;
  }

  out->node_id        = wire::read_nodeid48_le(payload + 1);
  out->ref_core_seq16 = wire::read_u16_le(payload + 7);

  out->has_pos_flags = (payload_len >= kTail1PayloadMax);
  out->pos_flags     = out->has_pos_flags ? payload[9]  : 0x00u;
  out->has_sats      = (payload_len >= kTail1PayloadMax);
  out->sats          = out->has_sats      ? payload[10] : 0x00u;

  return Tail1DecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
