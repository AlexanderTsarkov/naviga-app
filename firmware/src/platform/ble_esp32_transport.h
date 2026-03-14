#pragma once

#include <cstddef>
#include <cstdint>

#include "naviga/hal/interfaces.h"
#include "platform/ble_transport_core.h"

class BLECharacteristic;
class BLEServer;
class BLEService;
class BLEAdvertising;

namespace naviga {

/** First-phase BLE contract version (S04 canon). Exact match required for normal connect. */
constexpr uint8_t kBleContractVersionMajor = 1;
constexpr uint8_t kBleContractVersionMinor = 0;

/** Naviga manufacturer ID for BLE advertising (first-phase; reversible). */
constexpr uint16_t kBleNavigaManufacturerId = 0x6E4F;

class BleEsp32Transport : public IBleTransport {
 public:
  BleEsp32Transport();

  void init();

  /** S04 Slice 1: Update advertising with display identity and BLE contract version. */
  void set_advertising_content(const char* display_identity, uint8_t version_major, uint8_t version_minor);

  void set_device_info(const uint8_t* data, size_t len) override;
  void set_node_table_response(const uint8_t* data, size_t len) override;
  void set_status(const uint8_t* data, size_t len) override;
  bool get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const override;
  bool connected() const;
  void set_connected(bool connected);

 private:
  BleTransportCore core_;

  BLEServer* server_ = nullptr;
  BLEService* service_ = nullptr;
  BLEAdvertising* advertising_ = nullptr;
  BLECharacteristic* device_info_char_ = nullptr;
  BLECharacteristic* node_table_char_ = nullptr;
  BLECharacteristic* status_char_ = nullptr;
  BLECharacteristic* node_name_char_ = nullptr;
  BLECharacteristic* node_table_subscribe_char_ = nullptr;
  BLECharacteristic* profiles_list_char_ = nullptr;
  BLECharacteristic* profile_read_char_ = nullptr;
  bool connected_ = false;
};

} // namespace naviga
