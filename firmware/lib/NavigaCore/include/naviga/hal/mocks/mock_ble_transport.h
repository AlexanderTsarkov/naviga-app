#pragma once

#include <cstddef>
#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

class MockBleTransport : public IBleTransport {
 public:
  void set_device_info(const uint8_t* data, size_t len) override;
  void set_node_table_response(const uint8_t* data, size_t len) override;
  bool get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const override;

  size_t device_info_len() const;
  const uint8_t* device_info() const;
  size_t node_table_len() const;
  const uint8_t* node_table_data() const;
  void set_node_table_request(uint16_t snapshot_id, uint16_t page_index);

 private:
  uint8_t device_info_[256] = {0};
  size_t device_info_len_ = 0;

  uint8_t node_table_buf_[320] = {0};
  size_t node_table_len_ = 0;
  uint16_t req_snapshot_id_ = 0;
  uint16_t req_page_index_ = 0;
  bool has_request_ = false;
};

} // namespace naviga
