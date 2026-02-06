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

void MockBleTransport::set_node_table_page(uint8_t page_index, const uint8_t* data, size_t len) {
  if (page_index >= 4) {
    return;
  }
  const size_t copy_len = std::min(len, sizeof(page_buf_[page_index]));
  if (data && copy_len > 0) {
    std::memcpy(page_buf_[page_index], data, copy_len);
  }
  page_len_[page_index] = copy_len;
}

size_t MockBleTransport::device_info_len() const {
  return device_info_len_;
}

size_t MockBleTransport::page_len(uint8_t page_index) const {
  return page_index < 4 ? page_len_[page_index] : 0;
}

const uint8_t* MockBleTransport::device_info() const {
  return device_info_;
}

const uint8_t* MockBleTransport::page_data(uint8_t page_index) const {
  return page_index < 4 ? page_buf_[page_index] : nullptr;
}

} // namespace naviga
