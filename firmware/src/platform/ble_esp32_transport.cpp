#include "platform/ble_esp32_transport.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

#include <cstring>
#include <string>

namespace naviga {

namespace {

constexpr char kServiceUuid[] = "6e4f0001-1b9a-4c3a-9a3b-000000000001";
constexpr char kDeviceInfoUuid[] = "6e4f0002-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTableSnapshotUuid[] = "6e4f0003-1b9a-4c3a-9a3b-000000000001";
constexpr char kStatusUuid[] = "6e4f0007-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeNameUuid[] = "6e4f0008-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTableSubscribeUuid[] = "6e4f0009-1b9a-4c3a-9a3b-000000000001";
constexpr char kProfilesListUuid[] = "6e4f000a-1b9a-4c3a-9a3b-000000000001";
constexpr char kProfileReadUuid[] = "6e4f000b-1b9a-4c3a-9a3b-000000000001";
/** S04 #464: Targeted read — write node_id (8 bytes LE), read one canon record. */
constexpr char kTargetedReadUuid[] = "6e4f000c-1b9a-4c3a-9a3b-000000000001";
constexpr size_t kMaxDisplayIdentityLen = 32;

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
  explicit NodeTableSnapshotCallbacks(BleEsp32Transport* transport) : transport_(transport) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) {
      return;
    }
    const uint8_t* data = transport_->core_for_callbacks()->node_table_response_data();
    const size_t len = transport_->core_for_callbacks()->node_table_response_len();
    characteristic->setValue(const_cast<uint8_t*>(data), len);
  }

  void onWrite(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) {
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
    transport_->core_for_callbacks()->set_node_table_request(snapshot_id, page_index);
    // Deferred: runtime loop processes pending request (no NodeTable access from callback).
  }

 private:
  BleEsp32Transport* transport_ = nullptr;
};

class TargetedReadCallbacks : public BLECharacteristicCallbacks {
 public:
  explicit TargetedReadCallbacks(BleEsp32Transport* transport) : transport_(transport) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) {
      return;
    }
    const uint8_t* data = transport_->core_for_callbacks()->targeted_read_response_data();
    const size_t len = transport_->core_for_callbacks()->targeted_read_response_len();
    characteristic->setValue(const_cast<uint8_t*>(data), len);
  }

  void onWrite(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) {
      return;
    }
    const std::string value = characteristic->getValue();
    if (value.size() != 8) {
      return;
    }
    const auto* bytes = reinterpret_cast<const uint8_t*>(value.data());
    uint64_t node_id = 0;
    for (int i = 0; i < 8; ++i) {
      node_id |= static_cast<uint64_t>(bytes[i]) << (8 * i);
    }
    transport_->core_for_callbacks()->set_targeted_read_request(node_id);
    // Deferred: runtime loop processes pending request (no NodeTable access from callback).
  }

 private:
  BleEsp32Transport* transport_ = nullptr;
};

class StatusCallbacks : public BLECharacteristicCallbacks {
 public:
  explicit StatusCallbacks(BleTransportCore* core) : core_(core) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !core_) {
      return;
    }
    const uint8_t* data = core_->status_data();
    const size_t len = core_->status_len();
    characteristic->setValue(const_cast<uint8_t*>(data), len);
  }

 private:
  BleTransportCore* core_ = nullptr;
};

class StubReadCallbacks : public BLECharacteristicCallbacks {
 public:
  void onRead(BLECharacteristic* characteristic) override {
    if (characteristic) {
      characteristic->setValue("");
    }
  }
};

class StubReadWriteCallbacks : public BLECharacteristicCallbacks {
 public:
  void onRead(BLECharacteristic* characteristic) override {
    if (characteristic) {
      characteristic->setValue("");
    }
  }
  void onWrite(BLECharacteristic* /*characteristic*/) override {}
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
  node_table_char_->setCallbacks(new NodeTableSnapshotCallbacks(this));

  targeted_read_char_ = service_->createCharacteristic(
      kTargetedReadUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  targeted_read_char_->setCallbacks(new TargetedReadCallbacks(this));

  status_char_ = service_->createCharacteristic(kStatusUuid, BLECharacteristic::PROPERTY_READ);
  status_char_->setCallbacks(new StatusCallbacks(&core_));

  node_name_char_ = service_->createCharacteristic(
      kNodeNameUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  node_name_char_->setCallbacks(new StubReadWriteCallbacks());

  node_table_subscribe_char_ = service_->createCharacteristic(
      kNodeTableSubscribeUuid,
      BLECharacteristic::PROPERTY_NOTIFY);
  node_table_subscribe_char_->setCallbacks(new StubReadCallbacks());

  profiles_list_char_ = service_->createCharacteristic(kProfilesListUuid, BLECharacteristic::PROPERTY_READ);
  profiles_list_char_->setCallbacks(new StubReadCallbacks());

  profile_read_char_ = service_->createCharacteristic(kProfileReadUuid, BLECharacteristic::PROPERTY_READ);
  profile_read_char_->setCallbacks(new StubReadCallbacks());

  service_->start();

  advertising_ = BLEDevice::getAdvertising();
  advertising_->addServiceUUID(kServiceUuid);
  advertising_->setScanResponse(true);
  BLEAdvertisementData scan_resp;
  scan_resp.setName("Naviga");
  const uint8_t manu[] = {
      static_cast<uint8_t>(kBleNavigaManufacturerId & 0xFF),
      static_cast<uint8_t>(kBleNavigaManufacturerId >> 8),
      kBleContractVersionMajor,
      kBleContractVersionMinor
  };
  scan_resp.setManufacturerData(std::string(reinterpret_cast<const char*>(manu), sizeof(manu)));
  advertising_->setScanResponseData(scan_resp);
  advertising_->start();
}

void BleEsp32Transport::set_advertising_content(const char* display_identity,
                                                uint8_t version_major,
                                                uint8_t version_minor) {
  if (!advertising_ || !display_identity) {
    return;
  }
  char name_buf[kMaxDisplayIdentityLen];
  const size_t len = std::strlen(display_identity);
  const size_t copy_len = std::min(len, kMaxDisplayIdentityLen - 1);
  std::memcpy(name_buf, display_identity, copy_len);
  name_buf[copy_len] = '\0';

  advertising_->stop();
  BLEAdvertisementData scan_resp;
  scan_resp.setName(std::string(name_buf));
  const uint8_t manu[] = {
      static_cast<uint8_t>(kBleNavigaManufacturerId & 0xFF),
      static_cast<uint8_t>(kBleNavigaManufacturerId >> 8),
      version_major,
      version_minor
  };
  scan_resp.setManufacturerData(std::string(reinterpret_cast<const char*>(manu), sizeof(manu)));
  advertising_->setScanResponseData(scan_resp);
  advertising_->start();
}

void BleEsp32Transport::set_device_info(const uint8_t* data, size_t len) {
  core_.set_device_info(data, len);
}

void BleEsp32Transport::set_node_table_response(const uint8_t* data, size_t len) {
  core_.set_node_table_response(data, len);
}

void BleEsp32Transport::set_status(const uint8_t* data, size_t len) {
  core_.set_status(data, len);
}

bool BleEsp32Transport::get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const {
  return core_.get_node_table_request(snapshot_id, page_index);
}

void BleEsp32Transport::set_targeted_read_response(const uint8_t* data, size_t len) {
  core_.set_targeted_read_response(data, len);
}

void BleEsp32Transport::set_targeted_read_request(uint64_t node_id) {
  core_.set_targeted_read_request(node_id);
}

bool BleEsp32Transport::get_targeted_read_request(uint64_t* node_id) const {
  return core_.get_targeted_read_request(node_id);
}

const uint8_t* BleEsp32Transport::targeted_read_response_data() const {
  return core_.targeted_read_response_data();
}

size_t BleEsp32Transport::targeted_read_response_len() const {
  return core_.targeted_read_response_len();
}

void BleEsp32Transport::handle_node_table_write(uint16_t snapshot_id, uint16_t page_index) {
  if (request_handler_) {
    request_handler_->on_node_table_request(snapshot_id, page_index);
  }
}

void BleEsp32Transport::handle_targeted_read_write(uint64_t node_id) {
  if (request_handler_) {
    request_handler_->on_targeted_read_request(node_id);
  }
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

void BleEsp32Transport::set_advertising_content(const char* /*display_identity*/,
                                                 uint8_t /*version_major*/,
                                                 uint8_t /*version_minor*/) {}

void BleEsp32Transport::set_device_info(const uint8_t* data, size_t len) {
  core_.set_device_info(data, len);
}

void BleEsp32Transport::set_node_table_response(const uint8_t* data, size_t len) {
  core_.set_node_table_response(data, len);
}

void BleEsp32Transport::set_status(const uint8_t* data, size_t len) {
  core_.set_status(data, len);
}

bool BleEsp32Transport::get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const {
  return core_.get_node_table_request(snapshot_id, page_index);
}

void BleEsp32Transport::set_targeted_read_response(const uint8_t* data, size_t len) {
  core_.set_targeted_read_response(data, len);
}

void BleEsp32Transport::set_targeted_read_request(uint64_t node_id) {
  core_.set_targeted_read_request(node_id);
}

bool BleEsp32Transport::get_targeted_read_request(uint64_t* node_id) const {
  return core_.get_targeted_read_request(node_id);
}

const uint8_t* BleEsp32Transport::targeted_read_response_data() const {
  return core_.targeted_read_response_data();
}

size_t BleEsp32Transport::targeted_read_response_len() const {
  return core_.targeted_read_response_len();
}

void BleEsp32Transport::handle_node_table_write(uint16_t /*snapshot_id*/, uint16_t /*page_index*/) {}

void BleEsp32Transport::handle_targeted_read_write(uint64_t /*node_id*/) {}

bool BleEsp32Transport::connected() const {
  return connected_;
}

void BleEsp32Transport::set_connected(bool connected) {
  connected_ = connected;
}

} // namespace naviga

#endif
