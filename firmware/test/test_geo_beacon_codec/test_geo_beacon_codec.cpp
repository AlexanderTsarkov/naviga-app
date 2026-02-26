#include <unity.h>

#include <cmath>
#include <cstdint>

#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/geo_beacon_codec.cpp"

using naviga::protocol::ByteSpan;
using naviga::protocol::ConstByteSpan;
using naviga::protocol::DecodeError;
using naviga::protocol::GeoBeaconFields;
using naviga::protocol::decode_geo_beacon;
using naviga::protocol::encode_geo_beacon;
using naviga::protocol::kGeoBeaconSize;
using naviga::protocol::kGeoBeaconPayloadVersion;

// Tolerance for degree round-trip: packed24 precision is ~1.1 m lat, ~2.4 m lon.
// 0.00003 degrees ≈ 3.3 m — safely above quantisation noise.
static constexpr double kDegTol = 0.00003;

static bool deg_near(double a, double b, double tol) {
  return (a - b) < tol && (b - a) < tol;
}

// ── Golden vector (canonical) ──────────────────────────────────────────────
// From docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md §5.1:
//   node_id = 0x0000_AABB_CCDD_EEFF, seq=1, lat=55.7558°N, lon=37.6173°E
//   Expected payload (15 bytes): 00 FF EE DD CC BB AA 01 00 10 4C CF 05 C0 9A
void test_golden_vector_encode() {
  GeoBeaconFields in{};
  in.node_id = 0x0000AABBCCDDEEFFULL;
  in.pos_valid = 1;
  in.lat_deg = 55.7558;
  in.lon_deg = 37.6173;
  in.seq = 1;

  uint8_t buf[kGeoBeaconSize] = {};
  const size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, written);

  // Verify full byte sequence against canonical doc example.
  const uint8_t expected[15] = {
      0x00,                               // payloadVersion
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, // nodeId48 LE
      0x01, 0x00,                          // seq16 LE = 1
      0x10, 0x4C, 0xCF,                    // lat_u24 LE = 13585424 (55.7558°)
      0x05, 0xC0, 0x9A                     // lon_u24 LE = 10141701 (37.6173°)
  };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, kGeoBeaconSize);
}

void test_golden_vector_decode() {
  const uint8_t buf[15] = {
      0x00,
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
      0x01, 0x00,
      0x10, 0x4C, 0xCF,
      0x05, 0xC0, 0x9A
  };

  GeoBeaconFields out{};
  const DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(0x0000AABBCCDDEEFFULL, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(1, out.seq);
  TEST_ASSERT_EQUAL_UINT8(1, out.pos_valid);
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, 55.7558, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, 37.6173, kDegTol));
}

// ── Round-trip ─────────────────────────────────────────────────────────────
void test_round_trip() {
  GeoBeaconFields in{};
  in.node_id = 0x0000AABBCCDDEEFFULL;
  in.pos_valid = 1;
  in.lat_deg = 55.7558;
  in.lon_deg = 37.6173;
  in.seq = 4567;

  uint8_t buf[kGeoBeaconSize] = {};
  const size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, written);

  GeoBeaconFields out{};
  const DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(in.node_id, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(in.seq, out.seq);
  TEST_ASSERT_EQUAL_UINT8(1, out.pos_valid);
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, in.lat_deg, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, in.lon_deg, kDegTol));
}

// Round-trip at extreme corners of the valid range.
void test_round_trip_extremes() {
  GeoBeaconFields in{};
  in.node_id = 42;
  in.pos_valid = 1;
  in.seq = 1;

  uint8_t buf[kGeoBeaconSize] = {};
  GeoBeaconFields out{};

  // North Pole / Date Line
  in.lat_deg = 90.0;
  in.lon_deg = 180.0;
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, 90.0, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, 180.0, kDegTol));

  // South Pole / Anti-meridian
  in.lat_deg = -90.0;
  in.lon_deg = -180.0;
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, -90.0, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, -180.0, kDegTol));

  // Origin
  in.lat_deg = 0.0;
  in.lon_deg = 0.0;
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, 0.0, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, 0.0, kDegTol));
}

// ── Clamp: out-of-range inputs are clamped, not rejected ───────────────────
void test_clamp_out_of_range_lat() {
  GeoBeaconFields in{};
  in.node_id = 1;
  in.pos_valid = 1;
  in.lat_deg = 200.0;  // out of range → clamped to 90
  in.lon_deg = 0.0;
  in.seq = 1;

  uint8_t buf[kGeoBeaconSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));

  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, 90.0, kDegTol));
}

void test_clamp_out_of_range_lon() {
  GeoBeaconFields in{};
  in.node_id = 1;
  in.pos_valid = 1;
  in.lat_deg = 0.0;
  in.lon_deg = -500.0;  // out of range → clamped to -180
  in.seq = 1;

  uint8_t buf[kGeoBeaconSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));

  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, -180.0, kDegTol));
}

// ── pos_valid guard: no-fix must not produce a packet ─────────────────────
void test_encode_no_fix_returns_zero() {
  GeoBeaconFields in{};
  in.node_id = 1;
  in.pos_valid = 0;  // no fix
  in.lat_deg = 55.7558;
  in.lon_deg = 37.6173;
  in.seq = 1;

  uint8_t buf[kGeoBeaconSize] = {};
  const size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(0, written);
}

// ── Buffer size checks ─────────────────────────────────────────────────────
void test_encode_short_buffer() {
  GeoBeaconFields in{};
  uint8_t buf[kGeoBeaconSize - 1] = {};
  const size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(0, written);
}

void test_decode_short_buffer() {
  uint8_t buf[kGeoBeaconSize - 1] = {};
  GeoBeaconFields out{};
  const DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::ShortBuffer, err);
}

// ── payloadVersion check ───────────────────────────────────────────────────
void test_bad_payload_version() {
  GeoBeaconFields in{};
  in.node_id = 1;
  in.pos_valid = 1;
  in.lat_deg = 0.0;
  in.lon_deg = 0.0;
  in.seq = 0;

  uint8_t buf[kGeoBeaconSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  TEST_ASSERT_EQUAL_UINT8(kGeoBeaconPayloadVersion, buf[0]);

  buf[0] = 0xFF;  // unknown payloadVersion
  GeoBeaconFields out{};
  const DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::BadPayloadVersion, err);
}

// ── NodeID48: upper 16 bits stripped on encode, zero on decode ─────────────
void test_nodeid48_upper_bits_stripped() {
  GeoBeaconFields in{};
  in.node_id = 0xDEAD001122334455ULL;
  in.pos_valid = 1;
  in.lat_deg = 0.0;
  in.lon_deg = 0.0;
  in.seq = 0;

  uint8_t buf[kGeoBeaconSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));

  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_EQUAL_UINT64(0x0000001122334455ULL, out.node_id);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_golden_vector_encode);
  RUN_TEST(test_golden_vector_decode);
  RUN_TEST(test_round_trip);
  RUN_TEST(test_round_trip_extremes);
  RUN_TEST(test_clamp_out_of_range_lat);
  RUN_TEST(test_clamp_out_of_range_lon);
  RUN_TEST(test_encode_no_fix_returns_zero);
  RUN_TEST(test_encode_short_buffer);
  RUN_TEST(test_decode_short_buffer);
  RUN_TEST(test_bad_payload_version);
  RUN_TEST(test_nodeid48_upper_bits_stripped);
  return UNITY_END();
}
