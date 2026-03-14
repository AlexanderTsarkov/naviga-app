#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace naviga {

/** S04 #464: BLE canon record = 72 bytes. One targeted-read record fits. */
static constexpr size_t kBleCanonRecordBytes = 72;

class BleTransportCore {
 public:
  static constexpr size_t kMaxDeviceInfoLen = 256;
  /** S04 #464: fit 10 canon BLE records (72 B) + 10 B header = 730. */
  static constexpr size_t kMaxPageLen = 768;
  static constexpr size_t kMaxStatusLen = 64;

  void set_device_info(const uint8_t* data, size_t len);
  void set_node_table_response(const uint8_t* data, size_t len);
  void set_status(const uint8_t* data, size_t len);
  void set_node_table_request(uint16_t snapshot_id, uint16_t page_index);
  bool get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const;

  void set_targeted_read_response(const uint8_t* data, size_t len);
  void set_targeted_read_request(uint64_t node_id);
  bool get_targeted_read_request(uint64_t* node_id) const;
  const uint8_t* targeted_read_response_data() const;
  size_t targeted_read_response_len() const;

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

  std::array<uint8_t, kBleCanonRecordBytes> targeted_read_buf_{};
  size_t targeted_read_len_ = 0;
  uint64_t req_targeted_node_id_ = 0;
  bool has_targeted_request_ = false;
};

} // namespace naviga
