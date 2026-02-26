#include <unity.h>

#include <cstdint>

#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/geo_beacon_codec.cpp"

using naviga::protocol::ByteSpan;
using naviga::protocol::ConstByteSpan;
using naviga::protocol::DecodeError;
using naviga::protocol::GeoBeaconFields;
using naviga::protocol::decode_geo_beacon;
using naviga::protocol::encode_geo_beacon;

void test_round_trip() {
  GeoBeaconFields in{};
  // NodeID48: upper 16 bits must be 0x0000 (domain invariant).
  in.node_id = 0x0000AABBCCDDEEFFULL;
  in.pos_valid = 1;
  in.lat_e7 = 123456789;
  in.lon_e7 = -987654321;
  in.pos_age_s = 321;
  in.seq = 4567;

  uint8_t buf[22] = {};
  size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(22, written);

  GeoBeaconFields out{};
  DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(in.node_id, out.node_id);
  TEST_ASSERT_EQUAL_UINT8(in.pos_valid, out.pos_valid);
  TEST_ASSERT_EQUAL_INT32(in.lat_e7, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(in.lon_e7, out.lon_e7);
  TEST_ASSERT_EQUAL_UINT16(in.pos_age_s, out.pos_age_s);
  TEST_ASSERT_EQUAL_UINT16(in.seq, out.seq);
}

void test_known_vector_decode() {
  // 22-byte layout: [0] msg_type [1] ver [2] rsvd [3..8] nodeId48 LE
  //                 [9] pos_valid [10..13] lat_e7 [14..17] lon_e7
  //                 [18..19] pos_age_s [20..21] seq
  // node_id = 0x0000AABBCCDDEEFF → 6-byte LE: FF EE DD CC BB AA
  const uint8_t buf[22] = {
      0x01, 0x01, 0x00,
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
      0x01,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x10, 0x00,
      0x03, 0x02};

  GeoBeaconFields out{};
  DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(0x0000AABBCCDDEEFFULL, out.node_id);
  TEST_ASSERT_EQUAL_UINT8(1, out.pos_valid);
  TEST_ASSERT_EQUAL_INT32(0, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(0, out.lon_e7);
  TEST_ASSERT_EQUAL_UINT16(16, out.pos_age_s);
  TEST_ASSERT_EQUAL_UINT16(0x0203, out.seq);
}

void test_encode_short_buffer() {
  GeoBeaconFields in{};
  uint8_t buf[21] = {};
  size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(0, written);
}

void test_decode_short_buffer() {
  uint8_t buf[21] = {};
  GeoBeaconFields out{};
  DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::ShortBuffer, err);
}

void test_bad_msg_type_and_bad_version() {
  GeoBeaconFields in{};
  in.node_id = 1;
  in.pos_valid = 0;
  in.lat_e7 = 0;
  in.lon_e7 = 0;
  in.pos_age_s = 0;
  in.seq = 0;

  uint8_t buf[22] = {};
  TEST_ASSERT_EQUAL_UINT32(22, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));

  buf[0] = 0x02;
  GeoBeaconFields out{};
  DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::BadMsgType, err);

  buf[0] = 0x01;
  buf[1] = 0x02;
  err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::BadVersion, err);
}

void test_lat_lon_boundaries() {
  GeoBeaconFields in{};
  in.node_id = 42;
  in.pos_valid = 1;
  in.lat_e7 = 900000000;
  in.lon_e7 = 1800000000;
  in.pos_age_s = 0;
  in.seq = 0;

  uint8_t buf[22] = {};
  TEST_ASSERT_EQUAL_UINT32(22, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));

  GeoBeaconFields out{};
  DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);

  in.lat_e7 = 900000001;
  TEST_ASSERT_EQUAL_UINT32(22, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::InvalidRange, err);

  in.lat_e7 = 900000000;
  in.lon_e7 = 1800000001;
  TEST_ASSERT_EQUAL_UINT32(22, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::InvalidRange, err);
}

// Verify that upper 16 bits of node_id are not transmitted and are zero on decode.
void test_nodeid48_upper_bits_stripped() {
  GeoBeaconFields in{};
  // Set upper 16 bits non-zero; codec must only transmit lower 48.
  in.node_id = 0xDEAD001122334455ULL;
  in.pos_valid = 0;
  in.lat_e7 = 0;
  in.lon_e7 = 0;
  in.pos_age_s = 0;
  in.seq = 0;

  uint8_t buf[22] = {};
  TEST_ASSERT_EQUAL_UINT32(22, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));

  GeoBeaconFields out{};
  DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  // Decoded node_id must have upper 16 bits = 0x0000.
  TEST_ASSERT_EQUAL_UINT64(0x0000001122334455ULL, out.node_id);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_round_trip);
  RUN_TEST(test_known_vector_decode);
  RUN_TEST(test_encode_short_buffer);
  RUN_TEST(test_decode_short_buffer);
  RUN_TEST(test_bad_msg_type_and_bad_version);
  RUN_TEST(test_lat_lon_boundaries);
  RUN_TEST(test_nodeid48_upper_bits_stripped);
  return UNITY_END();
}
