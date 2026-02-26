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
using naviga::domain::PacketLogType;
using naviga::protocol::GeoBeaconFields;
using naviga::protocol::kGeoBeaconSize;

namespace {

uint16_t read_u16_le(const uint8_t* data) {
  return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
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
  fields.node_id = 0x0000334455667788ULL;
  fields.pos_valid = 1;
  fields.lat_deg = 55.7558;
  fields.lon_deg = 37.6173;

  uint8_t buffer[32] = {};
  size_t out_len = 0;
  TEST_ASSERT_FALSE(logic.build_tx(500, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);

  TEST_ASSERT_TRUE(logic.build_tx(1000, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, out_len);
}

// Role-derived cadence (#281): max_silence_ms forces TX when min_interval is not yet reached.
// With fix: send CORE; without fix: send ALIVE (per field_cadence_v0).
void test_tx_max_silence_triggers_send() {
  BeaconLogic logic;
  logic.set_min_interval_ms(60000);   // 60s — not reached in this test
  logic.set_max_silence_ms(10000);    // 10s — must send by then

  GeoBeaconFields fields{};
  fields.node_id = 1;
  fields.pos_valid = 1;
  fields.lat_deg = 0.0;
  fields.lon_deg = 0.0;

  uint8_t buffer[32] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::CORE;
  TEST_ASSERT_FALSE(logic.build_tx(5000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);

  TEST_ASSERT_TRUE(logic.build_tx(10000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, out_len);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::CORE), static_cast<int>(ptype));  // fix → CORE

  // No fix at max_silence → ALIVE
  fields.pos_valid = 0;
  TEST_ASSERT_TRUE(logic.build_tx(20000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::ALIVE), static_cast<int>(ptype));
}

// minDisplacement gating: at min_interval without position update → NO_SEND.
void test_tx_min_interval_no_update_no_send() {
  BeaconLogic logic;
  logic.set_min_interval_ms(5000);
  logic.set_max_silence_ms(60000);

  GeoBeaconFields fields{};
  fields.node_id = 1;
  fields.pos_valid = 1;
  fields.lat_deg = 0.0001;
  fields.lon_deg = 0.0002;

  uint8_t buffer[32] = {};
  size_t out_len = 1;
  TEST_ASSERT_FALSE(logic.build_tx(5000, fields, buffer, sizeof(buffer), &out_len, nullptr, nullptr, false));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);
}

void test_tx_payload_correctness() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1);

  GeoBeaconFields fields{};
  // NodeID48: upper 16 bits = 0x0000 for round-trip equality.
  fields.node_id = 0x0000CCDDEEFF0011ULL;
  fields.pos_valid = 1;
  fields.lat_deg = 55.7558;
  fields.lon_deg = 37.6173;

  uint8_t buffer[kGeoBeaconSize] = {};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(logic.build_tx(10, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, out_len);

  GeoBeaconFields decoded{};
  const auto err =
      naviga::protocol::decode_geo_beacon(naviga::protocol::ConstByteSpan{buffer, out_len}, &decoded);
  TEST_ASSERT_EQUAL(naviga::protocol::DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT8(1, decoded.pos_valid);
  // Packed24 precision tolerance: ~0.00003 degrees
  TEST_ASSERT_TRUE(decoded.lat_deg > 55.7557 && decoded.lat_deg < 55.7559);
  TEST_ASSERT_TRUE(decoded.lon_deg > 37.6172 && decoded.lon_deg < 37.6174);
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
  // NodeID48: upper 16 bits = 0x0000.
  fields.node_id = 0x0000030405060708ULL;
  fields.pos_valid = 1;
  fields.lat_deg = 55.7558;
  fields.lon_deg = 37.6173;
  fields.seq = 42;

  uint8_t payload[kGeoBeaconSize] = {};
  const size_t written = naviga::protocol::encode_geo_beacon(fields,
                                                             naviga::protocol::ByteSpan{payload, sizeof(payload)});
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconSize, written);

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
  RUN_TEST(test_tx_max_silence_triggers_send);
  RUN_TEST(test_tx_min_interval_no_update_no_send);
  RUN_TEST(test_tx_payload_correctness);
  RUN_TEST(test_rx_success_updates_node_table);
  RUN_TEST(test_rx_invalid_payload_no_change);
  return UNITY_END();
}
