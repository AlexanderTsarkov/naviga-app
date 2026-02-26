#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"

namespace naviga {
namespace protocol {

/**
 * Alive packet v0 — payload fields.
 *
 * On-air payload layout (9 bytes min, 10 bytes with aliveStatus):
 *   [0]     payloadVersion = 0x00
 *   [1..6]  nodeId48 LE (lower 48 bits of domain uint64_t)
 *   [7..8]  seq16 LE
 *   [9]     aliveStatus (optional; 0x00 = alive_no_fix)
 *
 * The 2-byte frame header (msg_type=0x02) is written by encode_alive_frame
 * and consumed by the framing layer on receive; it is NOT part of this struct.
 *
 * Spec: docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md, issue #304.
 */
struct AliveFields {
  uint64_t node_id      = 0;
  uint16_t seq          = 0;
  bool     has_status   = false;
  uint8_t  alive_status = 0x00;  ///< 0x00 = alive_no_fix (only value used in V1-A).
};

/** Alive payload version byte. */
constexpr uint8_t kAlivePayloadVersion = 0x00;

/** Minimum Alive payload size (without aliveStatus). */
constexpr size_t kAlivePayloadMin = 9;

/** Maximum Alive payload size (with aliveStatus). */
constexpr size_t kAlivePayloadMax = 10;

/** Minimum on-air Alive frame size (header + min payload). */
constexpr size_t kAliveFrameMin = kHeaderSize + kAlivePayloadMin;

/** Maximum on-air Alive frame size (header + max payload). */
constexpr size_t kAliveFrameMax = kHeaderSize + kAlivePayloadMax;

enum class AliveDecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
};

/**
 * Encode a complete on-air Alive frame: 2-byte header + payload.
 *
 * @param fields  Alive fields to encode.
 * @param out     Destination buffer; MUST have at least kAliveFrameMin bytes
 *                (or kAliveFrameMax if fields.has_status).
 * @param out_cap Capacity of \a out.
 * @return Number of bytes written (kAliveFrameMin or kAliveFrameMax), or 0 on error.
 */
size_t encode_alive_frame(const AliveFields& fields, uint8_t* out, size_t out_cap);

/**
 * Decode an Alive payload (without the 2-byte header).
 *
 * The caller MUST have already stripped the 2-byte frame header and verified
 * msg_type == MsgType::BeaconAlive before calling this function.
 *
 * @param payload     Pointer to payload bytes (after the header).
 * @param payload_len Number of payload bytes.
 * @param out         Output fields.
 * @return AliveDecodeError::Ok on success.
 */
AliveDecodeError decode_alive_payload(const uint8_t* payload, size_t payload_len,
                                      AliveFields* out);

} // namespace protocol
} // namespace naviga
