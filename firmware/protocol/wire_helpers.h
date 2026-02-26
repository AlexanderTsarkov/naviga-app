#pragma once

// Internal wire encoding/decoding helpers shared by protocol codecs.
// This header is included only from .cpp files; not part of the public API.

#include <cstdint>

namespace naviga {
namespace protocol {
namespace wire {

inline void write_u16_le(uint8_t* dst, uint16_t value) {
  dst[0] = static_cast<uint8_t>(value & 0xFFu);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

inline void write_nodeid48_le(uint8_t* dst, uint64_t value) {
  dst[0] = static_cast<uint8_t>(value & 0xFFu);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
  dst[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
  dst[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
  dst[4] = static_cast<uint8_t>((value >> 32) & 0xFFu);
  dst[5] = static_cast<uint8_t>((value >> 40) & 0xFFu);
}

inline uint16_t read_u16_le(const uint8_t* src) {
  return static_cast<uint16_t>(src[0]) |
         (static_cast<uint16_t>(src[1]) << 8);
}

inline uint64_t read_nodeid48_le(const uint8_t* src) {
  return static_cast<uint64_t>(src[0]) |
         (static_cast<uint64_t>(src[1]) << 8) |
         (static_cast<uint64_t>(src[2]) << 16) |
         (static_cast<uint64_t>(src[3]) << 24) |
         (static_cast<uint64_t>(src[4]) << 32) |
         (static_cast<uint64_t>(src[5]) << 40);
}

} // namespace wire
} // namespace protocol
} // namespace naviga
