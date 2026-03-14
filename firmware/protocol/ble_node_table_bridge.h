#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "domain/node_table.h"
#include "naviga/hal/interfaces.h"

namespace naviga {
namespace protocol {

struct DeviceInfoModel {
  uint8_t format_ver = 1;
  uint8_t ble_schema_ver = 1;
  uint8_t radio_proto_ver = 0;
  bool include_radio_proto_ver = true;

  uint64_t node_id = 0;
  uint16_t short_id = 0;
  uint8_t device_type = 0;

  const char* firmware_version = nullptr;
  const char* radio_module_model = nullptr;

  uint16_t band_id = 0;
  uint16_t power_min = 0;
  uint16_t power_max = 0;
  uint16_t channel_min = 0;
  uint16_t channel_max = 0;

  uint8_t network_mode = 0;
  uint16_t channel_id = 0;
  uint16_t public_channel_id = 1;
  uint16_t private_channel_min = 2;
  uint16_t private_channel_max = 0;

  uint32_t capabilities = 0;
};

class BleNodeTableBridge {
 public:
  /** S04 #464: canon BLE record format (all product-facing fields, exclusions applied). */
  static constexpr uint8_t kRecordFormatVer = 2;
  static constexpr uint8_t kPageSize = 10;
  /** Canon BLE record size (72 bytes). */
  static constexpr size_t kRecordBytesBle = 72;

  bool update_device_info(const DeviceInfoModel& model, IBleTransport& transport) const;
  bool update_node_table(uint32_t now_ms, domain::NodeTable& table, IBleTransport& transport) const;
  /** Fill targeted-read response for one record by node_id. now_ms used for age/stale. No-op if not found (0-byte response). */
  void update_targeted_read(uint32_t now_ms, domain::NodeTable& table, IBleTransport& transport) const;
  bool update_all(uint32_t now_ms,
                  const DeviceInfoModel& model,
                  domain::NodeTable& table,
                  IBleTransport& transport) const;

  /** S04 #465: 2s coalescing; build batch of full changed records (max kMaxBatchRecords); call from runtime only. */
  void update_subscription_batch(uint32_t now_ms,
                                 domain::NodeTable& table,
                                 IBleTransport& transport);

  static constexpr uint32_t kCoalescingWindowMs = 2000U;
  /** Cap per notify to stay under typical BLE MTU (1 + 5*72 = 361 bytes). */
  static constexpr size_t kMaxBatchRecords = 5;

 private:
  uint32_t last_emit_ms_ = 0;
  static constexpr size_t kMaxTrackedNodes = domain::NodeTable::kMaxNodes;
  struct TrackedNode {
    uint64_t node_id = 0;
    uint32_t hash = 0;
  };
  std::array<TrackedNode, kMaxTrackedNodes> prev_{};
  size_t prev_count_ = 0;
  std::array<uint64_t, kMaxBatchRecords> pending_ids_{};
  size_t pending_count_ = 0;
};

} // namespace protocol
} // namespace naviga
