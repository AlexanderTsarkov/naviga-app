#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"

namespace naviga {
namespace protocol {

/**
 * BeaconCore packed24 v0 payload fields.
 *
 * Payload layout (15 bytes, payloadVersion=0x00):
 *   [0]     payloadVersion = 0x00
 *   [1..6]  nodeId48 LE (lower 48 bits of domain uint64_t)
 *   [7..8]  seq16 LE
 *   [9..11] lat_u24 LE  (packed24 absolute WGS84 latitude)
 *   [12..14] lon_u24 LE (packed24 absolute WGS84 longitude)
 *
 * On-air frame = 2-byte header (msg_type=0x01) + 15-byte payload = 17 bytes total.
 *
 * pos_valid: not transmitted on-air. BeaconCore is only sent when fix is valid
 * (beacon_payload_encoding_v0 §3.1). Caller sets pos_valid=1; codec encodes
 * lat/lon only when pos_valid != 0.
 *
 * Encode formula:  lat_u24 = round((lat_deg + 90.0)  / 180.0 * 16777215)
 *                  lon_u24 = round((lon_deg + 180.0) / 360.0 * 16777215)
 * Decode formula:  lat_deg = lat_u24 / 16777215.0 * 180.0 - 90.0
 *                  lon_deg = lon_u24 / 16777215.0 * 360.0 - 180.0
 *
 * Precision: ~1.1 m (lat), ~2.4 m (lon at equator).
 *
 * Spec: beacon_payload_encoding_v0.md, issue #304.
 */
struct GeoBeaconFields {
  uint64_t node_id  = 0;
  uint8_t  pos_valid = 0;  ///< 1 = valid fix; 0 = no fix (send Alive instead of Core).
  double   lat_deg  = 0.0; ///< Latitude in degrees [-90, +90]. Clamped on encode.
  double   lon_deg  = 0.0; ///< Longitude in degrees [-180, +180]. Clamped on encode.
  uint16_t seq      = 0;
};

enum class DecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
  InvalidRange,
  BadMsgType,       ///< Header present but msg_type is not BeaconCore.
  PayloadLenMismatch, ///< Header payload_len != actual payload bytes.
};

struct ByteSpan {
  uint8_t* data;
  size_t   size;
};

struct ConstByteSpan {
  const uint8_t* data;
  size_t         size;
};

/** BeaconCore payload size in bytes (no frame header). */
constexpr size_t kGeoBeaconSize = 15;

/** BeaconCore on-air frame size (2-byte header + 15-byte payload). */
constexpr size_t kGeoBeaconFrameSize = kHeaderSize + kGeoBeaconSize;

/** payloadVersion byte value for BeaconCore v0. */
constexpr uint8_t kGeoBeaconPayloadVersion = 0x00;

/** msg_type value for BeaconCore (ootb_radio_v0.md §3.2). */
constexpr uint8_t kGeoBeaconMsgType = static_cast<uint8_t>(MsgType::BeaconCore);

/**
 * Encode a complete on-air BeaconCore frame: 2-byte header + 15-byte payload.
 *
 * Returns kGeoBeaconFrameSize (17) on success, 0 on error (no fix, short buffer).
 * Caller MUST provide a buffer of at least kGeoBeaconFrameSize bytes.
 */
size_t encode_geo_beacon(const GeoBeaconFields& fields, ByteSpan out);

/**
 * Decode a BeaconCore payload (15 bytes, WITHOUT the 2-byte frame header).
 *
 * Used by tests and by the framing dispatch layer after header has been stripped.
 * Returns DecodeError::Ok on success.
 */
DecodeError decode_geo_beacon(ConstByteSpan in, GeoBeaconFields* out);

/**
 * Decode a complete on-air BeaconCore frame (header + payload, 17 bytes).
 *
 * Reads and validates the 2-byte header (msg_type must be BeaconCore),
 * then decodes the payload. Returns DecodeError::Ok on success.
 */
DecodeError decode_geo_beacon_frame(ConstByteSpan in, GeoBeaconFields* out);

} // namespace protocol
} // namespace naviga
