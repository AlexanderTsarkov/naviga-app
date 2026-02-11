#include <unity.h>

#include <cstdint>
#include <vector>

#include "../../src/services/ubx_nav_pvt_parser.h"
#include "../../src/services/ubx_nav_pvt_parser.cpp"

using naviga::UbxFrameView;
using naviga::UbxParseStatus;
using naviga::UbxStreamParser;
using naviga::parse_nav_pvt_fix_lat_lon;

namespace {

std::vector<uint8_t> make_nav_pvt_frame(uint8_t fix_type, int32_t lat_e7, int32_t lon_e7) {
  std::vector<uint8_t> frame;
  frame.reserve(2 + 4 + UbxStreamParser::kNavPvtPayloadLen + 2);
  frame.push_back(UbxStreamParser::kSync1);
  frame.push_back(UbxStreamParser::kSync2);
  frame.push_back(UbxStreamParser::kClassNav);
  frame.push_back(UbxStreamParser::kIdNavPvt);
  frame.push_back(static_cast<uint8_t>(UbxStreamParser::kNavPvtPayloadLen & 0xFF));
  frame.push_back(static_cast<uint8_t>((UbxStreamParser::kNavPvtPayloadLen >> 8) & 0xFF));

  std::vector<uint8_t> payload(UbxStreamParser::kNavPvtPayloadLen, 0);
  payload[20] = fix_type;
  payload[24] = static_cast<uint8_t>(lon_e7 & 0xFF);
  payload[25] = static_cast<uint8_t>((lon_e7 >> 8) & 0xFF);
  payload[26] = static_cast<uint8_t>((lon_e7 >> 16) & 0xFF);
  payload[27] = static_cast<uint8_t>((lon_e7 >> 24) & 0xFF);
  payload[28] = static_cast<uint8_t>(lat_e7 & 0xFF);
  payload[29] = static_cast<uint8_t>((lat_e7 >> 8) & 0xFF);
  payload[30] = static_cast<uint8_t>((lat_e7 >> 16) & 0xFF);
  payload[31] = static_cast<uint8_t>((lat_e7 >> 24) & 0xFF);

  uint8_t ck_a = 0;
  uint8_t ck_b = 0;
  for (size_t i = 2; i < frame.size(); ++i) {
    ck_a = static_cast<uint8_t>(ck_a + frame[i]);
    ck_b = static_cast<uint8_t>(ck_b + ck_a);
  }
  for (uint8_t b : payload) {
    frame.push_back(b);
    ck_a = static_cast<uint8_t>(ck_a + b);
    ck_b = static_cast<uint8_t>(ck_b + ck_a);
  }
  frame.push_back(ck_a);
  frame.push_back(ck_b);
  return frame;
}

} // namespace

void test_parse_nav_pvt_valid_frame() {
  const int32_t lat = 571844000;
  const int32_t lon = 383663000;
  std::vector<uint8_t> bytes = make_nav_pvt_frame(3, lat, lon);

  UbxStreamParser parser;
  UbxFrameView frame{};
  UbxParseStatus status = UbxParseStatus::None;
  for (uint8_t b : bytes) {
    status = parser.push_byte(b, &frame);
  }

  TEST_ASSERT_EQUAL(static_cast<int>(UbxParseStatus::FrameOk), static_cast<int>(status));
  uint8_t fix_type = 0;
  int32_t out_lat = 0;
  int32_t out_lon = 0;
  TEST_ASSERT_TRUE(parse_nav_pvt_fix_lat_lon(frame, &fix_type, &out_lat, &out_lon));
  TEST_ASSERT_EQUAL_UINT8(3, fix_type);
  TEST_ASSERT_EQUAL_INT32(lat, out_lat);
  TEST_ASSERT_EQUAL_INT32(lon, out_lon);
}

void test_parse_nav_pvt_bad_checksum() {
  std::vector<uint8_t> bytes = make_nav_pvt_frame(2, 100, 200);
  bytes.back() ^= 0xFF;

  UbxStreamParser parser;
  UbxFrameView frame{};
  UbxParseStatus status = UbxParseStatus::None;
  for (uint8_t b : bytes) {
    status = parser.push_byte(b, &frame);
  }

  TEST_ASSERT_EQUAL(static_cast<int>(UbxParseStatus::FrameBadChecksum), static_cast<int>(status));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_nav_pvt_valid_frame);
  RUN_TEST(test_parse_nav_pvt_bad_checksum);
  return UNITY_END();
}
