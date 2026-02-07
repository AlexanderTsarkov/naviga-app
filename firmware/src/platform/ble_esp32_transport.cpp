#include "platform/ble_esp32_transport.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace naviga {

namespace {

constexpr char kServiceUuid[] = "6e4f0001-1b9a-4c3a-9a3b-000000000001";
constexpr char kDeviceInfoUuid[] = "6e4f0002-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTablePage0Uuid[] = "6e4f0003-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTablePage1Uuid[] = "6e4f0004-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTablePage2Uuid[] = "6e4f0005-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTablePage3Uuid[] = "6e4f0006-1b9a-4c3a-9a3b-000000000001";

class NavigaServerCallbacks : public BLEServerCallbacks {
 public:
  explicit NavigaServerCallbacks(BleEsp32Transport* transport) : transport_(transport) {}

  void onConnect(BLEServer* /*server*/) override {
    if (transport_) {
      transport_->set_connected(true);
    }
  }

  void onDisconnect(BLEServer* server) override {
    if (transport_) {
      transport_->set_connected(false);
    }
    if (server) {
      server->getAdvertising()->start();
    }
  }

 private:
  BleEsp32Transport* transport_ = nullptr;
};

class DeviceInfoCallbacks : public BLECharacteristicCallbacks {
 public:
  explicit DeviceInfoCallbacks(BleTransportCore* core) : core_(core) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !core_) {
      return;
    }
    const uint8_t* data = core_->device_info_data();
    const size_t len = core_->device_info_len();
    characteristic->setValue(const_cast<uint8_t*>(data), len);
  }

 private:
  BleTransportCore* core_ = nullptr;
};

class NodeTablePageCallbacks : public BLECharacteristicCallbacks {
 public:
  NodeTablePageCallbacks(BleTransportCore* core, uint8_t page_index)
      : core_(core), page_index_(page_index) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !core_) {
      return;
    }
    const uint8_t* data = core_->page_data(page_index_);
    const size_t len = core_->page_len(page_index_);
    characteristic->setValue(const_cast<uint8_t*>(data), len);
  }

 private:
  BleTransportCore* core_ = nullptr;
  uint8_t page_index_ = 0;
};

} // namespace

BleEsp32Transport::BleEsp32Transport() = default;

void BleEsp32Transport::init() {
  BLEDevice::init("Naviga");

  server_ = BLEDevice::createServer();
  connected_ = false;
  server_->setCallbacks(new NavigaServerCallbacks(this));

  service_ = server_->createService(kServiceUuid);

  device_info_char_ = service_->createCharacteristic(kDeviceInfoUuid, BLECharacteristic::PROPERTY_READ);
  device_info_char_->setCallbacks(new DeviceInfoCallbacks(&core_));

  page_chars_[0] = service_->createCharacteristic(kNodeTablePage0Uuid, BLECharacteristic::PROPERTY_READ);
  page_chars_[1] = service_->createCharacteristic(kNodeTablePage1Uuid, BLECharacteristic::PROPERTY_READ);
  page_chars_[2] = service_->createCharacteristic(kNodeTablePage2Uuid, BLECharacteristic::PROPERTY_READ);
  page_chars_[3] = service_->createCharacteristic(kNodeTablePage3Uuid, BLECharacteristic::PROPERTY_READ);

  for (uint8_t i = 0; i < BleTransportCore::kPageCount; ++i) {
    page_chars_[i]->setCallbacks(new NodeTablePageCallbacks(&core_, i));
  }

  service_->start();

  advertising_ = BLEDevice::getAdvertising();
  advertising_->addServiceUUID(kServiceUuid);
  advertising_->setScanResponse(true);
  advertising_->start();
}

void BleEsp32Transport::set_device_info(const uint8_t* data, size_t len) {
  core_.set_device_info(data, len);
}

bool BleEsp32Transport::connected() const {
  return connected_;
}

void BleEsp32Transport::set_connected(bool connected) {
  connected_ = connected;
}

void BleEsp32Transport::set_node_table_page(uint8_t page_index,
                                            const uint8_t* data,
                                            size_t len) {
  core_.set_node_table_page(page_index, data, len);
}

} // namespace naviga

#else

namespace naviga {

BleEsp32Transport::BleEsp32Transport() = default;

void BleEsp32Transport::init() {}

void BleEsp32Transport::set_device_info(const uint8_t* data, size_t len) {
  core_.set_device_info(data, len);
}

bool BleEsp32Transport::connected() const {
  return connected_;
}

void BleEsp32Transport::set_connected(bool connected) {
  connected_ = connected;
}

void BleEsp32Transport::set_node_table_page(uint8_t page_index,
                                            const uint8_t* data,
                                            size_t len) {
  core_.set_node_table_page(page_index, data, len);
}

} // namespace naviga

#endif
