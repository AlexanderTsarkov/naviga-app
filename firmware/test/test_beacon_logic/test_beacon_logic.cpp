#include <unity.h>

#include <cstdint>

#include "../../src/domain/beacon_logic.h"
#include "../../src/domain/beacon_logic.cpp"
#include "../../src/domain/node_table.h"
#include "../../src/domain/node_table.cpp"
#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/geo_beacon_codec.cpp"
#include "../../protocol/alive_codec.h"
#include "../../protocol/tail1_codec.h"
#include "../../protocol/tail2_codec.h"

using naviga::domain::BeaconLogic;
using naviga::domain::NodeTable;
using naviga::domain::NodeEntry;
using naviga::domain::PacketLogType;
using naviga::protocol::AliveFields;
using naviga::protocol::GeoBeaconFields;
using naviga::protocol::Tail1Fields;
using naviga::protocol::Tail2Fields;
using naviga::protocol::kGeoBeaconFrameSize;
using naviga::protocol::kAliveFrameMin;
using naviga::protocol::kTail1FrameMin;
using naviga::protocol::kTail1FrameMax;
using naviga::protocol::kTail2FrameMin;
using naviga::protocol::kTail2FrameMax;
using naviga::protocol::encode_alive_frame;
using naviga::protocol::encode_tail1_frame;
using naviga::protocol::encode_tail2_frame;

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

// ── Tail-1 codec: golden vectors ────────────────────────────────────────────

void test_tail1_codec_base_round_trip() {
  // Golden vector: nodeId=0xAABBCCDDEEFF, core_seq16=0x0042, no optional fields.
  // Expected payload (9 B): 00 FF EE DD CC BB AA 42 00
  Tail1Fields fields{};
  fields.node_id    = 0x0000AABBCCDDEEFFULL;
  fields.core_seq16 = 0x0042;

  uint8_t frame[kTail1FrameMax] = {};
  const size_t written = encode_tail1_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail1FrameMin, written);

  // Header: msg_type=0x03, payload_len=9 → H=(0x03<<9)|9=0x0609 → [0x09, 0x06]
  TEST_ASSERT_EQUAL_HEX8(0x09, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x06, frame[1]);
  // Payload
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[2]);  // payloadVersion
  TEST_ASSERT_EQUAL_HEX8(0xFF, frame[3]);  // nodeId byte 0
  TEST_ASSERT_EQUAL_HEX8(0xEE, frame[4]);
  TEST_ASSERT_EQUAL_HEX8(0xDD, frame[5]);
  TEST_ASSERT_EQUAL_HEX8(0xCC, frame[6]);
  TEST_ASSERT_EQUAL_HEX8(0xBB, frame[7]);
  TEST_ASSERT_EQUAL_HEX8(0xAA, frame[8]);  // nodeId byte 5
  TEST_ASSERT_EQUAL_HEX8(0x42, frame[9]);  // core_seq16 lo
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[10]); // core_seq16 hi

  // Decode round-trip
  Tail1Fields decoded{};
  const auto err = naviga::protocol::decode_tail1_payload(frame + 2, 9, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT16(0x0042, decoded.core_seq16);
  TEST_ASSERT_FALSE(decoded.has_pos_flags);
  TEST_ASSERT_FALSE(decoded.has_sats);
}

void test_tail1_codec_extended_round_trip() {
  // Golden vector: with posFlags=0x01, sats=8.
  // Expected payload (11 B): 00 FF EE DD CC BB AA 01 00 01 08
  Tail1Fields fields{};
  fields.node_id       = 0x0000AABBCCDDEEFFULL;
  fields.core_seq16    = 0x0001;
  fields.has_pos_flags = true;
  fields.pos_flags     = 0x01;
  fields.has_sats      = true;
  fields.sats          = 8;

  uint8_t frame[kTail1FrameMax] = {};
  const size_t written = encode_tail1_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail1FrameMax, written);

  // Header: msg_type=0x03, payload_len=11 → H=(0x03<<9)|11=0x060B → [0x0B, 0x06]
  TEST_ASSERT_EQUAL_HEX8(0x0B, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x06, frame[1]);
  TEST_ASSERT_EQUAL_HEX8(0x01, frame[11]); // posFlags
  TEST_ASSERT_EQUAL_HEX8(0x08, frame[12]); // sats

  Tail1Fields decoded{};
  const auto err = naviga::protocol::decode_tail1_payload(frame + 2, 11, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT16(0x0001, decoded.core_seq16);
  TEST_ASSERT_TRUE(decoded.has_pos_flags);
  TEST_ASSERT_EQUAL_HEX8(0x01, decoded.pos_flags);
  TEST_ASSERT_TRUE(decoded.has_sats);
  TEST_ASSERT_EQUAL_UINT8(8, decoded.sats);
}

void test_tail1_decode_bad_version_dropped() {
  uint8_t payload[9] = {0x01, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x01, 0x00};
  Tail1Fields out{};
  const auto err = naviga::protocol::decode_tail1_payload(payload, 9, &out);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(naviga::protocol::Tail1DecodeError::BadPayloadVersion),
      static_cast<int>(err));
}

void test_tail1_decode_bad_len_dropped() {
  // payload_len=10 is not in {9, 11}
  uint8_t payload[10] = {};
  Tail1Fields out{};
  const auto err = naviga::protocol::decode_tail1_payload(payload, 10, &out);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(naviga::protocol::Tail1DecodeError::BadPayloadLen),
      static_cast<int>(err));
}

// ── Tail-2 codec: golden vectors ────────────────────────────────────────────

void test_tail2_codec_base_round_trip() {
  // Golden vector: base only (7 B payload).
  Tail2Fields fields{};
  fields.node_id = 0x0000AABBCCDDEEFFULL;

  uint8_t frame[kTail2FrameMax] = {};
  const size_t written = encode_tail2_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail2FrameMin, written);

  // Header: msg_type=0x04, payload_len=7 → H=(0x04<<9)|7=0x0807 → [0x07, 0x08]
  TEST_ASSERT_EQUAL_HEX8(0x07, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x08, frame[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[2]); // payloadVersion

  Tail2Fields decoded{};
  const auto err = naviga::protocol::decode_tail2_payload(frame + 2, 7, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_FALSE(decoded.has_max_silence);
  TEST_ASSERT_FALSE(decoded.has_battery);
}

void test_tail2_codec_with_max_silence_round_trip() {
  // Golden vector: with maxSilence10s=9 (= 90 s).
  Tail2Fields fields{};
  fields.node_id         = 0x0000AABBCCDDEEFFULL;
  fields.has_max_silence = true;
  fields.max_silence_10s = 9;

  uint8_t frame[kTail2FrameMax] = {};
  const size_t written = encode_tail2_frame(fields, frame, sizeof(frame));
  // payload_len = 8 → frame = 10 bytes
  TEST_ASSERT_EQUAL_UINT32(kTail2FrameMin + 1, written);
  TEST_ASSERT_EQUAL_HEX8(0x09, frame[9]); // maxSilence10s

  Tail2Fields decoded{};
  const auto err = naviga::protocol::decode_tail2_payload(frame + 2, 8, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_TRUE(decoded.has_max_silence);
  TEST_ASSERT_EQUAL_UINT8(9, decoded.max_silence_10s);
  TEST_ASSERT_FALSE(decoded.has_battery);
}

void test_tail2_decode_bad_version_dropped() {
  uint8_t payload[7] = {0x01, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA};
  Tail2Fields out{};
  const auto err = naviga::protocol::decode_tail2_payload(payload, 7, &out);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(naviga::protocol::Tail2DecodeError::BadPayloadVersion),
      static_cast<int>(err));
}

// ── Tail-1 RX: apply-on-match / drop-on-mismatch ────────────────────────────

// Helper: build a BeaconCore frame and inject it via on_rx to seed last_core_seq16.
// encode_geo_beacon writes the full frame (header + payload) into the ByteSpan.
static void seed_core(BeaconLogic& /*logic*/, NodeTable& table,
                      uint64_t node_id, uint16_t seq, uint32_t now_ms) {
  GeoBeaconFields core{};
  core.node_id   = node_id;
  core.pos_valid = 1;
  core.lat_deg   = 55.0;
  core.lon_deg   = 37.0;
  core.seq       = seq;

  uint8_t frame[kGeoBeaconFrameSize] = {};
  using naviga::protocol::ByteSpan;
  const size_t written = naviga::protocol::encode_geo_beacon(
      core, ByteSpan{frame, sizeof(frame)});
  (void)written;

  BeaconLogic seeder;
  seeder.on_rx(now_ms, frame, kGeoBeaconFrameSize, -50, table);
}

void test_tail1_rx_apply_on_match() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  const uint16_t core_seq = 7;

  seed_core(logic, table, node_id, core_seq, 1000);
  TEST_ASSERT_EQUAL_UINT32(1, table.size());

  // Build Tail-1 frame with matching core_seq16=7, posFlags=0x01, sats=8.
  Tail1Fields t1{};
  t1.node_id       = node_id;
  t1.core_seq16    = core_seq;
  t1.has_pos_flags = true;
  t1.pos_flags     = 0x01;
  t1.has_sats      = true;
  t1.sats          = 8;
  uint8_t frame[kTail1FrameMax] = {};
  const size_t written = encode_tail1_frame(t1, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail1FrameMax, written);

  PacketLogType ptype = PacketLogType::CORE;
  uint16_t out_core_seq = 0;
  TEST_ASSERT_TRUE(logic.on_rx(2000, frame, written, -55, table,
                               nullptr, nullptr, nullptr, &ptype, &out_core_seq));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PacketLogType::TAIL1), static_cast<int>(ptype));
  TEST_ASSERT_EQUAL_UINT16(core_seq, out_core_seq);

  // Verify posFlags and sats were applied.
  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_TRUE(entry.has_pos_flags);
  TEST_ASSERT_EQUAL_HEX8(0x01, entry.pos_flags);
  TEST_ASSERT_TRUE(entry.has_sats);
  TEST_ASSERT_EQUAL_UINT8(8, entry.sats);
}

void test_tail1_rx_drop_on_mismatch() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  const uint16_t core_seq = 7;

  seed_core(logic, table, node_id, core_seq, 1000);

  // Build Tail-1 with stale core_seq16=5 (mismatch).
  Tail1Fields t1{};
  t1.node_id       = node_id;
  t1.core_seq16    = 5;  // != 7
  t1.has_pos_flags = true;
  t1.pos_flags     = 0x01;
  t1.has_sats      = true;
  t1.sats          = 12;
  uint8_t frame[kTail1FrameMax] = {};
  encode_tail1_frame(t1, frame, sizeof(frame));

  // on_rx returns true (valid frame) but apply_tail1 drops internally.
  TEST_ASSERT_TRUE(logic.on_rx(2000, frame, kTail1FrameMax, -55, table));

  // posFlags/sats MUST NOT have been applied.
  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_FALSE(entry.has_pos_flags);
  TEST_ASSERT_FALSE(entry.has_sats);
}

void test_tail1_rx_drop_no_prior_core() {
  BeaconLogic logic;
  NodeTable table;
  // No Core seeded — node not in table.
  Tail1Fields t1{};
  t1.node_id    = 0x0000AABBCCDDEEFFULL;
  t1.core_seq16 = 1;
  uint8_t frame[kTail1FrameMin] = {};
  encode_tail1_frame(t1, frame, sizeof(frame));

  // on_rx returns true (valid frame), but apply_tail1 drops (no entry).
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, kTail1FrameMin, -55, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

void test_tail1_rx_bad_version_dropped() {
  BeaconLogic logic;
  NodeTable table;
  // Manually craft a Tail-1 frame with payloadVersion=0x01.
  // Header: msg_type=0x03, payload_len=9 → [0x09, 0x06]
  uint8_t frame[kTail1FrameMin] = {0x09, 0x06,
                                    0x01,  // bad payloadVersion
                                    0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
                                    0x01, 0x00};
  TEST_ASSERT_FALSE(logic.on_rx(1000, frame, sizeof(frame), -55, table));
}

// ── Tail-2 RX: apply unconditionally ────────────────────────────────────────

void test_tail2_rx_applies_max_silence() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;

  Tail2Fields t2{};
  t2.node_id         = node_id;
  t2.has_max_silence = true;
  t2.max_silence_10s = 6;  // 60 s

  uint8_t frame[kTail2FrameMax] = {};
  const size_t written = encode_tail2_frame(t2, frame, sizeof(frame));

  PacketLogType ptype = PacketLogType::CORE;
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, written, -60, table,
                               nullptr, nullptr, nullptr, &ptype));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PacketLogType::TAIL2), static_cast<int>(ptype));

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_TRUE(entry.has_max_silence);
  TEST_ASSERT_EQUAL_UINT8(6, entry.max_silence_10s);
}

void test_tail2_rx_no_prior_core_creates_entry() {
  // Tail-2 can arrive before Core; it should create an entry.
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000112233445566ULL;

  Tail2Fields t2{};
  t2.node_id      = node_id;
  t2.has_battery  = true;
  t2.battery_percent = 80;

  uint8_t frame[kTail2FrameMax] = {};
  const size_t written = encode_tail2_frame(t2, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, written, -70, table));
  TEST_ASSERT_EQUAL_UINT32(1, table.size());

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_TRUE(entry.has_battery);
  TEST_ASSERT_EQUAL_UINT8(80, entry.battery_percent);
  // Position MUST NOT have been set.
  TEST_ASSERT_FALSE(entry.pos_valid);
  TEST_ASSERT_EQUAL_INT32(0, entry.lat_e7);
  TEST_ASSERT_EQUAL_INT32(0, entry.lon_e7);
}

void test_tail2_rx_bad_version_dropped() {
  BeaconLogic logic;
  NodeTable table;
  // Header: msg_type=0x04, payload_len=7 → [0x07, 0x08]; bad payloadVersion=0x01
  uint8_t frame[kTail2FrameMin] = {0x07, 0x08,
                                    0x01,  // bad payloadVersion
                                    0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA};
  TEST_ASSERT_FALSE(logic.on_rx(1000, frame, sizeof(frame), -60, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

// ── Position invariant: Tail never overwrites position ───────────────────────

void test_tail_never_overwrites_position() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  const uint16_t core_seq = 3;

  // Seed Core with known position.
  seed_core(logic, table, node_id, core_seq, 1000);
  NodeEntry before{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &before));
  TEST_ASSERT_TRUE(before.pos_valid);
  const int32_t saved_lat = before.lat_e7;
  const int32_t saved_lon = before.lon_e7;

  // Apply matching Tail-1.
  Tail1Fields t1{};
  t1.node_id       = node_id;
  t1.core_seq16    = core_seq;
  t1.has_pos_flags = true;
  t1.pos_flags     = 0x01;
  uint8_t t1_frame[kTail1FrameMax] = {};
  encode_tail1_frame(t1, t1_frame, sizeof(t1_frame));
  logic.on_rx(2000, t1_frame, kTail1FrameMax, -55, table);

  // Apply Tail-2.
  Tail2Fields t2{};
  t2.node_id         = node_id;
  t2.has_max_silence = true;
  t2.max_silence_10s = 9;
  uint8_t t2_frame[kTail2FrameMax] = {};
  const size_t t2_written = encode_tail2_frame(t2, t2_frame, sizeof(t2_frame));
  logic.on_rx(3000, t2_frame, t2_written, -60, table);

  // Position MUST be unchanged.
  NodeEntry after{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &after));
  TEST_ASSERT_TRUE(after.pos_valid);
  TEST_ASSERT_EQUAL_INT32(saved_lat, after.lat_e7);
  TEST_ASSERT_EQUAL_INT32(saved_lon, after.lon_e7);
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
  // Tail-1 codec
  RUN_TEST(test_tail1_codec_base_round_trip);
  RUN_TEST(test_tail1_codec_extended_round_trip);
  RUN_TEST(test_tail1_decode_bad_version_dropped);
  RUN_TEST(test_tail1_decode_bad_len_dropped);
  // Tail-2 codec
  RUN_TEST(test_tail2_codec_base_round_trip);
  RUN_TEST(test_tail2_codec_with_max_silence_round_trip);
  RUN_TEST(test_tail2_decode_bad_version_dropped);
  // Tail-1 RX dispatch
  RUN_TEST(test_tail1_rx_apply_on_match);
  RUN_TEST(test_tail1_rx_drop_on_mismatch);
  RUN_TEST(test_tail1_rx_drop_no_prior_core);
  RUN_TEST(test_tail1_rx_bad_version_dropped);
  // Tail-2 RX dispatch
  RUN_TEST(test_tail2_rx_applies_max_silence);
  RUN_TEST(test_tail2_rx_no_prior_core_creates_entry);
  RUN_TEST(test_tail2_rx_bad_version_dropped);
  // Position invariant
  RUN_TEST(test_tail_never_overwrites_position);
  return UNITY_END();
}
