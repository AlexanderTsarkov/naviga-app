#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {
namespace protocol {

struct GeoBeaconFields {
  uint64_t node_id;
  uint8_t pos_valid;
  int32_t lat_e7;
  int32_t lon_e7;
  uint16_t pos_age_s;
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

constexpr size_t kGeoBeaconSize = 22;
constexpr uint8_t kGeoBeaconMsgType = 0x01;
constexpr uint8_t kGeoBeaconVersion = 1;

size_t encode_geo_beacon(const GeoBeaconFields& fields, ByteSpan out);
DecodeError decode_geo_beacon(ConstByteSpan in, GeoBeaconFields* out);

} // namespace protocol
} // namespace naviga
