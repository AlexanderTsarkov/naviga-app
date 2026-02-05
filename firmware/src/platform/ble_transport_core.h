#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace naviga {

class BleTransportCore {
 public:
  static constexpr size_t kMaxDeviceInfoLen = 256;
  static constexpr size_t kMaxPageLen = 256;
  static constexpr uint8_t kPageCount = 4;

  void set_device_info(const uint8_t* data, size_t len);
  bool set_node_table_page(uint8_t page_index, const uint8_t* data, size_t len);

  const uint8_t* device_info_data() const;
  size_t device_info_len() const;

  const uint8_t* page_data(uint8_t page_index) const;
  size_t page_len(uint8_t page_index) const;

 private:
  std::array<uint8_t, kMaxDeviceInfoLen> device_info_{};
  size_t device_info_len_ = 0;

  std::array<std::array<uint8_t, kMaxPageLen>, kPageCount> page_buf_{};
  std::array<size_t, kPageCount> page_len_ = {0, 0, 0, 0};
};

} // namespace naviga
