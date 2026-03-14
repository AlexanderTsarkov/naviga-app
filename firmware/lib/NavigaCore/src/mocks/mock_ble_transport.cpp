#include "naviga/hal/mocks/mock_ble_transport.h"

#include <algorithm>
#include <cstring>

namespace naviga {

void MockBleTransport::set_device_info(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, sizeof(device_info_));
  if (data && copy_len > 0) {
    std::memcpy(device_info_, data, copy_len);
  }
  device_info_len_ = copy_len;
}

void MockBleTransport::set_node_table_response(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, sizeof(node_table_buf_));
  if (data && copy_len > 0) {
    std::memcpy(node_table_buf_, data, copy_len);
  }
  node_table_len_ = copy_len;
}

void MockBleTransport::set_status(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, sizeof(status_buf_));
  if (data && copy_len > 0) {
    std::memcpy(status_buf_, data, copy_len);
  }
  status_len_ = copy_len;
}

bool MockBleTransport::get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const {
  if (!has_request_ || !snapshot_id || !page_index) {
    return false;
  }
  *snapshot_id = req_snapshot_id_;
  *page_index = req_page_index_;
  return true;
}

void MockBleTransport::set_targeted_read_response(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, sizeof(targeted_read_buf_));
  if (data && copy_len > 0) {
    std::memcpy(targeted_read_buf_, data, copy_len);
  }
  targeted_read_len_ = copy_len;
}

void MockBleTransport::set_targeted_read_request(uint16_t short_id) {
  req_targeted_short_id_ = short_id;
  has_targeted_request_ = true;
}

bool MockBleTransport::get_targeted_read_request(uint16_t* short_id) const {
  if (!has_targeted_request_ || !short_id) {
    return false;
  }
  *short_id = req_targeted_short_id_;
  return true;
}

const uint8_t* MockBleTransport::targeted_read_response_data() const {
  return targeted_read_buf_;
}

size_t MockBleTransport::targeted_read_response_len() const {
  return targeted_read_len_;
}

size_t MockBleTransport::device_info_len() const {
  return device_info_len_;
}

const uint8_t* MockBleTransport::device_info() const {
  return device_info_;
}

size_t MockBleTransport::node_table_len() const {
  return node_table_len_;
}

const uint8_t* MockBleTransport::node_table_data() const {
  return node_table_buf_;
}

void MockBleTransport::set_node_table_request(uint16_t snapshot_id, uint16_t page_index) {
  req_snapshot_id_ = snapshot_id;
  req_page_index_ = page_index;
  has_request_ = true;
}

} // namespace naviga
