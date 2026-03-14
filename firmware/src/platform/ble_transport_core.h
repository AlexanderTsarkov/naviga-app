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

  /** S04 #465: Subscription batch = 1 byte count + N × 72-byte records. Max N = kMaxSubscriptionBatchRecords. */
  static constexpr size_t kMaxSubscriptionBatchRecords = 5;
  static constexpr size_t kMaxSubscriptionBatchLen = 1 + kMaxSubscriptionBatchRecords * kBleCanonRecordBytes;

  void set_subscription_update_payload(const uint8_t* data, size_t len);
  const uint8_t* subscription_update_data() const;
  size_t subscription_update_len() const;

  /** S04 #466: Self node_name short display label: 1-byte length + UTF-8 payload (max 24 bytes). */
  static constexpr size_t kMaxNodeNameLen = 24;
  static constexpr size_t kMaxNodeNamePayloadLen = 1 + kMaxNodeNameLen;  // length byte + payload

  void set_node_name_value(const uint8_t* data, size_t len);
  const uint8_t* node_name_value_data() const;
  size_t node_name_value_len() const;

  void set_node_name_write_request(const uint8_t* data, size_t len);
  bool has_node_name_write_request() const { return has_node_name_write_; }
  const uint8_t* node_name_write_request_data() const;
  size_t node_name_write_request_len() const;
  void clear_node_name_write_request();

  /** S04 #467: Profiles list (single read). Payload: n_radio(1), radio_ids(4×n), n_user(1), user_ids(4×n). LE. */
  static constexpr size_t kMaxProfilesListLen = 64;
  void set_profiles_list(const uint8_t* data, size_t len);
  const uint8_t* profiles_list_data() const;
  size_t profiles_list_len() const;

  /** S04 #467: Profile read request (1 byte type: 0=radio, 1=user; 4 bytes id LE). Set from GATT onWrite. */
  static constexpr size_t kProfileReadRequestLen = 5;
  void set_profile_read_request(uint8_t type, uint32_t id);
  bool get_profile_read_request(uint8_t* type, uint32_t* id) const;
  bool has_profile_read_request() const { return has_profile_read_request_; }
  void clear_profile_read_request();

  /** S04 #467: Profile read response (one serialized profile). Filled by bridge in update_ble. */
  static constexpr size_t kMaxProfileReadResponseLen = 64;
  void set_profile_read_response(const uint8_t* data, size_t len);
  const uint8_t* profile_read_response_data() const;
  size_t profile_read_response_len() const;

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

  std::array<uint8_t, kMaxSubscriptionBatchLen> subscription_update_buf_{};
  size_t subscription_update_len_ = 0;

  std::array<uint8_t, kMaxNodeNamePayloadLen> node_name_value_buf_{};
  size_t node_name_value_len_ = 0;

  std::array<uint8_t, kMaxNodeNamePayloadLen> node_name_write_buf_{};
  size_t node_name_write_len_ = 0;
  bool has_node_name_write_ = false;

  std::array<uint8_t, kMaxProfilesListLen> profiles_list_buf_{};
  size_t profiles_list_len_ = 0;

  uint8_t profile_read_request_type_ = 0;
  uint32_t profile_read_request_id_ = 0;
  bool has_profile_read_request_ = false;

  std::array<uint8_t, kMaxProfileReadResponseLen> profile_read_response_buf_{};
  size_t profile_read_response_len_ = 0;
};

} // namespace naviga
