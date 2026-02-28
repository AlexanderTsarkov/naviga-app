#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"
#include "wire_helpers.h"

namespace naviga {
namespace protocol {

/**
 * Node_OOTB_Informative packet v0 (legacy wire enum: BEACON_INFO, msg_type=0x05).
 *
 * Carries static / user-config fields. Dynamic runtime fields are in
 * Node_OOTB_Operational (msg_type=0x04, tail2_codec.h).
 *
 * On-air payload layout (Common prefix + Useful payload):
 *   [0]      payloadVersion = 0x00          (Common prefix byte 0)
 *   [1..6]   nodeId48 LE                    (Common prefix bytes 1–6)
 *   [7..8]   seq16 LE                       (Common prefix bytes 7–8: global per-node counter)
 *   [9]      maxSilence10s  (optional) uint8; unit=10s; 0=absent/unknown
 *   [10..11] hwProfileId    (optional) uint16 LE; 0xFFFF=not present
 *   [12..13] fwVersionId    (optional) uint16 LE; 0xFFFF=not present
 *
 * Minimum: 9 bytes (Common only). Maximum (all optional): 14 bytes.
 * On-air with 2-byte header: 11 bytes min, 16 bytes max.
 *
 * Spec: docs/product/areas/nodetable/contract/info_packet_encoding_v0.md, issue #314.
 */
struct InfoFields {
  uint64_t node_id           = 0;
  uint16_t seq16             = 0;  ///< Common prefix: global per-node counter.

  bool    has_max_silence    = false;
  uint8_t max_silence_10s    = 0;      ///< 0 = absent/unknown; unit = 10 s.

  bool     has_hw_profile    = false;
  uint16_t hw_profile_id     = 0xFFFF; ///< 0xFFFF = not present.

  bool     has_fw_version    = false;
  uint16_t fw_version_id     = 0xFFFF; ///< 0xFFFF = not present.
};

/** Info (Informative) payload version byte. */
constexpr uint8_t kInfoPayloadVersion = 0x00;

/** Minimum Info payload size (Common 9 B only). */
constexpr size_t kInfoPayloadMin = 9;

/** Maximum Info payload size (all optional fields present). */
constexpr size_t kInfoPayloadMax = 14;

/** Minimum on-air Info frame size (header + min payload). */
constexpr size_t kInfoFrameMin = kHeaderSize + kInfoPayloadMin;

/** Maximum on-air Info frame size (header + max payload). */
constexpr size_t kInfoFrameMax = kHeaderSize + kInfoPayloadMax;

/** Payload offsets for optional fields (relative to start of payload). */
constexpr size_t kInfoOffsetSeq16      = 7;   ///< Common: global seq16.
constexpr size_t kInfoOffsetMaxSilence = 9;   ///< maxSilence10s.
constexpr size_t kInfoOffsetHwProfile  = 10;  ///< hwProfileId (2 B LE).
constexpr size_t kInfoOffsetFwVersion  = 12;  ///< fwVersionId (2 B LE).

constexpr size_t kInfoSizeWithMaxSilence = 10;
constexpr size_t kInfoSizeWithHwProfile  = 12;
constexpr size_t kInfoSizeWithFwVersion  = 14;

enum class InfoDecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
  BadPayloadLen,  ///< payload_len < 9 or > 14.
};

/**
 * Encode a complete on-air Info (Informative) frame: 2-byte header + payload.
 *
 * Fields are included up to and including the last field where has_* is true.
 *
 * @param fields  Info fields to encode.
 * @param out     Destination buffer; MUST have at least kInfoFrameMin bytes.
 * @param out_cap Capacity of \a out.
 * @return Number of bytes written, or 0 on error.
 */
inline size_t encode_info_frame(const InfoFields& fields, uint8_t* out, size_t out_cap) {
  size_t payload_len = kInfoPayloadMin;
  if (fields.has_fw_version)   { payload_len = kInfoSizeWithFwVersion;  }
  else if (fields.has_hw_profile)  { payload_len = kInfoSizeWithHwProfile; }
  else if (fields.has_max_silence) { payload_len = kInfoSizeWithMaxSilence; }

  const size_t frame_size = kHeaderSize + payload_len;
  if (!out || out_cap < frame_size) {
    return 0;
  }

  PacketHeader hdr;
  hdr.msg_type    = MsgType::BeaconInfo;
  hdr.reserved    = 0;
  hdr.payload_len = static_cast<uint8_t>(payload_len);
  if (!encode_header(hdr, out, out_cap)) {
    return 0;
  }

  uint8_t* p = out + kHeaderSize;
  p[0] = kInfoPayloadVersion;
  wire::write_nodeid48_le(p + 1, fields.node_id);
  wire::write_u16_le(p + kInfoOffsetSeq16, fields.seq16);

  if (payload_len >= kInfoSizeWithMaxSilence) {
    p[kInfoOffsetMaxSilence] = fields.max_silence_10s;
  }
  if (payload_len >= kInfoSizeWithHwProfile) {
    wire::write_u16_le(p + kInfoOffsetHwProfile, fields.hw_profile_id);
  }
  if (payload_len >= kInfoSizeWithFwVersion) {
    wire::write_u16_le(p + kInfoOffsetFwVersion, fields.fw_version_id);
  }

  return frame_size;
}

/**
 * Decode an Info (Informative) payload (without the 2-byte header).
 *
 * The caller MUST have already stripped the 2-byte frame header and verified
 * msg_type == MsgType::BeaconInfo before calling this function.
 *
 * Optional fields are decoded based on payload_len. Valid range: [9..14].
 *
 * @param payload     Pointer to payload bytes (after the header).
 * @param payload_len Number of payload bytes.
 * @param out         Output fields.
 * @return InfoDecodeError::Ok on success.
 */
inline InfoDecodeError decode_info_payload(const uint8_t* payload, size_t payload_len,
                                           InfoFields* out) {
  if (!payload || !out || payload_len < kInfoPayloadMin) {
    return InfoDecodeError::ShortBuffer;
  }
  if (payload_len > kInfoPayloadMax) {
    return InfoDecodeError::BadPayloadLen;
  }
  if (payload[0] != kInfoPayloadVersion) {
    return InfoDecodeError::BadPayloadVersion;
  }

  out->node_id = wire::read_nodeid48_le(payload + 1);
  out->seq16   = wire::read_u16_le(payload + kInfoOffsetSeq16);

  out->has_max_silence = (payload_len >= kInfoSizeWithMaxSilence);
  out->max_silence_10s = out->has_max_silence ? payload[kInfoOffsetMaxSilence] : 0u;

  out->has_hw_profile  = (payload_len >= kInfoSizeWithHwProfile);
  out->hw_profile_id   = out->has_hw_profile
                           ? wire::read_u16_le(payload + kInfoOffsetHwProfile)
                           : 0xFFFFu;

  out->has_fw_version  = (payload_len >= kInfoSizeWithFwVersion);
  out->fw_version_id   = out->has_fw_version
                           ? wire::read_u16_le(payload + kInfoOffsetFwVersion)
                           : 0xFFFFu;

  return InfoDecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
