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

using naviga::domain::BeaconLogic;
using naviga::domain::NodeTable;
using naviga::domain::NodeEntry;
using naviga::domain::PacketLogType;
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
  AliveFields alive_f{};
  alive_f.node_id = node_id;
  alive_f.seq     = 2;
  uint8_t alive_frame[kAliveFrameMin] = {};
  TEST_ASSERT_EQUAL_UINT32(kAliveFrameMin,
      encode_alive_frame(alive_f, alive_frame, sizeof(alive_frame)));
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

// ── decode_header: accepts 0x05 ──────────────────────────────────────────────

void test_decode_header_accepts_0x05() {
  // msg_type=0x05, payload_len=9 → H=(0x05<<9)|9=0x0A09 → [0x09, 0x0A]
  uint8_t frame[2] = {0x09, 0x0A};
  naviga::protocol::PacketHeader hdr{};
  TEST_ASSERT_TRUE(naviga::protocol::decode_header(frame, sizeof(frame), &hdr));
  TEST_ASSERT_EQUAL(static_cast<int>(naviga::protocol::MsgType::BeaconInfo),
                    static_cast<int>(hdr.msg_type));
  TEST_ASSERT_EQUAL_UINT8(9, hdr.payload_len);
}

void test_decode_header_drops_0x06() {
  // msg_type=0x06 (unknown): H=(0x06<<9)|9=0x0C09 → [0x09, 0x0C]
  uint8_t frame[2] = {0x09, 0x0C};
  naviga::protocol::PacketHeader hdr{};
  TEST_ASSERT_FALSE(naviga::protocol::decode_header(frame, sizeof(frame), &hdr));
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

// ── Tail-1 RX: apply-on-match / drop-on-mismatch ────────────────────────────

// Helper: build a BeaconCore frame and inject it via on_rx to seed last_core_seq16.
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

  // Build Tail-1 frame with matching ref_core_seq16=7, own seq16=8, posFlags=0x01, sats=8.
  Tail1Fields t1{};
  t1.node_id          = node_id;
  t1.seq16            = 8;  // own global counter (newer than core_seq=7)
  t1.ref_core_seq16   = core_seq;
  t1.has_pos_flags    = true;
  t1.pos_flags     = 0x01;
  t1.has_sats      = true;
  t1.sats          = 8;
  uint8_t frame[kTail1FrameMax] = {};
  const size_t written = encode_tail1_frame(t1, frame, sizeof(frame));
  TEST_ASSERT_EQUAL_UINT32(kTail1FrameMax, written);

  PacketLogType ptype = PacketLogType::CORE;
  uint16_t out_core_seq = 0;
  uint16_t out_seq = 0;
  TEST_ASSERT_TRUE(logic.on_rx(2000, frame, written, -55, table,
                               nullptr, &out_seq, nullptr, &ptype, &out_core_seq));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PacketLogType::TAIL1), static_cast<int>(ptype));
  TEST_ASSERT_EQUAL_UINT16(8, out_seq);           // tail's own seq16
  TEST_ASSERT_EQUAL_UINT16(core_seq, out_core_seq); // ref_core_seq16

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

  // Build Tail-1 with stale ref_core_seq16=5 (mismatch), own seq16=8.
  Tail1Fields t1{};
  t1.node_id          = node_id;
  t1.seq16            = 8;
  t1.ref_core_seq16   = 5;  // != 7
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
  t1.node_id          = 0x0000AABBCCDDEEFFULL;
  t1.seq16            = 1;
  t1.ref_core_seq16   = 1;
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
  // Header: msg_type=0x03, payload_len=11 → [0x0B, 0x06]
  uint8_t frame[kTail1FrameMin] = {0x0B, 0x06,
                                    0x01,  // bad payloadVersion
                                    0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
                                    0x01, 0x00,  // seq16
                                    0x01, 0x00}; // ref_core_seq16
  TEST_ASSERT_FALSE(logic.on_rx(1000, frame, sizeof(frame), -55, table));
}

// ── Tail-1 RX: duplicate suppression ────────────────────────────────────────

void test_tail1_rx_duplicate_seq16_ignored() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;
  const uint16_t core_seq = 7;

  seed_core(logic, table, node_id, core_seq, 1000);

  // First Tail-1: seq16=8, ref=7, sats=8.
  Tail1Fields t1{};
  t1.node_id        = node_id;
  t1.seq16          = 8;
  t1.ref_core_seq16 = core_seq;
  t1.has_sats       = true;
  t1.sats           = 8;
  uint8_t frame[kTail1FrameMax] = {};
  encode_tail1_frame(t1, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(2000, frame, kTail1FrameMax, -55, table));

  // Second Tail-1: same seq16=8 (duplicate), different sats=99.
  t1.sats = 99;
  encode_tail1_frame(t1, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(2100, frame, kTail1FrameMax, -55, table));

  // sats MUST still be 8 (first apply wins; duplicate ignored).
  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_EQUAL_UINT8(8, entry.sats);
}

// ── Tail-2 (Operational) RX ──────────────────────────────────────────────────

void test_tail2_rx_applies_battery() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;

  Tail2Fields t2{};
  t2.node_id      = node_id;
  t2.seq16        = 5;
  t2.has_battery  = true;
  t2.battery_percent = 80;

  uint8_t frame[kTail2FrameMax] = {};
  const size_t written = encode_tail2_frame(t2, frame, sizeof(frame));

  PacketLogType ptype = PacketLogType::CORE;
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, written, -60, table,
                               nullptr, nullptr, nullptr, &ptype));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PacketLogType::TAIL2), static_cast<int>(ptype));

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_TRUE(entry.has_battery);
  TEST_ASSERT_EQUAL_UINT8(80, entry.battery_percent);
}

void test_tail2_rx_no_prior_core_creates_entry() {
  // Tail-2 can arrive before Core; it should create an entry.
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000112233445566ULL;

  Tail2Fields t2{};
  t2.node_id      = node_id;
  t2.seq16        = 3;
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
  // Header: msg_type=0x04, payload_len=9 → [0x09, 0x08]; bad payloadVersion=0x01
  uint8_t frame[kTail2FrameMin] = {0x09, 0x08,
                                    0x01,  // bad payloadVersion
                                    0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
                                    0x01, 0x00};
  TEST_ASSERT_FALSE(logic.on_rx(1000, frame, sizeof(frame), -60, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

// ── Info (Informative) RX ────────────────────────────────────────────────────

void test_info_rx_applies_max_silence() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;

  InfoFields info{};
  info.node_id         = node_id;
  info.seq16           = 6;
  info.has_max_silence = true;
  info.max_silence_10s = 9;  // 90 s

  uint8_t frame[kInfoFrameMax] = {};
  const size_t written = encode_info_frame(info, frame, sizeof(frame));

  PacketLogType ptype = PacketLogType::CORE;
  uint16_t out_seq = 0;
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, written, -60, table,
                               nullptr, &out_seq, nullptr, &ptype));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PacketLogType::INFO), static_cast<int>(ptype));
  TEST_ASSERT_EQUAL_UINT16(6, out_seq);

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_TRUE(entry.has_max_silence);
  TEST_ASSERT_EQUAL_UINT8(9, entry.max_silence_10s);
}

void test_info_rx_full_fields() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000112233445566ULL;

  InfoFields info{};
  info.node_id        = node_id;
  info.seq16          = 10;
  info.has_max_silence = true;
  info.max_silence_10s = 6;
  info.has_hw_profile  = true;
  info.hw_profile_id   = 0x0201;
  info.has_fw_version  = true;
  info.fw_version_id   = 0x0302;

  uint8_t frame[kInfoFrameMax] = {};
  const size_t written = encode_info_frame(info, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, written, -55, table));

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  TEST_ASSERT_TRUE(entry.has_max_silence);
  TEST_ASSERT_EQUAL_UINT8(6, entry.max_silence_10s);
  TEST_ASSERT_TRUE(entry.has_hw_profile);
  TEST_ASSERT_EQUAL_UINT16(0x0201, entry.hw_profile_id);
  TEST_ASSERT_TRUE(entry.has_fw_version);
  TEST_ASSERT_EQUAL_UINT16(0x0302, entry.fw_version_id);
  // Position MUST NOT have been set.
  TEST_ASSERT_FALSE(entry.pos_valid);
}

void test_info_rx_no_prior_core_creates_entry() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;

  InfoFields info{};
  info.node_id         = node_id;
  info.seq16           = 1;
  info.has_max_silence = true;
  info.max_silence_10s = 3;

  uint8_t frame[kInfoFrameMax] = {};
  const size_t written = encode_info_frame(info, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, written, -70, table));
  TEST_ASSERT_EQUAL_UINT32(1, table.size());
}

void test_info_rx_bad_version_dropped() {
  BeaconLogic logic;
  NodeTable table;
  // Header: msg_type=0x05, payload_len=9 → [0x09, 0x0A]; bad payloadVersion=0x01
  uint8_t frame[kInfoFrameMin] = {0x09, 0x0A,
                                   0x01,  // bad payloadVersion
                                   0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
                                   0x01, 0x00};
  TEST_ASSERT_FALSE(logic.on_rx(1000, frame, sizeof(frame), -60, table));
  TEST_ASSERT_EQUAL_UINT32(0, table.size());
}

// ── RX dedupe: (nodeId48, seq16) ─────────────────────────────────────────────

void test_rx_dedupe_operational_same_seq16_ignored() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;

  // First Tail-2: seq16=5, battery=80.
  Tail2Fields t2{};
  t2.node_id         = node_id;
  t2.seq16           = 5;
  t2.has_battery     = true;
  t2.battery_percent = 80;
  uint8_t frame[kTail2FrameMax] = {};
  encode_tail2_frame(t2, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, kTail2FrameMin + 1, -60, table));

  // Second Tail-2: same seq16=5, battery=99 (duplicate).
  t2.battery_percent = 99;
  encode_tail2_frame(t2, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(1100, frame, kTail2FrameMin + 1, -60, table));

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  // battery MUST still be 80 (first apply wins; duplicate ignored).
  TEST_ASSERT_EQUAL_UINT8(80, entry.battery_percent);
}

void test_rx_dedupe_info_same_seq16_ignored() {
  BeaconLogic logic;
  NodeTable table;
  const uint64_t node_id = 0x0000AABBCCDDEEFFULL;

  // First Info: seq16=3, maxSilence=6.
  InfoFields info{};
  info.node_id         = node_id;
  info.seq16           = 3;
  info.has_max_silence = true;
  info.max_silence_10s = 6;
  uint8_t frame[kInfoFrameMax] = {};
  encode_info_frame(info, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(1000, frame, kInfoFrameMin + 1, -60, table));

  // Second Info: same seq16=3, maxSilence=99 (duplicate).
  info.max_silence_10s = 99;
  encode_info_frame(info, frame, sizeof(frame));
  TEST_ASSERT_TRUE(logic.on_rx(1100, frame, kInfoFrameMin + 1, -60, table));

  NodeEntry entry{};
  TEST_ASSERT_TRUE(table.find_entry_for_test(node_id, &entry));
  // maxSilence MUST still be 6 (first apply wins; duplicate ignored).
  TEST_ASSERT_EQUAL_UINT8(6, entry.max_silence_10s);
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
  t1.node_id          = node_id;
  t1.seq16            = 4;  // newer than core_seq=3
  t1.ref_core_seq16   = core_seq;
  t1.has_pos_flags    = true;
  t1.pos_flags     = 0x01;
  uint8_t t1_frame[kTail1FrameMax] = {};
  encode_tail1_frame(t1, t1_frame, sizeof(t1_frame));
  logic.on_rx(2000, t1_frame, kTail1FrameMax, -55, table);

  // Apply Tail-2 (Operational).
  Tail2Fields t2{};
  t2.node_id     = node_id;
  t2.seq16       = 5;
  t2.has_battery = true;
  t2.battery_percent = 90;
  uint8_t t2_frame[kTail2FrameMax] = {};
  const size_t t2_written = encode_tail2_frame(t2, t2_frame, sizeof(t2_frame));
  logic.on_rx(3000, t2_frame, t2_written, -60, table);

  // Apply Info (Informative).
  InfoFields info{};
  info.node_id         = node_id;
  info.seq16           = 6;
  info.has_max_silence = true;
  info.max_silence_10s = 9;
  uint8_t info_frame[kInfoFrameMax] = {};
  const size_t info_written = encode_info_frame(info, info_frame, sizeof(info_frame));
  logic.on_rx(4000, info_frame, info_written, -60, table);

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
  // decode_header 0x05
  RUN_TEST(test_decode_header_accepts_0x05);
  RUN_TEST(test_decode_header_drops_0x06);
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
  // Tail-1 RX dispatch
  RUN_TEST(test_tail1_rx_apply_on_match);
  RUN_TEST(test_tail1_rx_drop_on_mismatch);
  RUN_TEST(test_tail1_rx_drop_no_prior_core);
  RUN_TEST(test_tail1_rx_bad_version_dropped);
  RUN_TEST(test_tail1_rx_duplicate_seq16_ignored);
  // Tail-2 RX dispatch
  RUN_TEST(test_tail2_rx_applies_battery);
  RUN_TEST(test_tail2_rx_no_prior_core_creates_entry);
  RUN_TEST(test_tail2_rx_bad_version_dropped);
  // Info RX dispatch
  RUN_TEST(test_info_rx_applies_max_silence);
  RUN_TEST(test_info_rx_full_fields);
  RUN_TEST(test_info_rx_no_prior_core_creates_entry);
  RUN_TEST(test_info_rx_bad_version_dropped);
  // Dedupe
  RUN_TEST(test_rx_dedupe_operational_same_seq16_ignored);
  RUN_TEST(test_rx_dedupe_info_same_seq16_ignored);
  // Position invariant
  RUN_TEST(test_tail_never_overwrites_position);
  return UNITY_END();
}
