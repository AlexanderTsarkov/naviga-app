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

void MockBleTransport::set_targeted_read_request(uint64_t node_id) {
  req_targeted_node_id_ = node_id;
  has_targeted_request_ = true;
}

bool MockBleTransport::get_targeted_read_request(uint64_t* node_id) const {
  if (!has_targeted_request_ || !node_id) {
    return false;
  }
  *node_id = req_targeted_node_id_;
  return true;
}

const uint8_t* MockBleTransport::targeted_read_response_data() const {
  return targeted_read_buf_;
}

size_t MockBleTransport::targeted_read_response_len() const {
  return targeted_read_len_;
}

void MockBleTransport::set_subscription_update_payload(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, sizeof(subscription_update_buf_));
  if (data && copy_len > 0) {
    std::memcpy(subscription_update_buf_, data, copy_len);
  }
  subscription_update_len_ = copy_len;
}

void MockBleTransport::send_subscription_update() {}

void MockBleTransport::set_profiles_list(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, sizeof(profiles_list_buf_));
  if (data && copy_len > 0) {
    std::memcpy(profiles_list_buf_, data, copy_len);
  }
  profiles_list_len_ = copy_len;
}

const uint8_t* MockBleTransport::profiles_list_data() const {
  return profiles_list_buf_;
}

size_t MockBleTransport::profiles_list_len() const {
  return profiles_list_len_;
}

bool MockBleTransport::get_profile_read_request(uint8_t* type, uint32_t* id) const {
  if (!has_profile_read_request_ || !type || !id) return false;
  *type = profile_read_request_type_;
  *id = profile_read_request_id_;
  return true;
}

bool MockBleTransport::has_profile_read_request() const {
  return has_profile_read_request_;
}

void MockBleTransport::set_profile_read_response(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, sizeof(profile_read_response_buf_));
  if (data && copy_len > 0) {
    std::memcpy(profile_read_response_buf_, data, copy_len);
  }
  profile_read_response_len_ = copy_len;
}

const uint8_t* MockBleTransport::profile_read_response_data() const {
  return profile_read_response_buf_;
}

size_t MockBleTransport::profile_read_response_len() const {
  return profile_read_response_len_;
}

void MockBleTransport::clear_profile_read_request() {
  has_profile_read_request_ = false;
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
