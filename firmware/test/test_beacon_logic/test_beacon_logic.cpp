#include <unity.h>

#include <cstdint>

#include "../../src/domain/beacon_logic.h"
#include "../../src/domain/beacon_logic.cpp"
#include "../../src/domain/node_table.h"
#include "../../src/domain/node_table.cpp"
#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/geo_beacon_codec.cpp"

using naviga::domain::BeaconLogic;
using naviga::domain::NodeTable;
using naviga::protocol::GeoBeaconFields;

namespace {

uint16_t read_u16_le(const uint8_t* data) {
  return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
}

uint32_t read_u32_le(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

uint64_t read_u64_le(const uint8_t* data) {
  uint64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value |= (static_cast<uint64_t>(data[i]) << (8 * i));
  }
  return value;
}

} // namespace

void test_tx_cadence() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);

  GeoBeaconFields fields{};
  fields.node_id = 0x1122334455667788ULL;
  fields.pos_valid = 1;
  fields.lat_e7 = 10;
  fields.lon_e7 = 20;
  fields.pos_age_s = 3;

  uint8_t buffer[32] = {};
  size_t out_len = 0;
  TEST_ASSERT_FALSE(logic.build_tx(500, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);

  TEST_ASSERT_TRUE(logic.build_tx(1000, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(24, out_len);
}

void test_tx_payload_correctness() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1);

  GeoBeaconFields fields{};
  fields.node_id = 0xAABBCCDDEEFF0011ULL;
  fields.pos_valid = 1;
  fields.lat_e7 = 123;
  fields.lon_e7 = -456;
  fields.pos_age_s = 7;

  uint8_t buffer[24] = {};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(logic.build_tx(10, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(24, out_len);

  GeoBeaconFields decoded{};
  const auto err =
      naviga::protocol::decode_geo_beacon(naviga::protocol::ConstByteSpan{buffer, out_len}, &decoded);
  TEST_ASSERT_EQUAL(naviga::protocol::DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT8(fields.pos_valid, decoded.pos_valid);
  TEST_ASSERT_EQUAL_INT32(fields.lat_e7, decoded.lat_e7);
  TEST_ASSERT_EQUAL_INT32(fields.lon_e7, decoded.lon_e7);
  TEST_ASSERT_EQUAL_UINT16(fields.pos_age_s, decoded.pos_age_s);
  TEST_ASSERT_EQUAL_UINT16(1, decoded.seq);

  TEST_ASSERT_TRUE(logic.build_tx(20, fields, buffer, sizeof(buffer), &out_len));
  naviga::protocol::decode_geo_beacon(naviga::protocol::ConstByteSpan{buffer, out_len}, &decoded);
  TEST_ASSERT_EQUAL_UINT16(2, decoded.seq);
}

void test_rx_success_updates_node_table() {
  BeaconLogic logic;
  NodeTable table;
  table.set_expected_interval_s(10);

  GeoBeaconFields fields{};
  fields.node_id = 0x0102030405060708ULL;
  fields.pos_valid = 1;
  fields.lat_e7 = 100;
  fields.lon_e7 = 200;
  fields.pos_age_s = 5;
  fields.seq = 42;

  uint8_t payload[24] = {};
  const size_t written = naviga::protocol::encode_geo_beacon(fields,
                                                             naviga::protocol::ByteSpan{payload, sizeof(payload)});
  TEST_ASSERT_EQUAL_UINT32(24, written);

  TEST_ASSERT_TRUE(logic.on_rx(1000, payload, written, -55, table));
  TEST_ASSERT_EQUAL_UINT32(1, table.size());

  uint8_t page[NodeTable::kRecordBytes * 2] = {};
  const size_t bytes = table.get_page(1000, 0, 10, page, sizeof(page));
  TEST_ASSERT_EQUAL_UINT32(NodeTable::kRecordBytes, bytes);
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, read_u64_le(page));
  TEST_ASSERT_EQUAL_INT8(-55, static_cast<int8_t>(page[23]));
  TEST_ASSERT_EQUAL_UINT16(fields.seq, read_u16_le(page + 24));
}

void test_rx_invalid_payload_no_change() {
  BeaconLogic logic;
  NodeTable table;
  const uint8_t bad_payload[3] = {0x01, 0x01, 0x00};
  TEST_ASSERT_FALSE(logic.on_rx(100, bad_payload, sizeof(bad_payload), -10, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_tx_cadence);
  RUN_TEST(test_tx_payload_correctness);
  RUN_TEST(test_rx_success_updates_node_table);
  RUN_TEST(test_rx_invalid_payload_no_change);
  return UNITY_END();
}
