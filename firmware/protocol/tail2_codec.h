#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"
#include "wire_helpers.h"

namespace naviga {
namespace protocol {

/**
 * BeaconTail-2 packet v0 — payload fields.
 *
 * On-air payload layout (fields present if payload_len covers their offset):
 *   [0]      payloadVersion = 0x00
 *   [1..6]   nodeId48 LE
 *   [7]      maxSilence10s  (optional) uint8; unit=10s; 0=absent/unknown
 *   [8]      batteryPercent (optional) uint8; 0xFF=not present
 *   [9..10]  hwProfileId    (optional) uint16 LE; 0xFFFF=not present
 *   [11..12] fwVersionId    (optional) uint16 LE; 0xFFFF=not present
 *   [13..16] uptimeSec      (optional) uint32 LE; 0xFFFFFFFF=not present
 *
 * Minimum: 7 bytes. Maximum (all optional): 17 bytes.
 * On-air with 2-byte header: 9 bytes min, 19 bytes max.
 *
 * Spec: docs/product/areas/nodetable/contract/tail2_packet_encoding_v0.md, issue #307.
 */
struct Tail2Fields {
  uint64_t node_id           = 0;

  bool    has_max_silence    = false;
  uint8_t max_silence_10s    = 0;    ///< 0 = absent/unknown; unit = 10 s.

  bool    has_battery        = false;
  uint8_t battery_percent    = 0xFF; ///< 0xFF = not present.

  bool     has_hw_profile    = false;
  uint16_t hw_profile_id     = 0xFFFF; ///< 0xFFFF = not present.

  bool     has_fw_version    = false;
  uint16_t fw_version_id     = 0xFFFF; ///< 0xFFFF = not present.

  bool     has_uptime        = false;
  uint32_t uptime_sec        = 0xFFFFFFFFu; ///< 0xFFFFFFFF = not present.
};

/** Tail-2 payload version byte. */
constexpr uint8_t kTail2PayloadVersion = 0x00;

/** Minimum Tail-2 payload size (payloadVersion + nodeId48). */
constexpr size_t kTail2PayloadMin = 7;

/** Maximum Tail-2 payload size (all optional fields present). */
constexpr size_t kTail2PayloadMax = 17;

/** Minimum on-air Tail-2 frame size (header + min payload). */
constexpr size_t kTail2FrameMin = kHeaderSize + kTail2PayloadMin;

/** Maximum on-air Tail-2 frame size (header + max payload). */
constexpr size_t kTail2FrameMax = kHeaderSize + kTail2PayloadMax;

/** Payload offsets for optional fields. */
constexpr size_t kTail2OffsetMaxSilence    = 7;
constexpr size_t kTail2OffsetBattery       = 8;
constexpr size_t kTail2OffsetHwProfile     = 9;
constexpr size_t kTail2OffsetFwVersion     = 11;
constexpr size_t kTail2OffsetUptime        = 13;
constexpr size_t kTail2SizeWithMaxSilence  = 8;
constexpr size_t kTail2SizeWithBattery     = 9;
constexpr size_t kTail2SizeWithHwProfile   = 11;
constexpr size_t kTail2SizeWithFwVersion   = 13;
constexpr size_t kTail2SizeWithUptime      = 17;

enum class Tail2DecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
  BadPayloadLen,  ///< payload_len < 7 or > 17.
};

/**
 * Encode a complete on-air Tail-2 frame: 2-byte header + payload.
 *
 * Fields are included up to and including the last field where has_* is true.
 * All intermediate fields MUST be present (no gaps).
 *
 * @param fields  Tail-2 fields to encode.
 * @param out     Destination buffer; MUST have at least kTail2FrameMin bytes.
 * @param out_cap Capacity of \a out.
 * @return Number of bytes written, or 0 on error.
 */
inline size_t encode_tail2_frame(const Tail2Fields& fields, uint8_t* out, size_t out_cap) {
  // Determine payload size: include up to the last present optional field.
  size_t payload_len = kTail2PayloadMin;
  if (fields.has_uptime)     { payload_len = kTail2SizeWithUptime;     }
  else if (fields.has_fw_version)  { payload_len = kTail2SizeWithFwVersion;  }
  else if (fields.has_hw_profile)  { payload_len = kTail2SizeWithHwProfile;  }
  else if (fields.has_battery)     { payload_len = kTail2SizeWithBattery;    }
  else if (fields.has_max_silence) { payload_len = kTail2SizeWithMaxSilence; }

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

  if (payload_len >= kTail2SizeWithMaxSilence) {
    p[kTail2OffsetMaxSilence] = fields.max_silence_10s;
  }
  if (payload_len >= kTail2SizeWithBattery) {
    p[kTail2OffsetBattery] = fields.battery_percent;
  }
  if (payload_len >= kTail2SizeWithHwProfile) {
    wire::write_u16_le(p + kTail2OffsetHwProfile, fields.hw_profile_id);
  }
  if (payload_len >= kTail2SizeWithFwVersion) {
    wire::write_u16_le(p + kTail2OffsetFwVersion, fields.fw_version_id);
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
 * Decode a Tail-2 payload (without the 2-byte header).
 *
 * The caller MUST have already stripped the 2-byte frame header and verified
 * msg_type == MsgType::BeaconTail2 before calling this function.
 *
 * Optional fields are decoded based on payload_len (fields present if their
 * offset + size <= payload_len). Valid range: [7..17].
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

  out->has_max_silence = (payload_len >= kTail2SizeWithMaxSilence);
  out->max_silence_10s = out->has_max_silence ? payload[kTail2OffsetMaxSilence] : 0u;

  out->has_battery     = (payload_len >= kTail2SizeWithBattery);
  out->battery_percent = out->has_battery ? payload[kTail2OffsetBattery] : 0xFFu;

  out->has_hw_profile  = (payload_len >= kTail2SizeWithHwProfile);
  out->hw_profile_id   = out->has_hw_profile
                           ? wire::read_u16_le(payload + kTail2OffsetHwProfile)
                           : 0xFFFFu;

  out->has_fw_version  = (payload_len >= kTail2SizeWithFwVersion);
  out->fw_version_id   = out->has_fw_version
                           ? wire::read_u16_le(payload + kTail2OffsetFwVersion)
                           : 0xFFFFu;

  out->has_uptime      = (payload_len >= kTail2SizeWithUptime);
  out->uptime_sec      = out->has_uptime
                           ? (static_cast<uint32_t>(payload[kTail2OffsetUptime + 0])       |
                              (static_cast<uint32_t>(payload[kTail2OffsetUptime + 1]) << 8) |
                              (static_cast<uint32_t>(payload[kTail2OffsetUptime + 2]) << 16)|
                              (static_cast<uint32_t>(payload[kTail2OffsetUptime + 3]) << 24))
                           : 0xFFFFFFFFu;

  return Tail2DecodeError::Ok;
}

} // namespace protocol
} // namespace naviga
