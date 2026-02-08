#include "platform/ble_esp32_transport.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace naviga {

namespace {

constexpr char kServiceUuid[] = "6e4f0001-1b9a-4c3a-9a3b-000000000001";
constexpr char kDeviceInfoUuid[] = "6e4f0002-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTableSnapshotUuid[] = "6e4f0003-1b9a-4c3a-9a3b-000000000001";

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

class NodeTableSnapshotCallbacks : public BLECharacteristicCallbacks {
 public:
  explicit NodeTableSnapshotCallbacks(BleTransportCore* core) : core_(core) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !core_) {
      return;
    }
    const uint8_t* data = core_->node_table_response_data();
    const size_t len = core_->node_table_response_len();
    characteristic->setValue(const_cast<uint8_t*>(data), len);
  }

  void onWrite(BLECharacteristic* characteristic) override {
    if (!characteristic || !core_) {
      return;
    }
    const std::string value = characteristic->getValue();
    if (value.size() != 4) {
      return;
    }
    const auto* bytes = reinterpret_cast<const uint8_t*>(value.data());
    const uint16_t snapshot_id =
        static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
    const uint16_t page_index =
        static_cast<uint16_t>(bytes[2] | (static_cast<uint16_t>(bytes[3]) << 8));
    core_->set_node_table_request(snapshot_id, page_index);
  }

 private:
  BleTransportCore* core_ = nullptr;
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

  node_table_char_ = service_->createCharacteristic(
      kNodeTableSnapshotUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  node_table_char_->setCallbacks(new NodeTableSnapshotCallbacks(&core_));

  service_->start();

  advertising_ = BLEDevice::getAdvertising();
  advertising_->addServiceUUID(kServiceUuid);
  advertising_->setScanResponse(true);
  advertising_->start();
}

void BleEsp32Transport::set_device_info(const uint8_t* data, size_t len) {
  core_.set_device_info(data, len);
}

void BleEsp32Transport::set_node_table_response(const uint8_t* data, size_t len) {
  core_.set_node_table_response(data, len);
}

bool BleEsp32Transport::get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const {
  return core_.get_node_table_request(snapshot_id, page_index);
}

bool BleEsp32Transport::connected() const {
  return connected_;
}

void BleEsp32Transport::set_connected(bool connected) {
  connected_ = connected;
}

} // namespace naviga

#else

namespace naviga {

BleEsp32Transport::BleEsp32Transport() = default;

void BleEsp32Transport::init() {}

void BleEsp32Transport::set_device_info(const uint8_t* data, size_t len) {
  core_.set_device_info(data, len);
}

void BleEsp32Transport::set_node_table_response(const uint8_t* data, size_t len) {
  core_.set_node_table_response(data, len);
}

bool BleEsp32Transport::get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const {
  return core_.get_node_table_request(snapshot_id, page_index);
}

bool BleEsp32Transport::connected() const {
  return connected_;
}

void BleEsp32Transport::set_connected(bool connected) {
  connected_ = connected;
}

} // namespace naviga

#endif
