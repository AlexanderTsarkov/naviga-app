#include "ble_status_bridge.h"

#include <array>

namespace naviga {
namespace protocol {

namespace {

constexpr size_t kHeaderBytes = 4;
constexpr size_t kTlvHeaderBytes = 2;
constexpr size_t kGnssValueBytes = 4;
constexpr size_t kStatusPayloadBytes = kHeaderBytes + kTlvHeaderBytes + kGnssValueBytes;

inline void write_u16_le(uint8_t* out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

} // namespace

bool BleStatusBridge::update_status(uint32_t now_ms,
                                    const GnssSnapshot& gnss_snapshot,
                                    IBleTransport& transport) const {
  std::array<uint8_t, kStatusPayloadBytes> buffer{};
  size_t offset = 0;

  // Header v1: status_format_ver (u8), reserved_flags (u8), reserved (u16).
  buffer[offset++] = kStatusFormatVer;
  buffer[offset++] = 0;
  write_u16_le(buffer.data() + offset, 0);
  offset += 2;

  // TLV: GNSS (type=1, length=4): gnss_state, pos_valid, pos_age_s.
  buffer[offset++] = kTlvTypeGnss;
  buffer[offset++] = static_cast<uint8_t>(kGnssValueBytes);

  buffer[offset++] = static_cast<uint8_t>(gnss_snapshot.fix_state);
  buffer[offset++] = gnss_snapshot.pos_valid ? 1U : 0U;

  uint16_t pos_age_s = 0;
  if (gnss_snapshot.pos_valid && gnss_snapshot.last_fix_ms > 0 && now_ms >= gnss_snapshot.last_fix_ms) {
    pos_age_s = static_cast<uint16_t>((now_ms - gnss_snapshot.last_fix_ms) / 1000U);
  }
  write_u16_le(buffer.data() + offset, pos_age_s);
  offset += 2;

  transport.set_status(buffer.data(), offset);
  return true;
}

} // namespace protocol
} // namespace naviga
