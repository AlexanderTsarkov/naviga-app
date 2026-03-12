#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"
#include "wire_helpers.h"

namespace naviga {
namespace protocol {

/**
 * Node_Pos_Full v0.2 (#435) — position + Pos_Quality in one packet.
 *
 * Payload: 9 B common (payloadVersion, nodeId48, seq16) + lat_u24 (3 B) + lon_u24 (3 B) + Pos_Quality (2 B LE).
 * Total payload 17 B; on-air 19 B with 2-byte header (msg_type=0x06).
 *
 * Pos_Quality 2 B LE: fix_type [2:0], pos_sats [8:3], pos_accuracy_bucket [11:9], pos_flags_small [15:12].
 */
struct PosFullFields {
  uint64_t node_id  = 0;
  uint16_t seq16    = 0;
  double   lat_deg  = 0.0;
  double   lon_deg  = 0.0;
  uint8_t  fix_type = 0;             ///< 3 bits (0=no_fix, 1=2d, 2=3d).
  uint8_t  pos_sats = 0;             ///< 6 bits (0–63).
  uint8_t  pos_accuracy_bucket = 0;   ///< 3 bits.
  uint8_t  pos_flags_small    = 0;   ///< 4 bits.
};

constexpr uint8_t kPosFullPayloadVersion = 0x00;
constexpr size_t kPosFullPayloadSize = 17;
constexpr size_t kPosFullFrameSize = kHeaderSize + kPosFullPayloadSize;

/** Pack Pos_Quality into 2 bytes LE. */
inline void pack_pos_quality_le(uint8_t fix_type, uint8_t pos_sats,
                                uint8_t pos_accuracy_bucket, uint8_t pos_flags_small,
                                uint8_t* out) {
  const uint16_t w = (static_cast<uint16_t>(fix_type & 0x07u)) |
                     (static_cast<uint16_t>(pos_sats & 0x3Fu) << 3) |
                     (static_cast<uint16_t>(pos_accuracy_bucket & 0x07u) << 9) |
                     (static_cast<uint16_t>(pos_flags_small & 0x0Fu) << 12);
  out[0] = static_cast<uint8_t>(w & 0xFFu);
  out[1] = static_cast<uint8_t>((w >> 8) & 0xFFu);
}

/** Unpack Pos_Quality from 2 bytes LE. */
inline void unpack_pos_quality_le(const uint8_t* in,
                                  uint8_t* fix_type, uint8_t* pos_sats,
                                  uint8_t* pos_accuracy_bucket, uint8_t* pos_flags_small) {
  const uint16_t w = static_cast<uint16_t>(in[0]) | (static_cast<uint16_t>(in[1]) << 8);
  *fix_type = static_cast<uint8_t>(w & 0x07u);
  *pos_sats = static_cast<uint8_t>((w >> 3) & 0x3Fu);
  *pos_accuracy_bucket = static_cast<uint8_t>((w >> 9) & 0x07u);
  *pos_flags_small = static_cast<uint8_t>((w >> 12) & 0x0Fu);
}

enum class PosFullDecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
  InvalidRange,
};

/**
 * Encode a complete Node_Pos_Full frame (2-byte header + 17-byte payload).
 * Uses same lat/lon encoding as GeoBeacon (u24 from degrees).
 */
size_t encode_pos_full_frame(const PosFullFields& fields, uint8_t* out, size_t out_cap);

/**
 * Decode Node_Pos_Full payload (17 bytes, without header).
 * Caller must have verified msg_type == BeaconPosFull and payload_len == 17.
 */
PosFullDecodeError decode_pos_full_payload(const uint8_t* payload, size_t payload_len,
                                           PosFullFields* out);

} // namespace protocol
} // namespace naviga
