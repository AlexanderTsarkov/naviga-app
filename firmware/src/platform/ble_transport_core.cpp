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

void BleTransportCore::set_targeted_read_response(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, targeted_read_buf_.size());
  if (data && copy_len > 0) {
    std::memcpy(targeted_read_buf_.data(), data, copy_len);
  }
  targeted_read_len_ = copy_len;
}

void BleTransportCore::set_targeted_read_request(uint64_t node_id) {
  req_targeted_node_id_ = node_id;
  has_targeted_request_ = true;
}

bool BleTransportCore::get_targeted_read_request(uint64_t* node_id) const {
  if (!has_targeted_request_ || !node_id) {
    return false;
  }
  *node_id = req_targeted_node_id_;
  return true;
}

const uint8_t* BleTransportCore::targeted_read_response_data() const {
  return targeted_read_buf_.data();
}

size_t BleTransportCore::targeted_read_response_len() const {
  return targeted_read_len_;
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

void BleTransportCore::set_subscription_update_payload(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, subscription_update_buf_.size());
  if (data && copy_len > 0) {
    std::memcpy(subscription_update_buf_.data(), data, copy_len);
  }
  subscription_update_len_ = copy_len;
}

const uint8_t* BleTransportCore::subscription_update_data() const {
  return subscription_update_buf_.data();
}

size_t BleTransportCore::subscription_update_len() const {
  return subscription_update_len_;
}

} // namespace naviga
