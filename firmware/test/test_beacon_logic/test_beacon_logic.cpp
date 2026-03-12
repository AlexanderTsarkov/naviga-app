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
#include "../../protocol/info_codec.h"
#include "../../protocol/pos_full_codec.h"
#include "../../protocol/pos_full_codec.cpp"
#include "../../protocol/status_codec.h"
#include "../../protocol/status_codec.cpp"

using naviga::domain::BeaconLogic;
using naviga::domain::NodeTable;
using naviga::domain::NodeEntry;
using naviga::domain::PacketLogType;
using naviga::domain::SelfTelemetry;
using naviga::domain::TxPriority;
using naviga::domain::TxBestEffortClass;
using naviga::domain::kSlotPosFull;
using naviga::domain::kSlotAlive;
using naviga::domain::kSlotStatus;
using naviga::protocol::AliveFields;
using naviga::protocol::GeoBeaconFields;
using naviga::protocol::Tail1Fields;
using naviga::protocol::Tail2Fields;
using naviga::protocol::InfoFields;
using naviga::protocol::kGeoBeaconFrameSize;
using naviga::protocol::kAliveFrameMin;
using naviga::protocol::kTail1FrameMin;
using naviga::protocol::kTail1FrameMax;
using naviga::protocol::kTail2FrameMin;
using naviga::protocol::kTail2FrameMax;
using naviga::protocol::kInfoFrameMin;
using naviga::protocol::kInfoFrameMax;
using naviga::protocol::kPosFullFrameSize;
using naviga::protocol::kStatusFrameSize;
using naviga::protocol::encode_alive_frame;
using naviga::protocol::encode_tail1_frame;
using naviga::protocol::encode_tail2_frame;
using naviga::protocol::encode_info_frame;

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
constexpr size_t kTestBufSize = 32;

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
  TEST_ASSERT_EQUAL_UINT32(kPosFullFrameSize, out_len);
}

// #417: Default start — first packet carries seq16 = 1 (canon: fresh start).
void test_seq16_default_first_packet_is_one() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  GeoBeaconFields fields{};
  fields.node_id   = 0x0000334455667788ULL;
  fields.pos_valid = 1;
  fields.lat_deg   = 55.7558;
  fields.lon_deg   = 37.6173;
  uint8_t buffer[kTestBufSize] = {};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(logic.build_tx(1000, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(kPosFullFrameSize, out_len);
  // Seq16 at payload offset 7-8; frame = header(2) + payload.
  TEST_ASSERT_EQUAL_UINT16(1, read_u16_le(buffer + 2 + 7));
}

// #417: set_initial_seq16(restored) → first packet uses restored + 1 (canon rx_semantics_v0 §5.3).
void test_seq16_set_initial_then_first_packet_is_initial_plus_one() {
  BeaconLogic logic;
  logic.set_initial_seq16(42);
  logic.set_min_interval_ms(1000);
  GeoBeaconFields fields{};
  fields.node_id   = 0x0000334455667788ULL;
  fields.pos_valid = 1;
  fields.lat_deg   = 55.7558;
  fields.lon_deg   = 37.6173;
  uint8_t buffer[kTestBufSize] = {};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(logic.build_tx(1000, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(kPosFullFrameSize, out_len);
  TEST_ASSERT_EQUAL_UINT16(43, read_u16_le(buffer + 2 + 7));
}

// #417: set_initial_seq16(100) → first next_seq16() returns 101 (via encoded packet).
void test_seq16_set_initial_100_first_packet_is_101() {
  BeaconLogic logic;
  logic.set_initial_seq16(100);
  logic.set_min_interval_ms(1000);
  GeoBeaconFields fields{};
  fields.node_id   = 1;
  fields.pos_valid = 1;
  fields.lat_deg   = 0.0;
  fields.lon_deg   = 0.0;
  uint8_t buffer[kTestBufSize] = {};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(logic.build_tx(1000, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT16(101, read_u16_le(buffer + 2 + 7));
}

// #417 wraparound: set_initial_seq16(65535) → first packet seq16 == 0 (valid; must be persisted).
void test_seq16_wraparound_first_packet_is_zero() {
  BeaconLogic logic;
  logic.set_initial_seq16(65535);
  logic.set_min_interval_ms(1000);
  GeoBeaconFields fields{};
  fields.node_id   = 1;
  fields.pos_valid = 1;
  fields.lat_deg   = 0.0;
  fields.lon_deg   = 0.0;
  uint8_t buffer[kTestBufSize] = {};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(logic.build_tx(1000, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT16(0, read_u16_le(buffer + 2 + 7));
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
  PacketLogType ptype = PacketLogType::POS_FULL;
  TEST_ASSERT_FALSE(logic.build_tx(5000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);

  TEST_ASSERT_TRUE(logic.build_tx(10000, fields, buffer, sizeof(buffer), &out_len, &ptype));
  TEST_ASSERT_EQUAL_UINT32(kPosFullFrameSize, out_len);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::POS_FULL), static_cast<int>(ptype));
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
  PacketLogType ptype = PacketLogType::POS_FULL;

  // At min_interval: no fix → no PosFull, no Alive (only at maxSilence).
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

  uint8_t buffer[kPosFullFrameSize] = {};
  size_t out_len = 0;
  TEST_ASSERT_TRUE(logic.build_tx(10, fields, buffer, sizeof(buffer), &out_len));
  TEST_ASSERT_EQUAL_UINT32(kPosFullFrameSize, out_len);

  naviga::protocol::PosFullFields decoded{};
  const auto err = naviga::protocol::decode_pos_full_payload(
      buffer + 2, out_len - 2, &decoded);
  TEST_ASSERT_EQUAL(naviga::protocol::PosFullDecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_TRUE(decoded.lat_deg > 55.7557 && decoded.lat_deg < 55.7559);
  TEST_ASSERT_TRUE(decoded.lon_deg > 37.6172 && decoded.lon_deg < 37.6174);
  TEST_ASSERT_EQUAL_UINT16(1, decoded.seq16);

  TEST_ASSERT_TRUE(logic.build_tx(20, fields, buffer, sizeof(buffer), &out_len));
  naviga::protocol::decode_pos_full_payload(buffer + 2, out_len - 2, &decoded);
  TEST_ASSERT_EQUAL_UINT16(2, decoded.seq16);
}

// ── RX: v0.2-only; v0.1 packets dropped (#438) ───────────────────────────────

void test_rx_v01_packets_dropped() {
  BeaconLogic logic;
  NodeTable table;
  // Minimal valid-looking frames for 0x01, 0x03, 0x04, 0x05: header + payload.
  uint8_t core_frame[2 + 15] = {0x0F, 0x02, 0x00};
  for (size_t i = 3; i < sizeof(core_frame); ++i) core_frame[i] = 0;
  TEST_ASSERT_FALSE(logic.on_rx(1000, core_frame, sizeof(core_frame), -50, table));
  uint8_t tail1_frame[2 + 11] = {0x0B, 0x06};
  for (size_t i = 2; i < sizeof(tail1_frame); ++i) tail1_frame[i] = 0;
  TEST_ASSERT_FALSE(logic.on_rx(1000, tail1_frame, sizeof(tail1_frame), -50, table));
  uint8_t tail2_frame[2 + 9] = {0x09, 0x08};
  for (size_t i = 2; i < sizeof(tail2_frame); ++i) tail2_frame[i] = 0;
  TEST_ASSERT_FALSE(logic.on_rx(1000, tail2_frame, sizeof(tail2_frame), -50, table));
  uint8_t info_frame[2 + 9] = {0x09, 0x0A};
  for (size_t i = 2; i < sizeof(info_frame); ++i) info_frame[i] = 0;
  TEST_ASSERT_FALSE(logic.on_rx(1000, info_frame, sizeof(info_frame), -50, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

// ── RX: v0.2 Node_Pos_Full (0x06) and Node_Status (0x07) (#435) ───────────────

void test_rx_pos_full_applies_position_and_quality() {
  BeaconLogic logic;
  NodeTable table;
  naviga::protocol::PosFullFields pos{};
  pos.node_id = 0x0000112233445566ULL;
  pos.seq16 = 3;
  pos.lat_deg = 55.0;
  pos.lon_deg = 37.0;
  pos.fix_type = 2;
  pos.pos_sats = 10;
  pos.pos_accuracy_bucket = 4;
  pos.pos_flags_small = 1;
  uint8_t frame[naviga::protocol::kPosFullFrameSize] = {};
  const size_t written = naviga::protocol::encode_pos_full_frame(pos, frame, sizeof(frame));
  TEST_ASSERT_EQUAL(naviga::protocol::kPosFullFrameSize, written);

  PacketLogType rx_type = PacketLogType::ALIVE;
  TEST_ASSERT_TRUE(logic.on_rx(2000, frame, written, -60, table,
                               nullptr, nullptr, nullptr, &rx_type));
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::POS_FULL), static_cast<int>(rx_type));

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(pos.node_id, &entry));
  TEST_ASSERT_TRUE(entry.pos_valid);
  TEST_ASSERT_EQUAL_UINT16(3, entry.last_seq);
  TEST_ASSERT_EQUAL_UINT16(3, entry.last_core_seq16);
  TEST_ASSERT_TRUE(entry.has_pos_flags);
  TEST_ASSERT_TRUE(entry.has_sats);
  // Pos_Quality → NodeTable packing: pos_flags = pos_flags_small[0:3] | fix_type[4:6] | pos_accuracy_bucket bit0[7];
  // sats = pos_sats[0:5] | (pos_accuracy_bucket bits 1–2)[6:7]. (fix_type=2, pos_sats=10, pos_accuracy_bucket=4, pos_flags_small=1)
  TEST_ASSERT_EQUAL_UINT8(33, entry.pos_flags);   // 1 | (2<<4) | 0
  TEST_ASSERT_EQUAL_UINT8(138, entry.sats);      // 10 | ((4 & 0x06u) << 5)
}

void test_rx_status_applies_full_snapshot() {
  BeaconLogic logic;
  NodeTable table;
  naviga::protocol::StatusFields st{};
  st.node_id = 0x0000AABBCCDDEE11ULL;
  st.seq16 = 5;
  st.battery_percent = 75;
  st.uptime10m = 10;
  st.max_silence_10s = 6;
  st.hw_profile_id = 0x0201;
  st.fw_version_id = 0x0403;
  uint8_t frame[naviga::protocol::kStatusFrameSize] = {};
  const size_t written = naviga::protocol::encode_status_frame(st, frame, sizeof(frame));
  TEST_ASSERT_EQUAL(naviga::protocol::kStatusFrameSize, written);

  PacketLogType rx_type = PacketLogType::ALIVE;
  TEST_ASSERT_TRUE(logic.on_rx(3000, frame, written, -50, table,
                               nullptr, nullptr, nullptr, &rx_type));
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS), static_cast<int>(rx_type));

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(st.node_id, &entry));
  TEST_ASSERT_EQUAL_UINT16(5, entry.last_seq);
  TEST_ASSERT_TRUE(entry.has_battery);
  TEST_ASSERT_EQUAL_UINT8(75, entry.battery_percent);
  TEST_ASSERT_TRUE(entry.has_uptime);
  TEST_ASSERT_EQUAL_UINT32(6000u, entry.uptime_sec);  // 10 * 600
  TEST_ASSERT_TRUE(entry.has_max_silence);
  TEST_ASSERT_EQUAL_UINT8(6, entry.max_silence_10s);
  TEST_ASSERT_EQUAL_UINT16(0x0201, entry.hw_profile_id);
  TEST_ASSERT_EQUAL_UINT16(0x0403, entry.fw_version_id);
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

// Alive and Pos_Full from same node: Alive does not overwrite position.
void test_rx_alive_does_not_overwrite_core_position() {
  BeaconLogic logic;
  NodeTable table;
  table.set_expected_interval_s(10);

  const uint64_t node_id = 0x0000112233445566ULL;

  // First: receive Node_Pos_Full (v0.2) with round coordinates so encode/decode is exact.
  naviga::protocol::PosFullFields pos{};
  pos.node_id = node_id;
  pos.seq16   = 1;
  pos.lat_deg = 55.0;
  pos.lon_deg = 37.0;
  pos.fix_type = 1;
  pos.pos_sats = 0;
  pos.pos_accuracy_bucket = 0;
  pos.pos_flags_small = 0;
  uint8_t pos_frame[naviga::protocol::kPosFullFrameSize] = {};
  const size_t pos_len = naviga::protocol::encode_pos_full_frame(pos, pos_frame, sizeof(pos_frame));
  TEST_ASSERT_EQUAL(naviga::protocol::kPosFullFrameSize, pos_len);
  TEST_ASSERT_TRUE(logic.on_rx(1000, pos_frame, pos_len, -50, table));

  // Then: receive an Alive packet (seq=2, no position).
  AliveFields alive_f{};
  alive_f.node_id = node_id;
  alive_f.seq     = 2;
  uint8_t alive_frame[kAliveFrameMin] = {};
  TEST_ASSERT_EQUAL_UINT32(kAliveFrameMin,
      encode_alive_frame(alive_f, alive_frame, sizeof(alive_frame)));
  TEST_ASSERT_TRUE(logic.on_rx(2000, alive_frame, kAliveFrameMin, -60, table));

  // Node table should still have the node; position must not be overwritten by Alive.
  TEST_ASSERT_EQUAL_UINT32(1, table.size());
  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_TRUE(entry.pos_valid);
  // Allow small rounding from pos_full encode/decode (55.0/37.0 → lat_e7/lon_e7).
  TEST_ASSERT_INT_WITHIN(100, 550000000, entry.lat_e7);
  TEST_ASSERT_INT_WITHIN(100, 370000000, entry.lon_e7);
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

// ── decode_header: v0.2-only; v0.1 msg_types rejected (#438) ─────────────────

void test_decode_header_rejects_v01_msgtypes() {
  naviga::protocol::PacketHeader hdr{};
  // 0x01 Core
  uint8_t f01[2] = {0x0F, 0x02};
  TEST_ASSERT_FALSE(naviga::protocol::decode_header(f01, 2, &hdr));
  // 0x03 Tail1
  uint8_t f03[2] = {0x0B, 0x06};
  TEST_ASSERT_FALSE(naviga::protocol::decode_header(f03, 2, &hdr));
  // 0x04 Tail2
  uint8_t f04[2] = {0x09, 0x08};
  TEST_ASSERT_FALSE(naviga::protocol::decode_header(f04, 2, &hdr));
  // 0x05 Info
  uint8_t f05[2] = {0x09, 0x0A};
  TEST_ASSERT_FALSE(naviga::protocol::decode_header(f05, 2, &hdr));
}

void test_decode_header_accepts_0x06() {
  // msg_type=0x06 (BeaconPosFull, v0.2): H=(0x06<<9)|9=0x0C09 → [0x09, 0x0C]
  uint8_t frame[2] = {0x09, 0x0C};
  naviga::protocol::PacketHeader hdr{};
  TEST_ASSERT_TRUE(naviga::protocol::decode_header(frame, sizeof(frame), &hdr));
  TEST_ASSERT_EQUAL(static_cast<int>(naviga::protocol::MsgType::BeaconPosFull),
                    static_cast<int>(hdr.msg_type));
}

// ── Tail-1 codec: golden vectors ────────────────────────────────────────────
// New layout: Common(9B) + ref_core_seq16(2B) [+ posFlags + sats]
// kTail1FrameMin = 13 B, kTail1FrameMax = 15 B

void test_tail1_codec_base_round_trip() {
  // Golden vector: nodeId=0xAABBCCDDEEFF, seq16=0x0010, ref_core_seq16=0x0042, no optional.
  // Expected payload (11 B): 00 FF EE DD CC BB AA 10 00 42 00
  Tail1Fields fields{};
  fields.node_id         = 0x0000AABBCCDDEEFFULL;
  fields.seq16           = 0x0010;
  fields.ref_core_seq16  = 0x0042;

  uint8_t frame[kTail1FrameMax] = {};
  const size_t written = encode_tail1_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail1FrameMin, written);

  // Header: msg_type=0x03, payload_len=11 → H=(0x03<<9)|11=0x060B → [0x0B, 0x06]
  TEST_ASSERT_EQUAL_HEX8(0x0B, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x06, frame[1]);
  // Payload
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[2]);  // payloadVersion
  TEST_ASSERT_EQUAL_HEX8(0xFF, frame[3]);  // nodeId byte 0
  TEST_ASSERT_EQUAL_HEX8(0xEE, frame[4]);
  TEST_ASSERT_EQUAL_HEX8(0xDD, frame[5]);
  TEST_ASSERT_EQUAL_HEX8(0xCC, frame[6]);
  TEST_ASSERT_EQUAL_HEX8(0xBB, frame[7]);
  TEST_ASSERT_EQUAL_HEX8(0xAA, frame[8]);  // nodeId byte 5
  TEST_ASSERT_EQUAL_HEX8(0x10, frame[9]);  // seq16 lo
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[10]); // seq16 hi
  TEST_ASSERT_EQUAL_HEX8(0x42, frame[11]); // ref_core_seq16 lo
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[12]); // ref_core_seq16 hi

  // Decode round-trip
  Tail1Fields decoded{};
  const auto err = naviga::protocol::decode_tail1_payload(frame + 2, 11, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT16(0x0010, decoded.seq16);
  TEST_ASSERT_EQUAL_UINT16(0x0042, decoded.ref_core_seq16);
  TEST_ASSERT_FALSE(decoded.has_pos_flags);
  TEST_ASSERT_FALSE(decoded.has_sats);
}

void test_tail1_codec_extended_round_trip() {
  // Golden vector: with posFlags=0x01, sats=8.
  // Expected payload (13 B): 00 FF EE DD CC BB AA 01 00 01 00 01 08
  Tail1Fields fields{};
  fields.node_id          = 0x0000AABBCCDDEEFFULL;
  fields.seq16            = 0x0001;
  fields.ref_core_seq16   = 0x0001;
  fields.has_pos_flags    = true;
  fields.pos_flags        = 0x01;
  fields.has_sats         = true;
  fields.sats             = 8;

  uint8_t frame[kTail1FrameMax] = {};
  const size_t written = encode_tail1_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail1FrameMax, written);

  // Header: msg_type=0x03, payload_len=13 → H=(0x03<<9)|13=0x060D → [0x0D, 0x06]
  TEST_ASSERT_EQUAL_HEX8(0x0D, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x06, frame[1]);
  TEST_ASSERT_EQUAL_HEX8(0x01, frame[13]); // posFlags
  TEST_ASSERT_EQUAL_HEX8(0x08, frame[14]); // sats

  Tail1Fields decoded{};
  const auto err = naviga::protocol::decode_tail1_payload(frame + 2, 13, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT16(0x0001, decoded.seq16);
  TEST_ASSERT_EQUAL_UINT16(0x0001, decoded.ref_core_seq16);
  TEST_ASSERT_TRUE(decoded.has_pos_flags);
  TEST_ASSERT_EQUAL_HEX8(0x01, decoded.pos_flags);
  TEST_ASSERT_TRUE(decoded.has_sats);
  TEST_ASSERT_EQUAL_UINT8(8, decoded.sats);
}

void test_tail1_seq16_distinct_from_ref_core_seq16() {
  // Verify that seq16 and ref_core_seq16 are independently stored.
  Tail1Fields fields{};
  fields.node_id        = 0x0000AABBCCDDEEFFULL;
  fields.seq16          = 0x00AA;
  fields.ref_core_seq16 = 0x0055;

  uint8_t frame[kTail1FrameMin] = {};
  const size_t written = encode_tail1_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail1FrameMin, written);

  Tail1Fields decoded{};
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(
      naviga::protocol::decode_tail1_payload(frame + 2, 11, &decoded)));
  TEST_ASSERT_EQUAL_UINT16(0x00AA, decoded.seq16);
  TEST_ASSERT_EQUAL_UINT16(0x0055, decoded.ref_core_seq16);
  TEST_ASSERT_NOT_EQUAL(decoded.seq16, decoded.ref_core_seq16);
}

void test_tail1_decode_bad_version_dropped() {
  // 11-byte payload with bad payloadVersion=0x01
  uint8_t payload[11] = {0x01, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
                          0x01, 0x00, 0x01, 0x00};
  Tail1Fields out{};
  const auto err = naviga::protocol::decode_tail1_payload(payload, 11, &out);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(naviga::protocol::Tail1DecodeError::BadPayloadVersion),
      static_cast<int>(err));
}

void test_tail1_decode_bad_len_dropped() {
  // payload_len=12 is not in {11, 13} (both are >= min=11, so this hits BadPayloadLen)
  uint8_t payload[12] = {};
  Tail1Fields out{};
  const auto err = naviga::protocol::decode_tail1_payload(payload, 12, &out);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(naviga::protocol::Tail1DecodeError::BadPayloadLen),
      static_cast<int>(err));
}

// ── Tail-2 (Operational) codec: golden vectors ──────────────────────────────
// New layout: Common(9B) + optional batteryPercent + optional uptimeSec
// kTail2FrameMin = 11 B

void test_tail2_codec_base_round_trip() {
  // Golden vector: base only (9 B payload).
  Tail2Fields fields{};
  fields.node_id = 0x0000AABBCCDDEEFFULL;
  fields.seq16   = 0x0007;

  uint8_t frame[kTail2FrameMax] = {};
  const size_t written = encode_tail2_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail2FrameMin, written);

  // Header: msg_type=0x04, payload_len=9 → H=(0x04<<9)|9=0x0809 → [0x09, 0x08]
  TEST_ASSERT_EQUAL_HEX8(0x09, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x08, frame[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[2]); // payloadVersion
  // seq16 at offset 7–8 in payload (frame offset 9–10)
  TEST_ASSERT_EQUAL_HEX8(0x07, frame[9]);  // seq16 lo
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[10]); // seq16 hi

  Tail2Fields decoded{};
  const auto err = naviga::protocol::decode_tail2_payload(frame + 2, 9, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT16(0x0007, decoded.seq16);
  TEST_ASSERT_FALSE(decoded.has_battery);
  TEST_ASSERT_FALSE(decoded.has_uptime);
}

void test_tail2_codec_with_battery_round_trip() {
  // Golden vector: with batteryPercent=75.
  Tail2Fields fields{};
  fields.node_id      = 0x0000AABBCCDDEEFFULL;
  fields.seq16        = 0x0003;
  fields.has_battery  = true;
  fields.battery_percent = 75;

  uint8_t frame[kTail2FrameMax] = {};
  const size_t written = encode_tail2_frame(fields, frame, sizeof(frame));
  // payload_len = 10 → frame = 12 bytes
  TEST_ASSERT_EQUAL_UINT32(kTail2FrameMin + 1, written);
  // batteryPercent at payload offset 9 → frame offset 11
  TEST_ASSERT_EQUAL_HEX8(75, frame[11]);

  Tail2Fields decoded{};
  const auto err = naviga::protocol::decode_tail2_payload(frame + 2, 10, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_TRUE(decoded.has_battery);
  TEST_ASSERT_EQUAL_UINT8(75, decoded.battery_percent);
  TEST_ASSERT_FALSE(decoded.has_uptime);
}

void test_tail2_decode_bad_version_dropped() {
  // 9-byte payload with bad payloadVersion=0x01
  uint8_t payload[9] = {0x01, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x01, 0x00};
  Tail2Fields out{};
  const auto err = naviga::protocol::decode_tail2_payload(payload, 9, &out);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(naviga::protocol::Tail2DecodeError::BadPayloadVersion),
      static_cast<int>(err));
}

// ── Info (Informative) codec: golden vectors ────────────────────────────────

void test_info_codec_base_round_trip() {
  // Golden vector: base only (9 B payload).
  InfoFields fields{};
  fields.node_id = 0x0000AABBCCDDEEFFULL;
  fields.seq16   = 0x0005;

  uint8_t frame[kInfoFrameMax] = {};
  const size_t written = encode_info_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kInfoFrameMin, written);

  // Header: msg_type=0x05, payload_len=9 → H=(0x05<<9)|9=0x0A09 → [0x09, 0x0A]
  TEST_ASSERT_EQUAL_HEX8(0x09, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x0A, frame[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[2]); // payloadVersion
  // seq16 at payload offset 7–8 (frame offset 9–10)
  TEST_ASSERT_EQUAL_HEX8(0x05, frame[9]);  // seq16 lo
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[10]); // seq16 hi

  InfoFields decoded{};
  const auto err = naviga::protocol::decode_info_payload(frame + 2, 9, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT16(0x0005, decoded.seq16);
  TEST_ASSERT_FALSE(decoded.has_max_silence);
  TEST_ASSERT_FALSE(decoded.has_hw_profile);
  TEST_ASSERT_FALSE(decoded.has_fw_version);
}

void test_info_codec_full_round_trip() {
  // Golden vector: all optional fields.
  InfoFields fields{};
  fields.node_id          = 0x0000AABBCCDDEEFFULL;
  fields.seq16            = 0x00FF;
  fields.has_max_silence  = true;
  fields.max_silence_10s  = 9;  // 90 s
  fields.has_hw_profile   = true;
  fields.hw_profile_id    = 0x0102;
  fields.has_fw_version   = true;
  fields.fw_version_id    = 0x0304;

  uint8_t frame[kInfoFrameMax] = {};
  const size_t written = encode_info_frame(fields, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kInfoFrameMax, written);

  InfoFields decoded{};
  const auto err = naviga::protocol::decode_info_payload(frame + 2, 14, &decoded);
  TEST_ASSERT_EQUAL_INT(0, static_cast<int>(err));
  TEST_ASSERT_EQUAL_UINT64(fields.node_id, decoded.node_id);
  TEST_ASSERT_EQUAL_UINT16(0x00FF, decoded.seq16);
  TEST_ASSERT_TRUE(decoded.has_max_silence);
  TEST_ASSERT_EQUAL_UINT8(9, decoded.max_silence_10s);
  TEST_ASSERT_TRUE(decoded.has_hw_profile);
  TEST_ASSERT_EQUAL_UINT16(0x0102, decoded.hw_profile_id);
  TEST_ASSERT_TRUE(decoded.has_fw_version);
  TEST_ASSERT_EQUAL_UINT16(0x0304, decoded.fw_version_id);
}

void test_info_decode_bad_version_dropped() {
  // 9-byte payload with bad payloadVersion=0x01
  uint8_t payload[9] = {0x01, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x01, 0x00};
  InfoFields out{};
  const auto err = naviga::protocol::decode_info_payload(payload, 9, &out);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(naviga::protocol::InfoDecodeError::BadPayloadVersion),
      static_cast<int>(err));
}

// (v0.1 RX tests removed #438; codec tests for tail1/tail2/info remain above.)

// ── TX queue: helpers ────────────────────────────────────────────────────────

static GeoBeaconFields make_self_fields(uint64_t node_id, bool pos_valid,
                                        double lat = 55.0, double lon = 37.0) {
  GeoBeaconFields f{};
  f.node_id   = node_id;
  f.pos_valid = pos_valid ? 1 : 0;
  f.lat_deg   = lat;
  f.lon_deg   = lon;
  return f;
}

// Decode the seq16 from a Core frame (payload offset 7–8, frame offset 9–10).
// Seq16 in PosFull/Core payload at bytes 7–8 (frame offset 9–10).
static uint16_t pos_full_frame_seq16(const uint8_t* frame) {
  return static_cast<uint16_t>(frame[9] | (static_cast<uint16_t>(frame[10]) << 8));
}

static uint16_t core_frame_seq16(const uint8_t* frame) {
  return pos_full_frame_seq16(frame);
}

static uint16_t tail1_frame_seq16(const uint8_t* frame) {
  return static_cast<uint16_t>(frame[9] | (static_cast<uint16_t>(frame[10]) << 8));
}
static uint16_t tail1_frame_ref_core_seq16(const uint8_t* frame) {
  return static_cast<uint16_t>(frame[11] | (static_cast<uint16_t>(frame[12]) << 8));
}

// ── TX queue: formation tests ────────────────────────────────────────────────

void test_txq_core_enqueues_tail_with_correct_ref() {
  // v0.2: When position valid, only Node_Pos_Full is enqueued (one seq16 per position).
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};

  logic.update_tx_queue(1000, self, telem, true);

  TEST_ASSERT_TRUE(logic.slot(kSlotPosFull).present);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::POS_FULL),
                    static_cast<int>(logic.slot(kSlotPosFull).pkt_type));

  const uint16_t pos_seq = pos_full_frame_seq16(logic.slot(kSlotPosFull).frame);
  TEST_ASSERT_EQUAL_UINT16(1u, pos_seq);
}

void test_txq_tail1_not_enqueued_without_core() {
  // v0.2: PosFull not enqueued when allow_core=false and no max_silence.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};

  logic.update_tx_queue(1000, self, telem, false);

  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
}

void test_txq_core_replacement_replaces_tail_too() {
  // v0.2: PosFull re-formed before send → slot replaced_count increments.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};

  logic.update_tx_queue(1000, self, telem, true);
  const uint16_t first_seq = pos_full_frame_seq16(logic.slot(kSlotPosFull).frame);
  TEST_ASSERT_EQUAL_UINT32(0, logic.slot(kSlotPosFull).replaced_count);

  self.lat_deg += 0.001;
  logic.update_tx_queue(2000, self, telem, true);
  const uint16_t second_seq = pos_full_frame_seq16(logic.slot(kSlotPosFull).frame);

  TEST_ASSERT_EQUAL_UINT32(1, logic.slot(kSlotPosFull).replaced_count);
  TEST_ASSERT_NOT_EQUAL(first_seq, second_seq);
}

void test_txq_alive_enqueued_when_no_fix_at_max_silence() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(2000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, false);  // no fix
  SelfTelemetry telem{};

  // Before max_silence: no Alive.
  logic.update_tx_queue(1000, self, telem, false);
  TEST_ASSERT_FALSE(logic.slot(kSlotAlive).present);
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);

  // At max_silence: Alive enqueued.
  logic.update_tx_queue(2000, self, telem, false);
  TEST_ASSERT_TRUE(logic.slot(kSlotAlive).present);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::ALIVE),
                    static_cast<int>(logic.slot(kSlotAlive).pkt_type));
  // Core must NOT be enqueued (no fix).
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
}

void test_txq_operational_enqueued_independently() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // Core not due (allow_core=false); cadence due at t=1000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery     = true;
  telem.battery_percent = 85;

  logic.update_tx_queue(1000, self, telem, false);  // allow_core=false; time_for_min=true

  // Core not enqueued (allow_core=false).
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
  // Operational enqueued when cadence is due.
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS),
                    static_cast<int>(logic.slot(kSlotStatus).pkt_type));
}

void test_txq_informative_enqueued_independently() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_max_silence = true;
  telem.max_silence_10s = 9;

  logic.update_tx_queue(1000, self, telem, false);  // time_for_min=true

  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS),
                    static_cast<int>(logic.slot(kSlotStatus).pkt_type));
}

// ── TX queue: telemetry gate (empty telemetry → no 0x04/0x05) ────────────────

void test_txq_empty_telemetry_no_operational_no_informative() {
  // Verifies the formation gates: with all has_* = false, neither 0x04 nor 0x05
  // is enqueued, regardless of Core/Alive state.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};  // all has_* = false (default)

  logic.update_tx_queue(1000, self, telem, true);

  TEST_ASSERT_FALSE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_FALSE(logic.slot(kSlotStatus).present);
  // Core and Tail1 may be present (unrelated to telemetry gate).
}

void test_txq_op_info_not_enqueued_before_cadence() {
  // #420: Op and Info are gated by (time_for_min || time_for_silence). When elapsed
  // is below both min_interval and max_silence, neither Operational nor Informative
  // must be enqueued, even if telemetry has data (canon cadence gate).
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery     = true;
  telem.battery_percent = 80;
  telem.has_max_silence = true;
  telem.max_silence_10s = 9;

  // First formation at t=500: elapsed=500 < min_interval and < max_silence.
  logic.update_tx_queue(500, self, telem, false);

  TEST_ASSERT_FALSE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_FALSE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
}

void test_txq_422_status_throttle_min_interval_respected() {
  // #422 Path B: After two status "sends" (on_status_sent), next formation within min_status_interval
  // must not enqueue another status (anti-burst).
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(120000);
  logic.set_min_status_interval_ms(30000);  // 30s

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery     = true;
  telem.battery_percent = 90;

  logic.update_tx_queue(1000, self, telem, false);   // enqueue Op (bootstrap)
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  logic.on_status_sent(1000);                         // first "send"

  logic.update_tx_queue(31000, self, telem, false);  // 30s later: interval elapsed, enqueue again
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  logic.on_status_sent(31000);                        // second "send" (bootstrap done)

  // 1 ms later: within min_status_interval, must NOT enqueue (replace) status.
  logic.update_tx_queue(31001, self, telem, false);
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  // created_at_ms unchanged (we did not replace the slot this pass).
  TEST_ASSERT_EQUAL_UINT32(1000, logic.slot(kSlotStatus).created_at_ms);
}

void test_txq_uptime_only_enqueues_operational_not_informative() {
  // has_uptime=true → 0x04 enqueued; has_max_silence=false → 0x05 NOT enqueued.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_uptime  = true;
  telem.uptime_sec  = 42;

  logic.update_tx_queue(1000, self, telem, false);  // time_for_min=true

  // v0.2: one Status slot; uptime-only enqueues Status.
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
}

void test_txq_max_silence_only_enqueues_informative_not_operational() {
  // has_max_silence=true → Status enqueued (v0.2 single Status slot).
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_max_silence = true;
  telem.max_silence_10s = 9;

  logic.update_tx_queue(1000, self, telem, false);  // time_for_min=true

  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
}

// #443: Status frame must encode role_id from SelfTelemetry (active user profile).
void test_txq_status_encodes_role_id_from_telemetry() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.role_id = 1;   // Dog
  telem.has_max_silence = true;
  telem.max_silence_10s = 9;

  logic.update_tx_queue(1000, self, telem, false);
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);

  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::CORE;
  TEST_ASSERT_TRUE(logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype));
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS), static_cast<int>(ptype));

  const size_t kHeaderSize = 2;
  if (out_len > kHeaderSize) {
    naviga::protocol::StatusFields decoded{};
    const auto err = naviga::protocol::decode_status_payload(
        buf + kHeaderSize, out_len - kHeaderSize, &decoded);
    TEST_ASSERT_EQUAL(static_cast<int>(naviga::protocol::StatusDecodeError::Ok), static_cast<int>(err));
    TEST_ASSERT_EQUAL_UINT8(1, decoded.role_id);
  }
}

// ── TX queue: dequeue / priority / fairness tests ────────────────────────────

void test_txq_dequeue_empty_returns_false() {
  BeaconLogic logic;
  uint8_t buf[65] = {};
  size_t out_len = 0;
  TEST_ASSERT_FALSE(logic.dequeue_tx(buf, sizeof(buf), &out_len));
  TEST_ASSERT_EQUAL_UINT32(0, out_len);
}

void test_txq_dequeue_core_before_operational() {
  // Core (P0) must be dequeued before Operational (P3). #422: no hitchhiking — Op and Core from separate passes.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery     = true;
  telem.battery_percent = 90;

  // First pass: enqueue Op only (allow_core=false).
  logic.update_tx_queue(1000, self, telem, false);
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  // Second pass: enqueue Core+Tail1 (allow_core=true); no Op this pass (no hitchhiking).
  logic.update_tx_queue(1000, self, telem, true);

  // Both Core and Operational should be present (Op from first pass, Core from second).
  TEST_ASSERT_TRUE(logic.slot(kSlotPosFull).present);
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);

  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::STATUS;
  TEST_ASSERT_TRUE(logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype));
  // PosFull dequeued first (highest priority).
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::POS_FULL), static_cast<int>(ptype));
  // PosFull slot cleared.
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
  // Status still pending.
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
}

void test_txq_dequeue_tail1_before_operational() {
  // Tail-1 (P2) must be dequeued before Operational (P3). #422: no hitchhiking — Op and Core from separate passes.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery     = true;
  telem.battery_percent = 90;

  logic.update_tx_queue(1000, self, telem, false);  // Op only
  logic.update_tx_queue(1000, self, telem, true);   // PosFull only (v0.2)

  // Dequeue PosFull first (P0).
  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::POS_FULL;
  logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::POS_FULL), static_cast<int>(ptype));

  // PosFull cleared; Status (P3) remains.
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);

  logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS), static_cast<int>(ptype));
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
}

void test_txq_fairness_higher_replaced_count_wins() {
  // Within same priority (P3), the slot with higher replaced_count is dequeued first.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000, 2000, 3000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);

  // Enqueue Operational once (replaced_count=0).
  SelfTelemetry telem_op{};
  telem_op.has_battery     = true;
  telem_op.battery_percent = 80;
  logic.update_tx_queue(1000, self, telem_op, false);  // time_for_min=true
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL_UINT32(0, logic.slot(kSlotStatus).replaced_count);
  TEST_ASSERT_EQUAL(static_cast<int>(TxPriority::P3_THROTTLED),
                    static_cast<int>(logic.slot(kSlotStatus).priority));

  // Enqueue Informative (replaces same Status slot; replaced_count=1).
  SelfTelemetry telem_info{};
  telem_info.has_max_silence = true;
  telem_info.max_silence_10s = 9;
  logic.update_tx_queue(2000, self, telem_info, false);  // time_for_min=true
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL_UINT32(1, logic.slot(kSlotStatus).replaced_count);

  // Replace again with Operational (replaced_count becomes 2).
  telem_op.battery_percent = 75;
  logic.update_tx_queue(3000, self, telem_op, false);  // time_for_min=true
  TEST_ASSERT_EQUAL_UINT32(2, logic.slot(kSlotStatus).replaced_count);

  // Both P3 slots present. Operational has replaced_count=1, Info has replaced_count=0.
  // Operational should be dequeued first (higher replaced_count = more starved).
  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::INFO;
  TEST_ASSERT_TRUE(logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype));
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS), static_cast<int>(ptype));

  // v0.2: one Status slot only; no second dequeue.
  TEST_ASSERT_FALSE(logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype));
}

void test_txq_fairness_older_created_at_wins_on_tie() {
  // Within same priority (P3) and same replaced_count, older created_at_ms wins.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000, 2000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);

  // Enqueue Operational at t=1000 (older).
  SelfTelemetry telem_op{};
  telem_op.has_battery     = true;
  telem_op.battery_percent = 80;
  logic.update_tx_queue(1000, self, telem_op, false);  // time_for_min=true

  // Enqueue Informative at t=2000 (newer).
  SelfTelemetry telem_info{};
  telem_info.has_max_silence = true;
  telem_info.max_silence_10s = 9;
  logic.update_tx_queue(2000, self, telem_info, false);  // time_for_min=true

  // Both replaced_count=0, same priority (P3). Operational (older) should win.
  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::INFO;
  TEST_ASSERT_TRUE(logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype));
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS), static_cast<int>(ptype));
}

void test_txq_slot_replacement_increments_count() {
  // Replacing a slot increments replaced_count; created_at_ms is preserved.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000, 2000, 3000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery     = true;
  telem.battery_percent = 90;

  logic.update_tx_queue(1000, self, telem, false);  // time_for_min=true
  TEST_ASSERT_EQUAL_UINT32(0, logic.slot(kSlotStatus).replaced_count);
  TEST_ASSERT_EQUAL_UINT32(1000, logic.slot(kSlotStatus).created_at_ms);

  telem.battery_percent = 80;
  logic.update_tx_queue(2000, self, telem, false);  // time_for_min=true
  TEST_ASSERT_EQUAL_UINT32(1, logic.slot(kSlotStatus).replaced_count);
  // created_at_ms preserved from first enqueue.
  TEST_ASSERT_EQUAL_UINT32(1000, logic.slot(kSlotStatus).created_at_ms);

  telem.battery_percent = 70;
  logic.update_tx_queue(3000, self, telem, false);  // time_for_min=true
  TEST_ASSERT_EQUAL_UINT32(2, logic.slot(kSlotStatus).replaced_count);
  TEST_ASSERT_EQUAL_UINT32(1000, logic.slot(kSlotStatus).created_at_ms);
}

void test_txq_dequeue_clears_slot() {
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery     = true;
  telem.battery_percent = 90;

  logic.update_tx_queue(1000, self, telem, false);  // time_for_min=true
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);

  uint8_t buf[65] = {};
  size_t out_len = 0;
  logic.dequeue_tx(buf, sizeof(buf), &out_len);
  // Slot must be cleared after dequeue.
  TEST_ASSERT_FALSE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_FALSE(logic.has_pending_tx());
}

void test_txq_no_core_when_allow_core_false() {
  // Core must not be enqueued when allow_core=false, even if time_for_min.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};

  logic.update_tx_queue(1000, self, telem, false);  // allow_core=false
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);
}

void test_txq_core_enqueues_at_max_silence_even_without_allow_core() {
  // At max_silence, Core is enqueued regardless of allow_core (silence override).
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(2000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};

  logic.update_tx_queue(2000, self, telem, false);  // allow_core=false but max_silence hit
  TEST_ASSERT_TRUE(logic.slot(kSlotPosFull).present);
  TEST_ASSERT_TRUE(logic.slot(kSlotPosFull).present);
}

// ── TX queue: Alive replacement + dequeue ────────────────────────────────────

void test_txq_alive_replacement_increments_count() {
  // Alive slot replacement increments replaced_count and preserves created_at_ms.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(2000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, false);  // no fix
  SelfTelemetry telem{};

  // First Alive at t=2000 (max_silence hit).
  logic.update_tx_queue(2000, self, telem, false);
  TEST_ASSERT_TRUE(logic.slot(kSlotAlive).present);
  TEST_ASSERT_EQUAL_UINT32(0, logic.slot(kSlotAlive).replaced_count);
  TEST_ASSERT_EQUAL_UINT32(2000, logic.slot(kSlotAlive).created_at_ms);

  // Second Alive at t=4000 (max_silence hit again; slot not yet dequeued).
  logic.update_tx_queue(4000, self, telem, false);
  TEST_ASSERT_TRUE(logic.slot(kSlotAlive).present);
  TEST_ASSERT_EQUAL_UINT32(1, logic.slot(kSlotAlive).replaced_count);
  // created_at_ms preserved from first enqueue.
  TEST_ASSERT_EQUAL_UINT32(2000, logic.slot(kSlotAlive).created_at_ms);
}

void test_txq_alive_dequeued_correctly() {
  // Alive slot is dequeued as ALIVE type with correct frame.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(2000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, false);  // no fix
  SelfTelemetry telem{};

  logic.update_tx_queue(2000, self, telem, false);
  TEST_ASSERT_TRUE(logic.slot(kSlotAlive).present);

  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::CORE;
  uint16_t core_seq = 0xFFFF;
  TEST_ASSERT_TRUE(logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype, &core_seq));
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::ALIVE), static_cast<int>(ptype));
  TEST_ASSERT_EQUAL_UINT16(0, core_seq);  // Alive has no ref_core_seq16
  TEST_ASSERT_TRUE(out_len >= naviga::protocol::kAliveFrameMin);
  // Slot cleared.
  TEST_ASSERT_FALSE(logic.slot(kSlotAlive).present);
  TEST_ASSERT_FALSE(logic.has_pending_tx());
}

// ── TX queue: Informative replacement + dequeue ──────────────────────────────

void test_txq_informative_replacement_increments_count() {
  // Informative slot replacement increments replaced_count and preserves created_at_ms.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000, 2000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);

  SelfTelemetry telem{};
  telem.has_max_silence = true;
  telem.max_silence_10s = 9;

  logic.update_tx_queue(1000, self, telem, false);  // time_for_min=true
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL_UINT32(0, logic.slot(kSlotStatus).replaced_count);
  TEST_ASSERT_EQUAL_UINT32(1000, logic.slot(kSlotStatus).created_at_ms);

  // Replace with new value.
  telem.max_silence_10s = 18;
  logic.update_tx_queue(2000, self, telem, false);  // time_for_min=true
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL_UINT32(1, logic.slot(kSlotStatus).replaced_count);
  TEST_ASSERT_EQUAL_UINT32(1000, logic.slot(kSlotStatus).created_at_ms);
}

void test_txq_informative_dequeued_correctly() {
  // Informative slot is dequeued as INFO type with correct frame.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);   // cadence due at t=1000
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);

  SelfTelemetry telem{};
  telem.has_max_silence = true;
  telem.max_silence_10s = 9;

  logic.update_tx_queue(1000, self, telem, false);  // time_for_min=true

  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::CORE;
  uint16_t core_seq = 0xFFFF;
  TEST_ASSERT_TRUE(logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype, &core_seq));
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS), static_cast<int>(ptype));
  TEST_ASSERT_EQUAL_UINT16(0, core_seq);
  TEST_ASSERT_TRUE(out_len >= naviga::protocol::kInfoFrameMin);
  TEST_ASSERT_FALSE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_FALSE(logic.has_pending_tx());
}

// ── TX queue: P1 reserved + full priority ordering ───────────────────────────

void test_txq_p1_never_assigned_in_formation() {
  // P1_SESSION_MESH is reserved for future Session/Mesh packets.
  // The formation pass MUST NOT assign P1 to any current OOTB packet type.
  // Verify that after a full formation pass (all 5 slots), no slot has P1.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery     = true;
  telem.battery_percent = 90;
  telem.has_max_silence = true;
  telem.max_silence_10s = 9;

  logic.update_tx_queue(1000, self, telem, true);

  for (size_t i = 0; i < naviga::domain::kTxSlotCount; ++i) {
    if (logic.slot(i).present) {
      TEST_ASSERT_NOT_EQUAL(static_cast<int>(TxPriority::P1_SESSION_MESH),
                            static_cast<int>(logic.slot(i).priority));
    }
  }
}

void test_txq_p2_tail1_before_p3_operational_informative() {
  // Core_Tail (P2) must be dequeued before Operational (P3) and Informative (P3).
  BeaconLogic logic;
  logic.set_min_interval_ms(500);    // cadence due at t=500 for Op/Info
  logic.set_max_silence_ms(120000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);

  // #422 Path B: at most one P3 per pass. Enqueue Operational then Informative in two passes.
  SelfTelemetry telem_op{};
  telem_op.has_battery     = true;
  telem_op.battery_percent = 90;
  logic.update_tx_queue(500, self, telem_op, false);  // Op only
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  SelfTelemetry telem_info{};
  telem_info.has_max_silence = true;
  telem_info.max_silence_10s = 9;
  logic.update_tx_queue(500, self, telem_info, false);  // Info only (second pass)
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL(static_cast<int>(TxPriority::P3_THROTTLED),
                    static_cast<int>(logic.slot(kSlotStatus).priority));
  TEST_ASSERT_EQUAL(static_cast<int>(TxPriority::P3_THROTTLED),
                    static_cast<int>(logic.slot(kSlotStatus).priority));
  TEST_ASSERT_FALSE(logic.slot(kSlotPosFull).present);

  // Now enqueue Core (and thus Core_Tail) at t=1000.
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);
  SelfTelemetry telem2{};
  logic.update_tx_queue(1000, self, telem2, true);
  TEST_ASSERT_TRUE(logic.slot(kSlotPosFull).present);
  TEST_ASSERT_EQUAL(static_cast<int>(TxPriority::P0_MUST_PERIODIC),
                    static_cast<int>(logic.slot(kSlotPosFull).priority));

  // Dequeue Core (P0) first.
  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::INFO;
  logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::POS_FULL), static_cast<int>(ptype));

  // Next: Status (P3). v0.2 has no Tail1.
  logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS), static_cast<int>(ptype));
}

void test_txq_starvation_increments_replaced_count() {
  // When we dequeue one slot, every other present slot gets +1 replaced_count (starvation). #422: no hitchhiking.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem{};
  telem.has_battery = true;
  telem.battery_percent = 90;

  logic.update_tx_queue(1000, self, telem, false);  // Op only (first pass)
  logic.update_tx_queue(1000, self, telem, true);   // Core+Tail1 (second pass; no Op this pass)
  TEST_ASSERT_TRUE(logic.slot(kSlotPosFull).present);
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL_UINT32(0, logic.slot(kSlotStatus).replaced_count);

  uint8_t buf[65] = {};
  size_t out_len = 0;
  logic.dequeue_tx(buf, sizeof(buf), &out_len);
  // Core was dequeued; Tail2 was present but not sent → starvation +1.
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);
  TEST_ASSERT_EQUAL_UINT32(1, logic.slot(kSlotStatus).replaced_count);
}

void test_txq_priority_ordering_p0_beats_all() {
  // Verify full ordering: P0 > P2 > P3 with all 5 slots present. #422: no hitchhiking — P3 and Core from separate passes.
  BeaconLogic logic;
  logic.set_min_interval_ms(1000);
  logic.set_max_silence_ms(30000);

  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  GeoBeaconFields self = make_self_fields(node_id, true);
  SelfTelemetry telem_op{};
  telem_op.has_battery     = true;
  telem_op.battery_percent = 90;
  SelfTelemetry telem_info{};
  telem_info.has_max_silence = true;
  telem_info.max_silence_10s = 9;

  // Enqueue Op, then Info, then Core+Tail1 (three passes; at most one P3 per pass).
  logic.update_tx_queue(1000, self, telem_op, false);
  logic.update_tx_queue(1000, self, telem_info, false);
  logic.update_tx_queue(1000, self, telem_op, true);

  TEST_ASSERT_TRUE(logic.slot(kSlotPosFull).present);   // P0
  TEST_ASSERT_TRUE(logic.slot(kSlotPosFull).present);   // P2
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);  // P3
  TEST_ASSERT_TRUE(logic.slot(kSlotStatus).present);   // P3
  TEST_ASSERT_EQUAL(static_cast<int>(TxPriority::P3_THROTTLED),
                    static_cast<int>(logic.slot(kSlotStatus).priority));
  TEST_ASSERT_EQUAL(static_cast<int>(TxPriority::P3_THROTTLED),
                    static_cast<int>(logic.slot(kSlotStatus).priority));

  uint8_t buf[65] = {};
  size_t out_len = 0;
  PacketLogType ptype = PacketLogType::INFO;

  // 1st dequeue: PosFull (P0).
  logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::POS_FULL), static_cast<int>(ptype));

  // 2nd dequeue: Status (P3). v0.2 has only 2 slots (PosFull, Status) in this scenario.
  logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype);
  TEST_ASSERT_EQUAL(static_cast<int>(PacketLogType::STATUS), static_cast<int>(ptype));

  // No more slots.
  TEST_ASSERT_FALSE(logic.dequeue_tx(buf, sizeof(buf), &out_len, &ptype));

  TEST_ASSERT_FALSE(logic.has_pending_tx());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_tx_cadence);
  RUN_TEST(test_seq16_default_first_packet_is_one);
  RUN_TEST(test_seq16_set_initial_then_first_packet_is_initial_plus_one);
  RUN_TEST(test_seq16_set_initial_100_first_packet_is_101);
  RUN_TEST(test_seq16_wraparound_first_packet_is_zero);
  RUN_TEST(test_tx_max_silence_triggers_core);
  RUN_TEST(test_tx_no_fix_at_max_silence_sends_alive);
  RUN_TEST(test_tx_min_interval_no_update_no_send);
  RUN_TEST(test_tx_payload_correctness);
  RUN_TEST(test_rx_v01_packets_dropped);
  RUN_TEST(test_rx_pos_full_applies_position_and_quality);
  RUN_TEST(test_rx_status_applies_full_snapshot);
  RUN_TEST(test_rx_alive_success_updates_node_table);
  RUN_TEST(test_rx_alive_does_not_overwrite_core_position);
  RUN_TEST(test_rx_invalid_frame_too_short);
  RUN_TEST(test_rx_unknown_msgtype_dropped);
  RUN_TEST(test_rx_payload_len_mismatch_dropped);
  RUN_TEST(test_decode_header_rejects_v01_msgtypes);
  RUN_TEST(test_decode_header_accepts_0x06);
  // Tail-1 codec
  RUN_TEST(test_tail1_codec_base_round_trip);
  RUN_TEST(test_tail1_codec_extended_round_trip);
  RUN_TEST(test_tail1_seq16_distinct_from_ref_core_seq16);
  RUN_TEST(test_tail1_decode_bad_version_dropped);
  RUN_TEST(test_tail1_decode_bad_len_dropped);
  // Tail-2 codec
  RUN_TEST(test_tail2_codec_base_round_trip);
  RUN_TEST(test_tail2_codec_with_battery_round_trip);
  RUN_TEST(test_tail2_decode_bad_version_dropped);
  // Info codec
  RUN_TEST(test_info_codec_base_round_trip);
  RUN_TEST(test_info_codec_full_round_trip);
  RUN_TEST(test_info_decode_bad_version_dropped);
  // TX queue: formation
  RUN_TEST(test_txq_core_enqueues_tail_with_correct_ref);
  RUN_TEST(test_txq_tail1_not_enqueued_without_core);
  RUN_TEST(test_txq_core_replacement_replaces_tail_too);
  RUN_TEST(test_txq_alive_enqueued_when_no_fix_at_max_silence);
  RUN_TEST(test_txq_operational_enqueued_independently);
  RUN_TEST(test_txq_informative_enqueued_independently);
  // TX queue: telemetry gate + cadence gate (#420)
  RUN_TEST(test_txq_empty_telemetry_no_operational_no_informative);
  RUN_TEST(test_txq_op_info_not_enqueued_before_cadence);
  RUN_TEST(test_txq_422_status_throttle_min_interval_respected);
  RUN_TEST(test_txq_uptime_only_enqueues_operational_not_informative);
  RUN_TEST(test_txq_max_silence_only_enqueues_informative_not_operational);
  RUN_TEST(test_txq_status_encodes_role_id_from_telemetry);
  // TX queue: dequeue / priority / fairness
  RUN_TEST(test_txq_dequeue_empty_returns_false);
  RUN_TEST(test_txq_dequeue_core_before_operational);
  RUN_TEST(test_txq_dequeue_tail1_before_operational);
  RUN_TEST(test_txq_fairness_higher_replaced_count_wins);
  RUN_TEST(test_txq_fairness_older_created_at_wins_on_tie);
  RUN_TEST(test_txq_slot_replacement_increments_count);
  RUN_TEST(test_txq_dequeue_clears_slot);
  RUN_TEST(test_txq_no_core_when_allow_core_false);
  RUN_TEST(test_txq_core_enqueues_at_max_silence_even_without_allow_core);
  // TX queue: Alive replacement + dequeue (gap coverage)
  RUN_TEST(test_txq_alive_replacement_increments_count);
  RUN_TEST(test_txq_alive_dequeued_correctly);
  // TX queue: Informative replacement + dequeue (gap coverage)
  RUN_TEST(test_txq_informative_replacement_increments_count);
  RUN_TEST(test_txq_informative_dequeued_correctly);
  // TX queue: P1 reserved; P2 vs P3 ordering; starvation; full priority ordering
  RUN_TEST(test_txq_p1_never_assigned_in_formation);
  RUN_TEST(test_txq_p2_tail1_before_p3_operational_informative);
  RUN_TEST(test_txq_starvation_increments_replaced_count);
  RUN_TEST(test_txq_priority_ordering_p0_beats_all);
  return UNITY_END();
}
