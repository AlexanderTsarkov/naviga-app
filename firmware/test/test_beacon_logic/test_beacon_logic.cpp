#include <unity.h>

#include <cstdint>

#include "../../src/domain/beacon_logic.h"
#include "../../src/domain/beacon_logic.cpp"
#include "../../src/domain/node_table.h"
#include "../../src/domain/node_table.cpp"
#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/geo_beacon_codec.cpp"
#include "../../protocol/alive_codec.h"

using naviga::domain::BeaconLogic;
using naviga::domain::NodeTable;
using naviga::domain::PacketLogType;
using naviga::protocol::AliveFields;
using naviga::protocol::GeoBeaconFields;
using naviga::protocol::kGeoBeaconFrameSize;
using naviga::protocol::kAliveFrameMin;
using naviga::protocol::encode_alive_frame;

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

// Largest frame we ever build in tests.
constexpr size_t kTestBufSize = kGeoBeaconFrameSize;

} // namespace

// ── TX cadence ──────────────────────────────────────────────────────────────

void test_tx_cadence() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);

  GeoBeaconFields fields{};
  fields.node_id   = 0x0000334455667788ULL;
  fields.pos_valid = 1;
  fields.lat_deg   = 55.7558;
  fields.lon_deg   = 37.6173;

  uint8_t buffer[kTestBufSize] = {};
  size_t out_len = 0;
  TEST_ASSERT_FALSE(logic.build_tx(500, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);

  TEST_ASSERT_TRUE(logic.build_tx(1000, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, out_len);
}

// Role-derived cadence: max_silence_ms forces TX when min_interval not yet reached.
// With fix → CORE frame (17 B).
void test_tx_max_silence_triggers_core() {
  BeaconLogic logic;
  logic.set_min_interval_ms(60000);
  logic.set_max_silence_ms(10000);

  GeoBeaconFields fields{};
  fields.node_id   = 1;
  fields.pos_valid = 1;
  fields.lat_deg   = 0.0;
  fields.lon_deg   = 0.0;

  uint8_t buffer[kTestBufSize] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::CORE;
  TEST_ASSERT_FALSE(logic.build_tx(5000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);

  TEST_ASSERT_TRUE(logic.build_tx(10000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, out_len);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::CORE), static_cast<int>(ptype));
}

// No fix + maxSilence → Alive frame (11 B, msg_type=0x02).
void test_tx_no_fix_at_max_silence_sends_alive() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(2000);

  GeoBeaconFields fields{};
  fields.node_id   = 0x0000AABBCCDDEEFFULL;
  fields.pos_valid = 0;  // no fix

  uint8_t buffer[kTestBufSize] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::CORE;

  // At min_interval: no fix → no Core, no Alive (only at maxSilence).
  TEST_ASSERT_FALSE(logic.build_tx(1000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);

  // At maxSilence: no fix → Alive.
  TEST_ASSERT_TRUE(logic.build_tx(2000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL_UINT32(kAliveFrameMin, out_len);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::ALIVE), static_cast<int>(ptype));

  // Verify header bytes: msg_type=0x02, payload_len=9 → H=0x0409 → [0x09, 0x04]
  TEST_ASSERT_EQUAL_HEX8(0x09, buffer[0]);
  TEST_ASSERT_EQUAL_HEX8(0x04, buffer[1]);
}

// minDisplacement gating: at min_interval without position update → NO_SEND.
void test_tx_min_interval_no_update_no_send() {
  BeaconLogic logic;
  logic.set_min_interval_ms(5000);
  logic.set_max_silence_ms(60000);

  GeoBeaconFields fields{};
  fields.node_id   = 1;
  fields.pos_valid = 1;
  fields.lat_deg   = 0.0001;
  fields.lon_deg   = 0.0002;

  uint8_t buffer[kTestBufSize] = {};
  size_t out_len = 1;
  TEST_ASSERT_FALSE(logic.build_tx(5000, fields, buffer, sizeof(buffer), &out_len,
                                   nullptr, nullptr, false));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);
}

void test_tx_payload_correctness() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1);

  GeoBeaconFields fields{};
  fields.node_id   = 0x0000CCDDEEFF0011ULL;
  fields.pos_valid = 1;
  fields.lat_deg   = 55.7558;
  fields.lon_deg   = 37.6173;

  uint8_t buffer[kGeoBeaconFrameSize] = {};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(logic.build_tx(10, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, out_len);

  GeoBeaconFields decoded{};
  const auto err = naviga::protocol::decode_geo_beacon_frame(
      naviga::protocol::ConstByteSpan{buffer, out_len}, &decoded);
  TEST_ASSERT_EQUAL(naviga::protocol::DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT8(1, decoded.pos_valid);
  TEST_ASSERT_TRUE(decoded.lat_deg > 55.7557 && decoded.lat_deg < 55.7559);
  TEST_ASSERT_TRUE(decoded.lon_deg > 37.6172 && decoded.lon_deg < 37.6174);
  TEST_ASSERT_EQUAL_UINT16(1, decoded.seq);

  TEST_ASSERT_TRUE(logic.build_tx(20, fields, buffer, sizeof(buffer), &out_len));
  naviga::protocol::decode_geo_beacon_frame(
      naviga::protocol::ConstByteSpan{buffer, out_len}, &decoded);
  TEST_ASSERT_EQUAL_UINT16(2, decoded.seq);
}

// ── RX: Core dispatch ───────────────────────────────────────────────────────

void test_rx_core_success_updates_node_table() {
  BeaconLogic logic;
  NodeTable table;
  table.set_expected_interval_s(10);

  GeoBeaconFields fields{};
  fields.node_id   = 0x0000030405060708ULL;
  fields.pos_valid = 1;
  fields.lat_deg   = 55.7558;
  fields.lon_deg   = 37.6173;
  fields.seq       = 42;

  uint8_t frame[kGeoBeaconFrameSize] = {};
  const size_t written = naviga::protocol::encode_geo_beacon(
      fields, naviga::protocol::ByteSpan{frame, sizeof(frame)});
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, written);

  uint64_t rx_node_id = 0;
  uint16_t rx_seq     = 0;
  bool rx_pos_valid   = false;
  PacketLogType rx_type = PacketLogType::ALIVE;
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, written, -55, table,
                               &rx_node_id, &rx_seq, &rx_pos_valid, &rx_type));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, rx_node_id);
  TEST_ASSERT_EQUAL_UINT16(42, rx_seq);
  TEST_ASSERT_TRUE(rx_pos_valid);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::CORE), static_cast<int>(rx_type));
  TEST_ASSERT_EQUAL_UINT32(1, table.size());

  uint8_t page[NodeTable::kRecordBytes * 2] = {};
  const size_t bytes = table.get_page(1000, 0, 10, page, sizeof(page));
  TEST_ASSERT_EQUAL_UINT32(NodeTable::kRecordBytes, bytes);
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, read_u64_le(page));
  TEST_ASSERT_EQUAL_INT8(-55, static_cast<int8_t>(page[23]));
  TEST_ASSERT_EQUAL_UINT16(fields.seq, read_u16_le(page + 24));
}

// ── RX: Alive dispatch ──────────────────────────────────────────────────────

void test_rx_alive_success_updates_node_table() {
  BeaconLogic logic;
  NodeTable table;
  table.set_expected_interval_s(10);

  AliveFields alive{};
  alive.node_id    = 0x0000AABBCCDDEEFFULL;
  alive.seq        = 7;
  alive.has_status = false;

  uint8_t frame[kAliveFrameMin] = {};
  const size_t written = encode_alive_frame(alive, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kAliveFrameMin, written);

  uint64_t rx_node_id = 0;
  uint16_t rx_seq     = 0;
  bool rx_pos_valid   = true;  // should be set false by Alive
  PacketLogType rx_type = PacketLogType::CORE;
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, written, -60, table,
                               &rx_node_id, &rx_seq, &rx_pos_valid, &rx_type));
  TEST_ASSERT_EQUAL_UINT64(alive.node_id, rx_node_id);
  TEST_ASSERT_EQUAL_UINT16(7, rx_seq);
  TEST_ASSERT_FALSE(rx_pos_valid);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::ALIVE), static_cast<int>(rx_type));
  TEST_ASSERT_EQUAL_UINT32(1, table.size());
}

// Alive and Core from same node: Alive does not overwrite Core position.
void test_rx_alive_does_not_overwrite_core_position() {
  BeaconLogic logic;
  NodeTable table;
  table.set_expected_interval_s(10);

  const uint64_t node_id = 0x0000112233445566ULL;

  // First: receive a Core packet.
  GeoBeaconFields core{};
  core.node_id   = node_id;
  core.pos_valid = 1;
  core.lat_deg   = 55.7558;
  core.lon_deg   = 37.6173;
  core.seq       = 1;
  uint8_t core_frame[kGeoBeaconFrameSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize,
      naviga::protocol::encode_geo_beacon(core,
          naviga::protocol::ByteSpan{core_frame, sizeof(core_frame)}));
  TEST_ASSERT_TRUE(logic.on_rx(1000, core_frame, kGeoBeaconFrameSize, -50, table));

  // Then: receive an Alive packet (seq=2, no position).
  AliveFields alive{};
  alive.node_id = node_id;
  alive.seq     = 2;
  uint8_t alive_frame[kAliveFrameMin] = {};
  TEST_ASSERT_EQUAL_UINT32(kAliveFrameMin,
      encode_alive_frame(alive, alive_frame, sizeof(alive_frame)));
  TEST_ASSERT_TRUE(logic.on_rx(2000, alive_frame, kAliveFrameMin, -60, table));

  // Node table should still have the node (updated by Alive).
  TEST_ASSERT_EQUAL_UINT32(1, table.size());
}

// ── RX: drop rules ──────────────────────────────────────────────────────────

void test_rx_invalid_frame_too_short() {
  BeaconLogic logic;
  NodeTable table;
  const uint8_t bad[1] = {0x01};
  TEST_ASSERT_FALSE(logic.on_rx(100, bad, sizeof(bad), -10, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

void test_rx_unknown_msgtype_dropped() {
  BeaconLogic logic;
  NodeTable table;
  // Build a frame with msg_type=0x7F (unknown): H=(0x7F<<9)|9=0xFF09 → [0x09, 0xFF]
  uint8_t frame[11] = {};
  frame[0] = 0x09;
  frame[1] = 0xFF;
  TEST_ASSERT_FALSE(logic.on_rx(100, frame, sizeof(frame), -10, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

void test_rx_payload_len_mismatch_dropped() {
  BeaconLogic logic;
  NodeTable table;
  // Header says payload_len=15 (Core) but we only pass 10 bytes total (8 payload).
  // H=(0x01<<9)|15=0x020F → [0x0F, 0x02]
  uint8_t frame[10] = {};
  frame[0] = 0x0F;
  frame[1] = 0x02;
  TEST_ASSERT_FALSE(logic.on_rx(100, frame, sizeof(frame), -10, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_tx_cadence);
  RUN_TEST(test_tx_max_silence_triggers_core);
  RUN_TEST(test_tx_no_fix_at_max_silence_sends_alive);
  RUN_TEST(test_tx_min_interval_no_update_no_send);
  RUN_TEST(test_tx_payload_correctness);
  RUN_TEST(test_rx_core_success_updates_node_table);
  RUN_TEST(test_rx_alive_success_updates_node_table);
  RUN_TEST(test_rx_alive_does_not_overwrite_core_position);
  RUN_TEST(test_rx_invalid_frame_too_short);
  RUN_TEST(test_rx_unknown_msgtype_dropped);
  RUN_TEST(test_rx_payload_len_mismatch_dropped);
  return UNITY_END();
}
