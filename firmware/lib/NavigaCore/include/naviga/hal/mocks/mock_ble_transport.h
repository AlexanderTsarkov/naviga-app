#pragma once

#include <cstddef>
#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

class MockBleTransport : public IBleTransport {
 public:
  void set_device_info(const uint8_t* data, size_t len) override;
  void set_node_table_response(const uint8_t* data, size_t len) override;
  void set_status(const uint8_t* data, size_t len) override;
  bool get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const override;
  void set_targeted_read_response(const uint8_t* data, size_t len) override;
  void set_targeted_read_request(uint64_t node_id) override;
  bool get_targeted_read_request(uint64_t* node_id) const override;
  const uint8_t* targeted_read_response_data() const override;
  size_t targeted_read_response_len() const override;
  void set_subscription_update_payload(const uint8_t* data, size_t len) override;
  void send_subscription_update() override;

  const uint8_t* subscription_update_data() const { return subscription_update_buf_; }
  size_t subscription_update_len() const { return subscription_update_len_; }

  size_t device_info_len() const;
  const uint8_t* device_info() const;
  size_t node_table_len() const;
  const uint8_t* node_table_data() const;
  void set_node_table_request(uint16_t snapshot_id, uint16_t page_index);

 private:
  uint8_t device_info_[256] = {0};
  size_t device_info_len_ = 0;

  uint8_t node_table_buf_[768] = {0};
  size_t node_table_len_ = 0;
  uint8_t status_buf_[64] = {0};
  size_t status_len_ = 0;
  uint16_t req_snapshot_id_ = 0;
  uint16_t req_page_index_ = 0;
  bool has_request_ = false;

  uint8_t targeted_read_buf_[72] = {0};
  size_t targeted_read_len_ = 0;
  uint64_t req_targeted_node_id_ = 0;
  bool has_targeted_request_ = false;

  static constexpr size_t kMaxSubscriptionBatchLen = 1 + 5 * 72;  // 1 + 5*72
  uint8_t subscription_update_buf_[kMaxSubscriptionBatchLen] = {0};
  size_t subscription_update_len_ = 0;
};

} // namespace naviga
