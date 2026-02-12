#include "platform/ble_transport_core.h"

#include <algorithm>
#include <cstring>

namespace naviga {

void BleTransportCore::set_device_info(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, device_info_.size());
  if (data && copy_len > 0) {
    std::memcpy(device_info_.data(), data, copy_len);
  }
  device_info_len_ = copy_len;
}

void BleTransportCore::set_node_table_response(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, node_table_buf_.size());
  if (data && copy_len > 0) {
    std::memcpy(node_table_buf_.data(), data, copy_len);
  }
  node_table_len_ = copy_len;
}

void BleTransportCore::set_status(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, status_buf_.size());
  if (data && copy_len > 0) {
    std::memcpy(status_buf_.data(), data, copy_len);
  }
  status_len_ = copy_len;
}

void BleTransportCore::set_node_table_request(uint16_t snapshot_id, uint16_t page_index) {
  req_snapshot_id_ = snapshot_id;
  req_page_index_ = page_index;
  has_request_ = true;
}

const uint8_t* BleTransportCore::device_info_data() const {
  return device_info_.data();
}

size_t BleTransportCore::device_info_len() const {
  return device_info_len_;
}

bool BleTransportCore::get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const {
  if (!has_request_ || !snapshot_id || !page_index) {
    return false;
  }
  *snapshot_id = req_snapshot_id_;
  *page_index = req_page_index_;
  return true;
}

const uint8_t* BleTransportCore::node_table_response_data() const {
  return node_table_buf_.data();
}

size_t BleTransportCore::node_table_response_len() const {
  return node_table_len_;
}

const uint8_t* BleTransportCore::status_data() const {
  return status_buf_.data();
}

size_t BleTransportCore::status_len() const {
  return status_len_;
}

} // namespace naviga
