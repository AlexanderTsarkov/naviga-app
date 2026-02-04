#pragma once

#include <cstddef>
#include <cstdint>

#include "hal/interfaces.h"

namespace naviga {

class MockBleTransport : public IBleTransport {
 public:
  void set_device_info(const uint8_t* data, size_t len) override;
  void set_node_table_page(uint8_t page_index, const uint8_t* data, size_t len) override;

  size_t device_info_len() const;
  size_t page_len(uint8_t page_index) const;
  const uint8_t* device_info() const;

 private:
  uint8_t device_info_[256] = {0};
  size_t device_info_len_ = 0;

  uint8_t page_buf_[4][256] = {{0}};
  size_t page_len_[4] = {0, 0, 0, 0};
};

} // namespace naviga
