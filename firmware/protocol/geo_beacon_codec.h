#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {
namespace protocol {

/**
 * BeaconCore packed24 v0 payload fields.
 *
 * On-air layout (15 bytes, payloadVersion=0x00):
 *   [0]     payloadVersion = 0x00
 *   [1..6]  nodeId48 LE (lower 48 bits of domain uint64_t)
 *   [7..8]  seq16 LE
 *   [9..11] lat_u24 LE  (packed24 absolute WGS84 latitude)
 *   [12..14] lon_u24 LE (packed24 absolute WGS84 longitude)
 *
 * pos_valid: not transmitted on-air. BeaconCore is only sent when fix is valid
 * (field_cadence_v0 §3.1). Caller sets pos_valid=1 to indicate valid fix;
 * codec encodes lat/lon only when pos_valid != 0. When pos_valid == 0, the
 * caller should send an Alive packet instead.
 *
 * Encode formula:  lat_u24 = round((lat_deg + 90.0)  / 180.0 * 16777215)
 *                  lon_u24 = round((lon_deg + 180.0) / 360.0 * 16777215)
 * Decode formula:  lat_deg = lat_u24 / 16777215.0 * 180.0 - 90.0
 *                  lon_deg = lon_u24 / 16777215.0 * 360.0 - 180.0
 *
 * Precision: ~1.1 m (lat), ~2.4 m (lon at equator).
 */
struct GeoBeaconFields {
  uint64_t node_id = 0;
  uint8_t pos_valid = 0;  ///< 1 = valid fix; 0 = no fix (send Alive instead of Core).
  double lat_deg = 0.0;   ///< Latitude in degrees [-90, +90]. Clamped on encode.
  double lon_deg = 0.0;   ///< Longitude in degrees [-180, +180]. Clamped on encode.
  uint16_t seq = 0;
};

enum class DecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
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

/** Payload size in bytes (no frame header). */
constexpr size_t kGeoBeaconSize = 15;
/** payloadVersion byte value for BeaconCore v0. */
constexpr uint8_t kGeoBeaconPayloadVersion = 0x00;
/** msg_type for the frame header (separate framing track; not written by this codec). */
constexpr uint8_t kGeoBeaconMsgType = 0x01;

size_t encode_geo_beacon(const GeoBeaconFields& fields, ByteSpan out);
DecodeError decode_geo_beacon(ConstByteSpan in, GeoBeaconFields* out);

} // namespace protocol
} // namespace naviga
