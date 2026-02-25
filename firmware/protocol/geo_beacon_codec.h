#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {
namespace protocol {

/** Decoded beacon fields (Core or Alive). For Alive, pos_valid=0 and lat_e7/lon_e7 unused. */
struct GeoBeaconFields {
  uint64_t node_id;
  uint8_t pos_valid;
  int32_t lat_e7;
  int32_t lon_e7;
  uint16_t pos_age_s;  // Core: not in 19B canon; set 0 when decoded. Alive: N/A.
  uint16_t seq;
};

enum class DecodeError {
  Ok = 0,
  ShortBuffer,
  BadMsgType,
  BadVersion,
  InvalidRange,
};

struct ByteSpan {
  uint8_t* data;
  size_t size;
};

struct ConstByteSpan {
  const uint8_t* data;
  size_t size;
};

// Canon (beacon_payload_encoding_v0, alive_packet_encoding_v0): Core 19B, Alive 11B.
constexpr size_t kCorePayloadSize = 19;
constexpr size_t kAlivePayloadSize = 11;
constexpr uint8_t kBeaconVersion = 0x00;

/** Max payload size for TX/RX buffers (Core is longer). */
constexpr size_t kGeoBeaconSize = kCorePayloadSize;

// Legacy alias for buffer sizing (same as Core).
constexpr uint8_t kGeoBeaconMsgType = 0x01;

/** Encode BeaconCore (19 B): version(1) | nodeId(8) | seq16(2) | positionLat(4) | positionLon(4). */
size_t encode_core(const GeoBeaconFields& fields, ByteSpan out);

/** Encode Alive (11 B): version(1) | nodeId(8) | seq16(2). */
size_t encode_alive(uint64_t node_id, uint16_t seq, ByteSpan out);

/** Decode BeaconCore (exactly 19 bytes). Fills pos_valid=1, lat, lon; pos_age_s=0. */
DecodeError decode_core(ConstByteSpan in, GeoBeaconFields* out);

/** Decode Alive (exactly 11 bytes). Fills node_id, seq; pos_valid=0, lat/lon=0, pos_age_s=0. */
DecodeError decode_alive(ConstByteSpan in, GeoBeaconFields* out);

/** Dispatch by length: 19 → decode_core, 11 → decode_alive; else error. */
DecodeError decode_beacon(ConstByteSpan in, GeoBeaconFields* out);

}  // namespace protocol
}  // namespace naviga
