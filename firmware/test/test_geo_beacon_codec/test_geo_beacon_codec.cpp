/**
 * Unit tests for #282: Core 19B and Alive 11B codecs per canon.
 */
#include <unity.h>

#include <cstdint>

#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/geo_beacon_codec.cpp"

using naviga::protocol::ByteSpan;
using naviga::protocol::ConstByteSpan;
using naviga::protocol::DecodeError;
using naviga::protocol::GeoBeaconFields;
using naviga::protocol::decode_alive;
using naviga::protocol::decode_beacon;
using naviga::protocol::decode_core;
using naviga::protocol::encode_alive;
using naviga::protocol::encode_core;
using naviga::protocol::kAlivePayloadSize;
using naviga::protocol::kCorePayloadSize;

void test_core_encode_size_exactly_19() {
  GeoBeaconFields in{};
  in.node_id = 1;
  in.pos_valid = 1;
  in.lat_e7 = 0;
  in.lon_e7 = 0;
  in.seq = 1;

  uint8_t buf[32] = {};
  size_t written = encode_core(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(kCorePayloadSize, written);
  TEST_ASSERT_EQUAL_UINT32(19, written);
}

void test_alive_encode_size_exactly_11() {
  uint8_t buf[32] = {};
  size_t written = encode_alive(0x1122334455667788ULL, 42, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(kAlivePayloadSize, written);
  TEST_ASSERT_EQUAL_UINT32(11, written);
}

void test_core_roundtrip() {
  GeoBeaconFields in{};
  in.node_id = 0x0123456789ABCDEFULL;
  in.pos_valid = 1;
  in.lat_e7 = 557558000;
  in.lon_e7 = 376173000;
  in.seq = 0x1234;

  uint8_t buf[32] = {};
  size_t written = encode_core(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(19, written);

  GeoBeaconFields out{};
  DecodeError err = decode_core(ConstByteSpan{buf, written}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(in.node_id, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(in.seq, out.seq);
  TEST_ASSERT_EQUAL_INT32(in.lat_e7, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(in.lon_e7, out.lon_e7);
  TEST_ASSERT_EQUAL_UINT8(1, out.pos_valid);
}

void test_alive_roundtrip() {
  const uint64_t node_id = 0xEFCDAB8967452301ULL;
  const uint16_t seq = 2;

  uint8_t buf[32] = {};
  size_t written = encode_alive(node_id, seq, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(11, written);

  GeoBeaconFields out{};
  DecodeError err = decode_alive(ConstByteSpan{buf, written}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(node_id, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(seq, out.seq);
  TEST_ASSERT_EQUAL_UINT8(0, out.pos_valid);
  TEST_ASSERT_EQUAL_INT32(0, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(0, out.lon_e7);
}

void test_decode_beacon_dispatch_by_length() {
  GeoBeaconFields core_in{};
  core_in.node_id = 1;
  core_in.pos_valid = 1;
  core_in.lat_e7 = 100;
  core_in.lon_e7 = 200;
  core_in.seq = 1;

  uint8_t core_buf[32] = {};
  size_t core_len = encode_core(core_in, ByteSpan{core_buf, sizeof(core_buf)});
  GeoBeaconFields decoded{};
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_beacon(ConstByteSpan{core_buf, core_len}, &decoded));
  TEST_ASSERT_EQUAL_UINT8(1, decoded.pos_valid);
  TEST_ASSERT_EQUAL_INT32(100, decoded.lat_e7);

  uint8_t alive_buf[32] = {};
  size_t alive_len = encode_alive(2, 3, ByteSpan{alive_buf, sizeof(alive_buf)});
  decoded = GeoBeaconFields{};
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_beacon(ConstByteSpan{alive_buf, alive_len}, &decoded));
  TEST_ASSERT_EQUAL_UINT8(0, decoded.pos_valid);
  TEST_ASSERT_EQUAL_UINT64(2, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT16(3, decoded.seq);
}

void test_wrong_length_rejected() {
  uint8_t buf[20] = {0x00};
  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::BadMsgType, decode_beacon(ConstByteSpan{buf, 12}, &out));
  TEST_ASSERT_EQUAL(DecodeError::BadMsgType, decode_beacon(ConstByteSpan{buf, 18}, &out));
  TEST_ASSERT_EQUAL(DecodeError::BadMsgType, decode_beacon(ConstByteSpan{buf, 10}, &out));
}

void test_core_wrong_codec_fails() {
  GeoBeaconFields in{};
  in.node_id = 1;
  in.pos_valid = 1;
  in.lat_e7 = 0;
  in.lon_e7 = 0;
  in.seq = 0;

  uint8_t buf[32] = {};
  encode_core(in, ByteSpan{buf, sizeof(buf)});

  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::ShortBuffer, decode_alive(ConstByteSpan{buf, 19}, &out));
}

void test_alive_wrong_codec_fails() {
  uint8_t buf[32] = {};
  encode_alive(1, 0, ByteSpan{buf, sizeof(buf)});

  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::ShortBuffer, decode_core(ConstByteSpan{buf, 11}, &out));
}

void test_bad_version_rejected() {
  uint8_t buf[32] = {};
  encode_core(GeoBeaconFields{0, 1, 0, 0, 0, 0}, ByteSpan{buf, sizeof(buf)});
  buf[0] = 0x01;

  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::BadVersion, decode_core(ConstByteSpan{buf, 19}, &out));
}

void test_core_lat_lon_invalid_range() {
  GeoBeaconFields in{};
  in.node_id = 1;
  in.pos_valid = 1;
  in.lat_e7 = 900000001;
  in.lon_e7 = 0;
  in.seq = 0;

  uint8_t buf[32] = {};
  size_t written = encode_core(in, ByteSpan{buf, sizeof(buf)});
  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::InvalidRange, decode_core(ConstByteSpan{buf, written}, &out));
}

void test_core_known_vector() {
  const uint8_t expected[19] = {
      0x00, 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01,
      0x01, 0x00,
      0xF0, 0xA8, 0x3B, 0x21,
      0xC8, 0xF1, 0x6B, 0x16
  };
  GeoBeaconFields out{};
  DecodeError err = decode_core(ConstByteSpan{expected, sizeof(expected)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(0x0123456789ABCDEFULL, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(1, out.seq);
  TEST_ASSERT_EQUAL_INT32(557558000, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(376173000, out.lon_e7);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_core_encode_size_exactly_19);
  RUN_TEST(test_alive_encode_size_exactly_11);
  RUN_TEST(test_core_roundtrip);
  RUN_TEST(test_alive_roundtrip);
  RUN_TEST(test_decode_beacon_dispatch_by_length);
  RUN_TEST(test_wrong_length_rejected);
  RUN_TEST(test_core_wrong_codec_fails);
  RUN_TEST(test_alive_wrong_codec_fails);
  RUN_TEST(test_bad_version_rejected);
  RUN_TEST(test_core_lat_lon_invalid_range);
  RUN_TEST(test_core_known_vector);
  return UNITY_END();
}
