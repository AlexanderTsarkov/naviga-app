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

class BleEsp32Transport : public IBleTransport {
 public:
  BleEsp32Transport();

  void init();

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
  bool connected_ = false;
};

} // namespace naviga
