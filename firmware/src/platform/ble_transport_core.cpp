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

bool BleTransportCore::set_node_table_page(uint8_t page_index,
                                           const uint8_t* data,
                                           size_t len) {
  if (page_index >= kPageCount) {
    return false;
  }
  const size_t copy_len = std::min(len, page_buf_[page_index].size());
  if (data && copy_len > 0) {
    std::memcpy(page_buf_[page_index].data(), data, copy_len);
  }
  page_len_[page_index] = copy_len;
  return true;
}

const uint8_t* BleTransportCore::device_info_data() const {
  return device_info_.data();
}

size_t BleTransportCore::device_info_len() const {
  return device_info_len_;
}

const uint8_t* BleTransportCore::page_data(uint8_t page_index) const {
  return page_index < kPageCount ? page_buf_[page_index].data() : nullptr;
}

size_t BleTransportCore::page_len(uint8_t page_index) const {
  return page_index < kPageCount ? page_len_[page_index] : 0;
}

} // namespace naviga
