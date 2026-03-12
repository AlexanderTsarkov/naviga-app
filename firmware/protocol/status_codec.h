#pragma once

#include <cstddef>
#include <cstdint>

#include "packet_header.h"
#include "wire_helpers.h"

namespace naviga {
namespace protocol {

/**
 * Node_Status v0.2 (#435) — full snapshot of operational + informative status.
 *
 * Payload: 9 B common (payloadVersion, nodeId48, seq16) + useful 10 B:
 *   batteryPercent (1), battery_Est_Rem_Time (1), TX_power_Ch_throttle (1),
 *   uptime10m (1), role_id (1), maxSilence10s (1), hwProfileId (2 LE), fwVersionId (2 LE).
 * Total payload 19 B; on-air 21 B (msg_type=0x07).
 * hw_profile_id / fw_version_id remain uint16 per canon.
 */
struct StatusFields {
  uint64_t node_id = 0;
  uint16_t seq16   = 0;
  uint8_t  battery_percent = 0xFF;
  uint8_t  battery_est_rem_time = 0xFF;  ///< 10-min units; 0xFF = N/A.
  uint8_t  tx_power_ch_throttle = 0;     ///< 4+4 bits or single byte.
  uint8_t  uptime10m = 0;
  uint8_t  role_id = 0;
  uint8_t  max_silence_10s = 0;
  uint16_t hw_profile_id = 0xFFFF;
  uint16_t fw_version_id = 0xFFFF;
};

constexpr uint8_t kStatusPayloadVersion = 0x00;
constexpr size_t kStatusPayloadSize = 19;
constexpr size_t kStatusFrameSize = kHeaderSize + kStatusPayloadSize;

enum class StatusDecodeError {
  Ok = 0,
  ShortBuffer,
  BadPayloadVersion,
};

size_t encode_status_frame(const StatusFields& fields, uint8_t* out, size_t out_cap);
StatusDecodeError decode_status_payload(const uint8_t* payload, size_t payload_len,
                                        StatusFields* out);

} // namespace protocol
} // namespace naviga
