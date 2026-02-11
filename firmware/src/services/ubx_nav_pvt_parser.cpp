#include "services/ubx_nav_pvt_parser.h"

namespace naviga {

constexpr uint8_t UbxStreamParser::kSync1;
constexpr uint8_t UbxStreamParser::kSync2;
constexpr uint8_t UbxStreamParser::kClassNav;
constexpr uint8_t UbxStreamParser::kIdNavPvt;
constexpr uint16_t UbxStreamParser::kNavPvtPayloadLen;
constexpr size_t UbxStreamParser::kMaxPayloadLen;

namespace {

inline int32_t read_i32_le(const uint8_t* p) {
  const uint32_t raw = static_cast<uint32_t>(p[0]) |
                       (static_cast<uint32_t>(p[1]) << 8) |
                       (static_cast<uint32_t>(p[2]) << 16) |
                       (static_cast<uint32_t>(p[3]) << 24);
  return static_cast<int32_t>(raw);
}

} // namespace

void UbxStreamParser::reset() {
  state_ = State::kWaitSync1;
  msg_class_ = 0;
  msg_id_ = 0;
  payload_len_ = 0;
  payload_pos_ = 0;
  ck_a_ = 0;
  ck_b_ = 0;
  rx_ck_a_ = 0;
}

void UbxStreamParser::checksum_accumulate(uint8_t v) {
  ck_a_ = static_cast<uint8_t>(ck_a_ + v);
  ck_b_ = static_cast<uint8_t>(ck_b_ + ck_a_);
}

UbxParseStatus UbxStreamParser::push_byte(uint8_t byte, UbxFrameView* out_frame) {
  if (!out_frame) {
    reset();
    return UbxParseStatus::None;
  }

  switch (state_) {
    case State::kWaitSync1:
      if (byte == kSync1) {
        state_ = State::kWaitSync2;
      }
      return UbxParseStatus::None;

    case State::kWaitSync2:
      if (byte == kSync2) {
        state_ = State::kClass;
        ck_a_ = 0;
        ck_b_ = 0;
      } else {
        state_ = (byte == kSync1) ? State::kWaitSync2 : State::kWaitSync1;
      }
      return UbxParseStatus::None;

    case State::kClass:
      msg_class_ = byte;
      checksum_accumulate(byte);
      state_ = State::kId;
      return UbxParseStatus::None;

    case State::kId:
      msg_id_ = byte;
      checksum_accumulate(byte);
      state_ = State::kLen1;
      return UbxParseStatus::None;

    case State::kLen1:
      payload_len_ = byte;
      checksum_accumulate(byte);
      state_ = State::kLen2;
      return UbxParseStatus::None;

    case State::kLen2:
      payload_len_ |= static_cast<uint16_t>(byte) << 8;
      checksum_accumulate(byte);
      if (payload_len_ > kMaxPayloadLen) {
        reset();
        return UbxParseStatus::None;
      }
      payload_pos_ = 0;
      state_ = (payload_len_ == 0) ? State::kCkA : State::kPayload;
      return UbxParseStatus::None;

    case State::kPayload:
      payload_[payload_pos_++] = byte;
      checksum_accumulate(byte);
      if (payload_pos_ >= payload_len_) {
        state_ = State::kCkA;
      }
      return UbxParseStatus::None;

    case State::kCkA:
      rx_ck_a_ = byte;
      state_ = State::kCkB;
      return UbxParseStatus::None;

    case State::kCkB: {
      const bool ck_ok = (rx_ck_a_ == ck_a_) && (byte == ck_b_);
      if (ck_ok) {
        out_frame->msg_class = msg_class_;
        out_frame->msg_id = msg_id_;
        out_frame->payload = payload_;
        out_frame->payload_len = payload_len_;
      }
      reset();
      return ck_ok ? UbxParseStatus::FrameOk : UbxParseStatus::FrameBadChecksum;
    }
  }

  reset();
  return UbxParseStatus::None;
}

bool parse_nav_pvt_fix_lat_lon(const UbxFrameView& frame,
                               uint8_t* out_fix_type,
                               int32_t* out_lat_e7,
                               int32_t* out_lon_e7) {
  if (!out_fix_type || !out_lat_e7 || !out_lon_e7) {
    return false;
  }
  if (frame.msg_class != UbxStreamParser::kClassNav || frame.msg_id != UbxStreamParser::kIdNavPvt) {
    return false;
  }
  if (!frame.payload || frame.payload_len != UbxStreamParser::kNavPvtPayloadLen) {
    return false;
  }

  // UBX-NAV-PVT offsets in payload (u-blox 8):
  // fixType @ 20, lon @ 24, lat @ 28.
  const uint8_t* p = frame.payload;
  *out_fix_type = p[20];
  *out_lon_e7 = read_i32_le(p + 24);
  *out_lat_e7 = read_i32_le(p + 28);
  return true;
}

} // namespace naviga
