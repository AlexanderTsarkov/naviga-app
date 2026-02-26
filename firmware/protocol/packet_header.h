#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {
namespace protocol {

/**
 * 2-byte on-air frame header (7+3+6 bit layout).
 *
 * Wire layout — 16-bit little-endian word H = byte0 | (byte1 << 8):
 *   Bits [15:9]  msg_type    7 bits  (0x00–0x7F)
 *   Bits [8:6]   reserved    3 bits  (MUST be 0 on send; ignored on receive)
 *   Bits [5:0]   payload_len 6 bits  (0–63; count of payload bytes after header)
 *
 * Encode:
 *   byte0 = ((reserved & 0x3) << 6) | payload_len   // H & 0xFF
 *   byte1 = (msg_type << 1) | (reserved >> 2)        // (H >> 8) & 0xFF
 *
 * Golden example: msg_type=0x01, reserved=0, payload_len=15
 *   H = 0x020F  →  wire [0x0F, 0x02]
 *
 * Spec: docs/protocols/ootb_radio_v0.md §3, issue #304.
 */

/** Size of the frame header in bytes. */
constexpr size_t kHeaderSize = 2;

/** Maximum payload length encodable in the 6-bit payload_len field. */
constexpr size_t kMaxPayloadLen = 63;

/** Maximum on-air frame size (header + max payload). */
constexpr size_t kMaxFrameSize = kHeaderSize + kMaxPayloadLen;

/** msg_type registry v0 (ootb_radio_v0.md §3.2). */
enum class MsgType : uint8_t {
  Reserved    = 0x00,  ///< MUST NOT be used; drop on receive.
  BeaconCore  = 0x01,  ///< Position-bearing; 15 B fixed payload.
  BeaconAlive = 0x02,  ///< Alive-bearing, non-position; 9–10 B payload.
  BeaconTail1 = 0x03,  ///< Tail-1 operational; 9 B min payload.
  BeaconTail2 = 0x04,  ///< Tail-2 slow state; 7 B min payload.
};

/** Decoded header fields (in-memory representation). */
struct PacketHeader {
  MsgType  msg_type    = MsgType::Reserved;
  uint8_t  reserved    = 0;
  uint8_t  payload_len = 0;
};

/**
 * Encode a PacketHeader into the first 2 bytes of \a out.
 *
 * @param hdr     Header to encode. reserved MUST be 0; payload_len MUST be ≤ 63.
 * @param out     Destination buffer; MUST have at least kHeaderSize bytes.
 * @param out_cap Capacity of \a out.
 * @return true on success; false if out is null, too small, or payload_len > 63.
 */
inline bool encode_header(const PacketHeader& hdr, uint8_t* out, size_t out_cap) {
  if (!out || out_cap < kHeaderSize) {
    return false;
  }
  if (hdr.payload_len > kMaxPayloadLen) {
    return false;
  }
  const uint8_t mt = static_cast<uint8_t>(hdr.msg_type);
  const uint8_t res = hdr.reserved & 0x07;
  out[0] = static_cast<uint8_t>(((res & 0x3u) << 6) | hdr.payload_len);
  out[1] = static_cast<uint8_t>((mt << 1) | (res >> 2));
  return true;
}

/**
 * Decode the first 2 bytes of \a in into \a hdr.
 *
 * Validates msg_type: returns false if msg_type is Reserved (0x00) or
 * outside the known registry (> BeaconTail2). reserved bits are stored
 * as-is (non-zero reserved is accepted per forward-compat rule).
 *
 * @return true if msg_type is known and non-reserved; false otherwise.
 */
inline bool decode_header(const uint8_t* in, size_t in_size, PacketHeader* hdr) {
  if (!in || !hdr || in_size < kHeaderSize) {
    return false;
  }
  const uint16_t H = static_cast<uint16_t>(in[0]) |
                     (static_cast<uint16_t>(in[1]) << 8);
  const uint8_t mt  = static_cast<uint8_t>((H >> 9) & 0x7Fu);
  const uint8_t res = static_cast<uint8_t>((H >> 6) & 0x07u);
  const uint8_t pl  = static_cast<uint8_t>(H & 0x3Fu);

  if (mt == static_cast<uint8_t>(MsgType::Reserved) ||
      mt > static_cast<uint8_t>(MsgType::BeaconTail2)) {
    return false;
  }
  hdr->msg_type    = static_cast<MsgType>(mt);
  hdr->reserved    = res;
  hdr->payload_len = pl;
  return true;
}

/**
 * Validate that the header's payload_len matches the actual payload bytes available.
 *
 * @param hdr            Decoded header.
 * @param actual_payload Number of bytes after the header in the received frame.
 * @return true if hdr.payload_len == actual_payload.
 */
inline bool validate_header(const PacketHeader& hdr, size_t actual_payload) {
  return hdr.payload_len == actual_payload;
}

} // namespace protocol
} // namespace naviga
