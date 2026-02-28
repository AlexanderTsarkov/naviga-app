#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"
#include "wire_helpers.h"

namespace naviga {
namespace protocol {

/**
 * Node_OOTB_Core_Tail packet v0 (legacy wire enum: BEACON_TAIL1) — payload fields.
 *
 * On-air payload layout (Common prefix + Useful payload):
 *   [0]      payloadVersion = 0x00          (Common prefix byte 0)
 *   [1..6]   nodeId48 LE                    (Common prefix bytes 1–6)
 *   [7..8]   seq16 LE                       (Common prefix bytes 7–8: this packet's own
 *                                            global per-node counter value; unique packet id;
 *                                            used for (nodeId48, seq16) dedupe)
 *   [9..10]  ref_core_seq16 LE              (Useful payload: Core linkage key —
 *                                            back-reference to the seq16 of the
 *                                            Node_OOTB_Core_Pos sample this Tail qualifies;
 *                                            seq16 ≠ ref_core_seq16)
 *   [11]     posFlags  (optional; 0x00 = not present)
 *   [12]     sats      (optional; 0x00 = not present)
 *
 * Minimum: 11 bytes (Common + ref_core_seq16). With posFlags + sats: 13 bytes.
 * On-air with 2-byte header: 13 bytes min, 15 bytes max.
 *
 * TX invariant: at most one Core_Tail per Core_Pos sample. A pending Core_Tail
 * for a given ref_core_seq16 MAY be replaced in the TX queue (e.g. if a newer
 * Core_Pos is sent first) but MUST NOT be duplicated.
 *
 * Spec: docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md, issue #307/#314.
 */
struct Tail1Fields {
  uint64_t node_id         = 0;
  uint16_t seq16           = 0;  ///< Common prefix: this packet's own global counter value.
  uint16_t ref_core_seq16  = 0;  ///< Useful payload: Core linkage key (seq16 of Core_Pos sample).
  bool     has_pos_flags   = false;
  uint8_t  pos_flags       = 0x00;  ///< 0x00 = not present.
  bool     has_sats        = false;
  uint8_t  sats            = 0x00;  ///< 0x00 = not present.
};

/** Tail-1 payload version byte. */
constexpr uint8_t kTail1PayloadVersion = 0x00;

/** Minimum Tail-1 payload size (Common 9 B + ref_core_seq16 2 B). */
constexpr size_t kTail1PayloadMin = 11;

/** Maximum Tail-1 payload size (with posFlags + sats). */
constexpr size_t kTail1PayloadMax = 13;

/** Minimum on-air Tail-1 frame size (header + min payload). */
constexpr size_t kTail1FrameMin = kHeaderSize + kTail1PayloadMin;

/** Maximum on-air Tail-1 frame size (header + max payload). */
constexpr size_t kTail1FrameMax = kHeaderSize + kTail1PayloadMax;

/** Payload offsets. */
constexpr size_t kTail1OffsetSeq16        = 7;   ///< Common: own packet seq16.
constexpr size_t kTail1OffsetRefCoreSeq16 = 9;   ///< Useful payload: Core linkage key.
constexpr size_t kTail1OffsetPosFlags     = 11;  ///< Optional.
constexpr size_t kTail1OffsetSats         = 12;  ///< Optional.

enum class Tail1DecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
  BadPayloadLen,  ///< payload_len not in {11, 13}.
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
  wire::write_u16_le(p + kTail1OffsetSeq16, fields.seq16);
  wire::write_u16_le(p + kTail1OffsetRefCoreSeq16, fields.ref_core_seq16);
  if (with_optional) {
    p[kTail1OffsetPosFlags] = fields.pos_flags;
    p[kTail1OffsetSats]     = fields.sats;
  }

  return frame_size;
}

/**
 * Decode a Tail-1 payload (without the 2-byte header).
 *
 * The caller MUST have already stripped the 2-byte frame header and verified
 * msg_type == MsgType::BeaconTail1 before calling this function.
 *
 * Valid payload_len values: 11 (base) or 13 (with posFlags + sats).
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
  out->seq16          = wire::read_u16_le(payload + kTail1OffsetSeq16);
  out->ref_core_seq16 = wire::read_u16_le(payload + kTail1OffsetRefCoreSeq16);

  out->has_pos_flags = (payload_len >= kTail1PayloadMax);
  out->pos_flags     = out->has_pos_flags ? payload[kTail1OffsetPosFlags] : 0x00u;
  out->has_sats      = (payload_len >= kTail1PayloadMax);
  out->sats          = out->has_sats      ? payload[kTail1OffsetSats]     : 0x00u;

  return Tail1DecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
