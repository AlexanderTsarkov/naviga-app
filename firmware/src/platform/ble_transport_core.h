#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace naviga {

class BleTransportCore {
 public:
  static constexpr size_t kMaxDeviceInfoLen = 256;
  static constexpr size_t kMaxPageLen = 320;
  static constexpr size_t kMaxStatusLen = 64;

  void set_device_info(const uint8_t* data, size_t len);
  void set_node_table_response(const uint8_t* data, size_t len);
  void set_status(const uint8_t* data, size_t len);
  void set_node_table_request(uint16_t snapshot_id, uint16_t page_index);
  bool get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const;

  const uint8_t* device_info_data() const;
  size_t device_info_len() const;

  const uint8_t* node_table_response_data() const;
  size_t node_table_response_len() const;
  const uint8_t* status_data() const;
  size_t status_len() const;

 private:
  std::array<uint8_t, kMaxDeviceInfoLen> device_info_{};
  size_t device_info_len_ = 0;

  std::array<uint8_t, kMaxPageLen> node_table_buf_{};
  size_t node_table_len_ = 0;
  std::array<uint8_t, kMaxStatusLen> status_buf_{};
  size_t status_len_ = 0;
  uint16_t req_snapshot_id_ = 0;
  uint16_t req_page_index_ = 0;
  bool has_request_ = false;
};

} // namespace naviga
