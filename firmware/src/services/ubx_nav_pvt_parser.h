#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {

struct UbxFrameView {
  uint8_t msg_class = 0;
  uint8_t msg_id = 0;
  const uint8_t* payload = nullptr;
  uint16_t payload_len = 0;
};

enum class UbxParseStatus : uint8_t {
  None = 0,
  FrameOk,
  FrameBadChecksum,
};

class UbxStreamParser {
 public:
  static constexpr uint8_t kSync1 = 0xB5;
  static constexpr uint8_t kSync2 = 0x62;
  static constexpr uint8_t kClassNav = 0x01;
  static constexpr uint8_t kIdNavPvt = 0x07;
  static constexpr uint16_t kNavPvtPayloadLen = 92;
  static constexpr size_t kMaxPayloadLen = 128;

  UbxParseStatus push_byte(uint8_t byte, UbxFrameView* out_frame);

 private:
  enum class State : uint8_t {
    kWaitSync1 = 0,
    kWaitSync2,
    kClass,
    kId,
    kLen1,
    kLen2,
    kPayload,
    kCkA,
    kCkB,
  };

  void reset();
  void checksum_accumulate(uint8_t v);

  State state_ = State::kWaitSync1;
  uint8_t msg_class_ = 0;
  uint8_t msg_id_ = 0;
  uint16_t payload_len_ = 0;
  uint16_t payload_pos_ = 0;
  uint8_t payload_[kMaxPayloadLen] = {};
  uint8_t ck_a_ = 0;
  uint8_t ck_b_ = 0;
  uint8_t rx_ck_a_ = 0;
};

bool parse_nav_pvt_fix_lat_lon(const UbxFrameView& frame,
                               uint8_t* out_fix_type,
                               int32_t* out_lat_e7,
                               int32_t* out_lon_e7);

} // namespace naviga
