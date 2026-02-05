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
  void set_node_table_page(uint8_t page_index, const uint8_t* data, size_t len) override;

 private:
  BleTransportCore core_;

  BLEServer* server_ = nullptr;
  BLEService* service_ = nullptr;
  BLEAdvertising* advertising_ = nullptr;
  BLECharacteristic* device_info_char_ = nullptr;
  BLECharacteristic* page_chars_[BleTransportCore::kPageCount] = {nullptr, nullptr, nullptr, nullptr};
};

} // namespace naviga
