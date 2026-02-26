#include <unity.h>

#include <cmath>
#include <cstdint>

#include "../../protocol/packet_header.h"
#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/geo_beacon_codec.cpp"
#include "../../protocol/alive_codec.h"
#include "../../protocol/alive_codec.cpp"

using naviga::protocol::AliveDecodeError;
using naviga::protocol::AliveFields;
using naviga::protocol::ByteSpan;
using naviga::protocol::ConstByteSpan;
using naviga::protocol::DecodeError;
using naviga::protocol::GeoBeaconFields;
using naviga::protocol::MsgType;
using naviga::protocol::PacketHeader;
using naviga::protocol::decode_geo_beacon;
using naviga::protocol::decode_geo_beacon_frame;
using naviga::protocol::decode_header;
using naviga::protocol::encode_geo_beacon;
using naviga::protocol::encode_header;
using naviga::protocol::kGeoBeaconFrameSize;
using naviga::protocol::kGeoBeaconSize;
using naviga::protocol::kGeoBeaconPayloadVersion;
using naviga::protocol::kHeaderSize;
using naviga::protocol::kAliveFrameMin;
using naviga::protocol::kAliveFrameMax;
using naviga::protocol::kAlivePayloadMin;
using naviga::protocol::decode_alive_payload;
using naviga::protocol::encode_alive_frame;
using naviga::protocol::validate_header;

static constexpr double kDegTol = 0.00003;

static bool deg_near(double a, double b, double tol) {
  return (a - b) < tol && (b - a) < tol;
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 1: Packet header encode/decode golden vectors
// ═══════════════════════════════════════════════════════════════════════════

// Golden vector from docs/protocols/ootb_radio_v0.md §3.1:
//   msg_type=0x01, reserved=0, payload_len=15
//   H = 0x020F  →  wire [0x0F, 0x02]
void test_header_golden_encode() {
  PacketHeader hdr;
  hdr.msg_type    = MsgType::BeaconCore;
  hdr.reserved    = 0;
  hdr.payload_len = 15;

  uint8_t buf[2] = {};
  TEST_ASSERT_TRUE(encode_header(hdr, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_HEX8(0x0F, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0x02, buf[1]);
}

void test_header_golden_decode() {
  const uint8_t wire[2] = {0x0F, 0x02};
  PacketHeader hdr;
  TEST_ASSERT_TRUE(decode_header(wire, sizeof(wire), &hdr));
  TEST_ASSERT_EQUAL(static_cast<int>(MsgType::BeaconCore), static_cast<int>(hdr.msg_type));
  TEST_ASSERT_EQUAL_UINT8(0, hdr.reserved);
  TEST_ASSERT_EQUAL_UINT8(15, hdr.payload_len);
}

// Encode/decode round-trip for all registered msg_types.
void test_header_roundtrip_all_types() {
  const MsgType types[] = {
      MsgType::BeaconCore,
      MsgType::BeaconAlive,
      MsgType::BeaconTail1,
      MsgType::BeaconTail2,
  };
  for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
    PacketHeader hdr;
    hdr.msg_type    = types[i];
    hdr.reserved    = 0;
    hdr.payload_len = static_cast<uint8_t>(i + 7u);

    uint8_t buf[2] = {};
    TEST_ASSERT_TRUE(encode_header(hdr, buf, sizeof(buf)));

    PacketHeader out;
    TEST_ASSERT_TRUE(decode_header(buf, sizeof(buf), &out));
    TEST_ASSERT_EQUAL(static_cast<int>(types[i]), static_cast<int>(out.msg_type));
    TEST_ASSERT_EQUAL_UINT8(0, out.reserved);
    TEST_ASSERT_EQUAL_UINT8(hdr.payload_len, out.payload_len);
  }
}

// Reserved msg_type (0x00) must be rejected on decode.
void test_header_decode_reserved_msgtype_rejected() {
  const uint8_t wire[2] = {0x00, 0x00};  // msg_type=0x00
  PacketHeader hdr;
  TEST_ASSERT_FALSE(decode_header(wire, sizeof(wire), &hdr));
}

// Unknown msg_type (> BeaconTail2) must be rejected on decode.
void test_header_decode_unknown_msgtype_rejected() {
  // msg_type=0x7F (127), payload_len=9:
  // H = (0x7F << 9) | 9 = 0xFF09  →  byte0=0x09, byte1=0xFF
  const uint8_t wire[2] = {0x09, 0xFF};
  PacketHeader hdr;
  TEST_ASSERT_FALSE(decode_header(wire, sizeof(wire), &hdr));
}

// Non-zero reserved bits must be accepted (forward-compat).
void test_header_decode_nonzero_reserved_accepted() {
  // Encode manually with reserved=0x07 (all 3 bits set):
  // byte0 = ((0x07 & 0x3) << 6) | 9 = (3<<6)|9 = 0xC9
  // byte1 = (0x01 << 1) | (0x07 >> 2) = 2 | 1 = 0x03
  const uint8_t wire[2] = {0xC9, 0x03};
  PacketHeader hdr;
  TEST_ASSERT_TRUE(decode_header(wire, sizeof(wire), &hdr));
  TEST_ASSERT_EQUAL(static_cast<int>(MsgType::BeaconCore), static_cast<int>(hdr.msg_type));
  TEST_ASSERT_EQUAL_UINT8(9, hdr.payload_len);
}

// validate_header: match and mismatch.
void test_header_validate() {
  PacketHeader hdr;
  hdr.msg_type    = MsgType::BeaconCore;
  hdr.reserved    = 0;
  hdr.payload_len = 15;
  TEST_ASSERT_TRUE(validate_header(hdr, 15));
  TEST_ASSERT_FALSE(validate_header(hdr, 14));
  TEST_ASSERT_FALSE(validate_header(hdr, 16));
}

// payload_len > 63 must be rejected on encode.
void test_header_encode_payload_len_overflow_rejected() {
  PacketHeader hdr;
  hdr.msg_type    = MsgType::BeaconCore;
  hdr.reserved    = 0;
  hdr.payload_len = 64;  // > kMaxPayloadLen
  uint8_t buf[2] = {};
  TEST_ASSERT_FALSE(encode_header(hdr, buf, sizeof(buf)));
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 2: BeaconCore framed encode/decode golden vectors
// ═══════════════════════════════════════════════════════════════════════════

// Full framed BeaconCore golden vector (17 bytes):
//   Header [0x0F, 0x02] + payload [00 FF EE DD CC BB AA 01 00 10 4C CF 05 C0 9A]
//   From beacon_payload_encoding_v0.md §5.1 + ootb_radio_v0.md §3.1
void test_framed_core_golden_encode() {
  GeoBeaconFields in{};
  in.node_id   = 0x0000AABBCCDDEEFFULL;
  in.pos_valid = 1;
  in.lat_deg   = 55.7558;
  in.lon_deg   = 37.6173;
  in.seq       = 1;

  uint8_t buf[kGeoBeaconFrameSize] = {};
  const size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, written);

  // Header bytes.
  TEST_ASSERT_EQUAL_HEX8(0x0F, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0x02, buf[1]);

  // Payload bytes (canonical from doc §5.1).
  const uint8_t expected_payload[15] = {
      0x00,                               // payloadVersion
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, // nodeId48 LE
      0x01, 0x00,                          // seq16 LE = 1
      0x10, 0x4C, 0xCF,                    // lat_u24 LE = 13585424
      0x05, 0xC0, 0x9A                     // lon_u24 LE = 10141701
  };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_payload, buf + kHeaderSize, kGeoBeaconSize);
}

void test_framed_core_golden_decode() {
  const uint8_t frame[17] = {
      0x0F, 0x02,                          // header
      0x00,                                // payloadVersion
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, // nodeId48 LE
      0x01, 0x00,                          // seq16 LE = 1
      0x10, 0x4C, 0xCF,                    // lat_u24 LE
      0x05, 0xC0, 0x9A                     // lon_u24 LE
  };

  GeoBeaconFields out{};
  const DecodeError err = decode_geo_beacon_frame(ConstByteSpan{frame, sizeof(frame)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(0x0000AABBCCDDEEFFULL, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(1, out.seq);
  TEST_ASSERT_EQUAL_UINT8(1, out.pos_valid);
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, 55.7558, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, 37.6173, kDegTol));
}

// decode_geo_beacon_frame rejects wrong msg_type.
void test_framed_core_decode_wrong_msgtype_rejected() {
  uint8_t frame[17] = {};
  // Set msg_type=0x02 (BeaconAlive) in header: payload_len=15 → H=(0x02<<9)|15=0x042F
  // byte0=0x2F, byte1=0x04
  frame[0] = 0x2F;
  frame[1] = 0x04;
  frame[2] = kGeoBeaconPayloadVersion;
  GeoBeaconFields out{};
  const DecodeError err = decode_geo_beacon_frame(ConstByteSpan{frame, sizeof(frame)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::BadMsgType, err);
}

// decode_geo_beacon_frame rejects payload_len mismatch.
void test_framed_core_decode_payload_len_mismatch_rejected() {
  // Build a valid 17-byte frame but with payload_len=14 in header (wrong).
  // H = (0x01<<9)|14 = 0x020E → byte0=0x0E, byte1=0x02
  uint8_t frame[17] = {};
  frame[0] = 0x0E;
  frame[1] = 0x02;
  frame[2] = kGeoBeaconPayloadVersion;
  GeoBeaconFields out{};
  const DecodeError err = decode_geo_beacon_frame(ConstByteSpan{frame, sizeof(frame)}, &out);
  TEST_ASSERT_EQUAL(DecodeError::PayloadLenMismatch, err);
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 3: Payload-only encode/decode (unchanged semantics, smaller buffer)
// ═══════════════════════════════════════════════════════════════════════════

void test_payload_golden_vector_encode() {
  GeoBeaconFields in{};
  in.node_id   = 0x0000AABBCCDDEEFFULL;
  in.pos_valid = 1;
  in.lat_deg   = 55.7558;
  in.lon_deg   = 37.6173;
  in.seq       = 1;

  // encode_geo_beacon now requires kGeoBeaconFrameSize buffer.
  uint8_t buf[kGeoBeaconFrameSize] = {};
  const size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, written);

  const uint8_t expected[15] = {
      0x00,
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
      0x01, 0x00,
      0x10, 0x4C, 0xCF,
      0x05, 0xC0, 0x9A
  };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf + kHeaderSize, kGeoBeaconSize);
}

void test_payload_golden_vector_decode() {
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

void test_round_trip() {
  GeoBeaconFields in{};
  in.node_id   = 0x0000AABBCCDDEEFFULL;
  in.pos_valid = 1;
  in.lat_deg   = 55.7558;
  in.lon_deg   = 37.6173;
  in.seq       = 4567;

  uint8_t buf[kGeoBeaconFrameSize] = {};
  const size_t written = encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)});
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, written);

  GeoBeaconFields out{};
  const DecodeError err = decode_geo_beacon_frame(ConstByteSpan{buf, written}, &out);
  TEST_ASSERT_EQUAL(DecodeError::Ok, err);
  TEST_ASSERT_EQUAL_UINT64(in.node_id, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(in.seq, out.seq);
  TEST_ASSERT_EQUAL_UINT8(1, out.pos_valid);
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, in.lat_deg, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, in.lon_deg, kDegTol));
}

void test_round_trip_extremes() {
  GeoBeaconFields in{};
  in.node_id   = 42;
  in.pos_valid = 1;
  in.seq       = 1;

  uint8_t buf[kGeoBeaconFrameSize] = {};
  GeoBeaconFields out{};

  in.lat_deg = 90.0; in.lon_deg = 180.0;
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon_frame(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, 90.0, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, 180.0, kDegTol));

  in.lat_deg = -90.0; in.lon_deg = -180.0;
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon_frame(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, -90.0, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, -180.0, kDegTol));

  in.lat_deg = 0.0; in.lon_deg = 0.0;
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon_frame(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, 0.0, kDegTol));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, 0.0, kDegTol));
}

void test_clamp_out_of_range_lat() {
  GeoBeaconFields in{};
  in.node_id = 1; in.pos_valid = 1; in.lat_deg = 200.0; in.lon_deg = 0.0; in.seq = 1;
  uint8_t buf[kGeoBeaconFrameSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon_frame(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lat_deg, 90.0, kDegTol));
}

void test_clamp_out_of_range_lon() {
  GeoBeaconFields in{};
  in.node_id = 1; in.pos_valid = 1; in.lat_deg = 0.0; in.lon_deg = -500.0; in.seq = 1;
  uint8_t buf[kGeoBeaconFrameSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon_frame(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_TRUE(deg_near(out.lon_deg, -180.0, kDegTol));
}

void test_encode_no_fix_returns_zero() {
  GeoBeaconFields in{};
  in.node_id = 1; in.pos_valid = 0; in.lat_deg = 55.7558; in.lon_deg = 37.6173; in.seq = 1;
  uint8_t buf[kGeoBeaconFrameSize] = {};
  TEST_ASSERT_EQUAL_UINT32(0, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
}

void test_encode_short_buffer() {
  GeoBeaconFields in{};
  in.pos_valid = 1;
  uint8_t buf[kGeoBeaconFrameSize - 1] = {};
  TEST_ASSERT_EQUAL_UINT32(0, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
}

void test_decode_short_buffer() {
  uint8_t buf[kGeoBeaconSize - 1] = {};
  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::ShortBuffer,
                    decode_geo_beacon(ConstByteSpan{buf, sizeof(buf)}, &out));
}

void test_bad_payload_version() {
  GeoBeaconFields in{};
  in.node_id = 1; in.pos_valid = 1; in.lat_deg = 0.0; in.lon_deg = 0.0; in.seq = 0;
  uint8_t buf[kGeoBeaconFrameSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  buf[kHeaderSize] = 0xFF;  // corrupt payloadVersion
  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::BadPayloadVersion,
                    decode_geo_beacon_frame(ConstByteSpan{buf, sizeof(buf)}, &out));
}

void test_nodeid48_upper_bits_stripped() {
  GeoBeaconFields in{};
  in.node_id = 0xDEAD001122334455ULL;
  in.pos_valid = 1; in.lat_deg = 0.0; in.lon_deg = 0.0; in.seq = 0;
  uint8_t buf[kGeoBeaconFrameSize] = {};
  TEST_ASSERT_EQUAL_UINT32(kGeoBeaconFrameSize, encode_geo_beacon(in, ByteSpan{buf, sizeof(buf)}));
  GeoBeaconFields out{};
  TEST_ASSERT_EQUAL(DecodeError::Ok, decode_geo_beacon_frame(ConstByteSpan{buf, sizeof(buf)}, &out));
  TEST_ASSERT_EQUAL_UINT64(0x0000001122334455ULL, out.node_id);
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 4: Alive codec golden vectors
// ═══════════════════════════════════════════════════════════════════════════

// Alive min frame (11 bytes): header [0x29, 0x04] + payload (9 bytes)
//   msg_type=0x02, payload_len=9 → H=(0x02<<9)|9 = 0x0409 → [0x09, 0x04]
void test_alive_golden_encode_min() {
  AliveFields in{};
  in.node_id    = 0x0000AABBCCDDEEFFULL;
  in.seq        = 1;
  in.has_status = false;

  uint8_t buf[kAliveFrameMin] = {};
  const size_t written = encode_alive_frame(in, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_UINT32(kAliveFrameMin, written);

  // Header: msg_type=0x02, payload_len=9 → H=0x0409 → [0x09, 0x04]
  TEST_ASSERT_EQUAL_HEX8(0x09, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0x04, buf[1]);

  // Payload: payloadVersion=0x00, nodeId48 LE, seq16 LE
  const uint8_t expected_payload[9] = {
      0x00,                               // payloadVersion
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, // nodeId48 LE
      0x01, 0x00                           // seq16 LE = 1
  };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_payload, buf + kHeaderSize, kAlivePayloadMin);
}

// Alive with aliveStatus (12 bytes): header [0x0A, 0x04] + payload (10 bytes)
//   msg_type=0x02, payload_len=10 → H=(0x02<<9)|10 = 0x040A → [0x0A, 0x04]
void test_alive_golden_encode_with_status() {
  AliveFields in{};
  in.node_id      = 0x0000AABBCCDDEEFFULL;
  in.seq          = 2;
  in.has_status   = true;
  in.alive_status = 0x00;

  uint8_t buf[kAliveFrameMax] = {};
  const size_t written = encode_alive_frame(in, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_UINT32(kAliveFrameMax, written);

  TEST_ASSERT_EQUAL_HEX8(0x0A, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0x04, buf[1]);

  const uint8_t expected_payload[10] = {
      0x00,
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
      0x02, 0x00,  // seq16 LE = 2
      0x00         // aliveStatus
  };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_payload, buf + kHeaderSize, 10);
}

void test_alive_decode_min() {
  const uint8_t payload[9] = {
      0x00,
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
      0x01, 0x00
  };
  AliveFields out{};
  TEST_ASSERT_EQUAL(AliveDecodeError::Ok,
                    decode_alive_payload(payload, sizeof(payload), &out));
  TEST_ASSERT_EQUAL_UINT64(0x0000AABBCCDDEEFFULL, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(1, out.seq);
  TEST_ASSERT_FALSE(out.has_status);
}

void test_alive_decode_with_status() {
  const uint8_t payload[10] = {
      0x00,
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,
      0x02, 0x00,
      0x00
  };
  AliveFields out{};
  TEST_ASSERT_EQUAL(AliveDecodeError::Ok,
                    decode_alive_payload(payload, sizeof(payload), &out));
  TEST_ASSERT_EQUAL_UINT64(0x0000AABBCCDDEEFFULL, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(2, out.seq);
  TEST_ASSERT_TRUE(out.has_status);
  TEST_ASSERT_EQUAL_UINT8(0x00, out.alive_status);
}

void test_alive_decode_short_buffer() {
  const uint8_t payload[8] = {};
  AliveFields out{};
  TEST_ASSERT_EQUAL(AliveDecodeError::ShortBuffer,
                    decode_alive_payload(payload, sizeof(payload), &out));
}

void test_alive_decode_bad_payload_version() {
  uint8_t payload[9] = {0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x01, 0x00};
  payload[0] = 0xFF;
  AliveFields out{};
  TEST_ASSERT_EQUAL(AliveDecodeError::BadPayloadVersion,
                    decode_alive_payload(payload, sizeof(payload), &out));
}

// Alive round-trip: encode frame → decode header → decode payload.
void test_alive_roundtrip() {
  AliveFields in{};
  in.node_id    = 0x0000334455667788ULL;
  in.seq        = 99;
  in.has_status = false;

  uint8_t buf[kAliveFrameMin] = {};
  TEST_ASSERT_EQUAL_UINT32(kAliveFrameMin, encode_alive_frame(in, buf, sizeof(buf)));

  // Decode header.
  naviga::protocol::PacketHeader hdr;
  TEST_ASSERT_TRUE(decode_header(buf, sizeof(buf), &hdr));
  TEST_ASSERT_EQUAL(static_cast<int>(naviga::protocol::MsgType::BeaconAlive),
                    static_cast<int>(hdr.msg_type));
  TEST_ASSERT_EQUAL_UINT8(kAlivePayloadMin, hdr.payload_len);
  TEST_ASSERT_TRUE(naviga::protocol::validate_header(hdr, sizeof(buf) - kHeaderSize));

  // Decode payload.
  AliveFields out{};
  TEST_ASSERT_EQUAL(AliveDecodeError::Ok,
                    decode_alive_payload(buf + kHeaderSize, hdr.payload_len, &out));
  TEST_ASSERT_EQUAL_UINT64(in.node_id, out.node_id);
  TEST_ASSERT_EQUAL_UINT16(in.seq, out.seq);
  TEST_ASSERT_FALSE(out.has_status);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  // Header golden vectors and rules
  RUN_TEST(test_header_golden_encode);
  RUN_TEST(test_header_golden_decode);
  RUN_TEST(test_header_roundtrip_all_types);
  RUN_TEST(test_header_decode_reserved_msgtype_rejected);
  RUN_TEST(test_header_decode_unknown_msgtype_rejected);
  RUN_TEST(test_header_decode_nonzero_reserved_accepted);
  RUN_TEST(test_header_validate);
  RUN_TEST(test_header_encode_payload_len_overflow_rejected);

  // Framed BeaconCore golden vectors
  RUN_TEST(test_framed_core_golden_encode);
  RUN_TEST(test_framed_core_golden_decode);
  RUN_TEST(test_framed_core_decode_wrong_msgtype_rejected);
  RUN_TEST(test_framed_core_decode_payload_len_mismatch_rejected);

  // BeaconCore payload semantics (unchanged)
  RUN_TEST(test_payload_golden_vector_encode);
  RUN_TEST(test_payload_golden_vector_decode);
  RUN_TEST(test_round_trip);
  RUN_TEST(test_round_trip_extremes);
  RUN_TEST(test_clamp_out_of_range_lat);
  RUN_TEST(test_clamp_out_of_range_lon);
  RUN_TEST(test_encode_no_fix_returns_zero);
  RUN_TEST(test_encode_short_buffer);
  RUN_TEST(test_decode_short_buffer);
  RUN_TEST(test_bad_payload_version);
  RUN_TEST(test_nodeid48_upper_bits_stripped);

  // Alive codec golden vectors
  RUN_TEST(test_alive_golden_encode_min);
  RUN_TEST(test_alive_golden_encode_with_status);
  RUN_TEST(test_alive_decode_min);
  RUN_TEST(test_alive_decode_with_status);
  RUN_TEST(test_alive_decode_short_buffer);
  RUN_TEST(test_alive_decode_bad_payload_version);
  RUN_TEST(test_alive_roundtrip);

  return UNITY_END();
}
