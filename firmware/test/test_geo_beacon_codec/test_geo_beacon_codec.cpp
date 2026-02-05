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
  in.node_id = 0x1122334455667788ULL;
  in.pos_valid = 1;
  in.lat_e7 = 123456789;
  in.lon_e7 = -987654321;
  in.pos_age_s = 321;
  in.seq = 4567;

  uint8_t buf[24] = {};
  size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(24, written);

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
  const uint8_t buf[24] = {
      0x01, 0x01, 0x00, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
      0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
      0x03, 0x02};

  GeoBeaconFields out{};
  DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(0x1122334455667788ULL, out.node_id);
  TEST_ASSERT_EQUAL_UINT8(1, out.pos_valid);
  TEST_ASSERT_EQUAL_INT32(0, out.lat_e7);
  TEST_ASSERT_EQUAL_INT32(0, out.lon_e7);
  TEST_ASSERT_EQUAL_UINT16(16, out.pos_age_s);
  TEST_ASSERT_EQUAL_UINT16(0x0203, out.seq);
}

void test_encode_short_buffer() {
  GeoBeaconFields in{};
  uint8_t buf[23] = {};
  size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(0, written);
}

void test_decode_short_buffer() {
  uint8_t buf[23] = {};
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

  uint8_t buf[24] = {};
  TEST_ASSERT_EQUAL_UINT32(24, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));

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

  uint8_t buf[24] = {};
  TEST_ASSERT_EQUAL_UINT32(24, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));

  GeoBeaconFields out{};
  DecodeError err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);

  in.lat_e7 = 900000001;
  TEST_ASSERT_EQUAL_UINT32(24, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::InvalidRange, err);

  in.lat_e7 = 900000000;
  in.lon_e7 = 1800000001;
  TEST_ASSERT_EQUAL_UINT32(24, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  err = decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::InvalidRange, err);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_round_trip);
  RUN_TEST(test_known_vector_decode);
  RUN_TEST(test_encode_short_buffer);
  RUN_TEST(test_decode_short_buffer);
  RUN_TEST(test_bad_msg_type_and_bad_version);
  RUN_TEST(test_lat_lon_boundaries);
  return UNITY_END();
}
