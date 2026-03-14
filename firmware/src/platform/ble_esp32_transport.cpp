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

/** S04 #467: Profiles list — single read from core buffer. */
class ProfilesListCallbacks : public BLECharacteristicCallbacks {
 public:
  explicit ProfilesListCallbacks(BleEsp32Transport* transport) : transport_(transport) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) return;
    BleTransportCore* core = transport_->core_for_callbacks();
    const uint8_t* data = core->profiles_list_data();
    const size_t len = core->profiles_list_len();
    if (len > 0) {
      characteristic->setValue(const_cast<uint8_t*>(data), len);
    } else {
      characteristic->setValue("");
    }
  }

 private:
  BleEsp32Transport* transport_ = nullptr;
};

/** S04 #467: Profile read — write request (1B type + 4B id LE), read response (one profile). */
class ProfileReadCallbacks : public BLECharacteristicCallbacks {
 public:
  explicit ProfileReadCallbacks(BleEsp32Transport* transport) : transport_(transport) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) return;
    BleTransportCore* core = transport_->core_for_callbacks();
    const uint8_t* data = core->profile_read_response_data();
    const size_t len = core->profile_read_response_len();
    if (len > 0) {
      characteristic->setValue(const_cast<uint8_t*>(data), len);
    } else {
      characteristic->setValue("");
    }
  }

  void onWrite(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) return;
    const std::string value = characteristic->getValue();
    if (value.size() != 5) return;
    const auto* bytes = reinterpret_cast<const uint8_t*>(value.data());
    const uint8_t type = bytes[0];
    uint32_t id = 0;
    for (int i = 0; i < 4; ++i) {
      id |= static_cast<uint32_t>(bytes[1 + i]) << (8 * i);
    }
    transport_->core_for_callbacks()->set_profile_read_request(type, id);
  }

 private:
  BleEsp32Transport* transport_ = nullptr;
};

/** S04 #466: Self node_name. First-phase encoding: 1-byte length (0–32) + UTF-8 payload. Callbacks only read/write core; runtime applies write. */
class NodeNameCallbacks : public BLECharacteristicCallbacks {
 public:
  explicit NodeNameCallbacks(BleEsp32Transport* transport) : transport_(transport) {}

  void onRead(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) {
      return;
    }
    BleTransportCore* core = transport_->core_for_callbacks();
    const uint8_t* data = core->node_name_value_data();
    const size_t len = core->node_name_value_len();
    if (len > 0) {
      characteristic->setValue(const_cast<uint8_t*>(data), len);
    } else {
      characteristic->setValue("");
    }
  }

  void onWrite(BLECharacteristic* characteristic) override {
    if (!characteristic || !transport_) {
      return;
    }
    const std::string value = characteristic->getValue();
    if (value.empty()) {
      return;
    }
    const auto* bytes = reinterpret_cast<const uint8_t*>(value.data());
    const size_t received = value.size();
    uint8_t len_byte = bytes[0];
    if (len_byte > BleTransportCore::kMaxNodeNameLen) {
      len_byte = static_cast<uint8_t>(BleTransportCore::kMaxNodeNameLen);
    }
    const size_t payload_len = (received >= 1u + len_byte) ? len_byte : (received > 1u ? received - 1u : 0u);
    const size_t total = 1u + payload_len;
    if (total > 0) {
      transport_->core_for_callbacks()->set_node_name_write_request(bytes, total);
    }
  }

 private:
  BleEsp32Transport* transport_ = nullptr;
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
  node_name_char_->setCallbacks(new NodeNameCallbacks(this));

  node_table_subscribe_char_ = service_->createCharacteristic(
      kNodeTableSubscribeUuid,
      BLECharacteristic::PROPERTY_NOTIFY);
  node_table_subscribe_char_->setCallbacks(new StubReadCallbacks());

  profiles_list_char_ = service_->createCharacteristic(kProfilesListUuid, BLECharacteristic::PROPERTY_READ);
  profiles_list_char_->setCallbacks(new ProfilesListCallbacks(this));

  profile_read_char_ = service_->createCharacteristic(
      kProfileReadUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  profile_read_char_->setCallbacks(new ProfileReadCallbacks(this));

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

void BleEsp32Transport::set_subscription_update_payload(const uint8_t* data, size_t len) {
  core_.set_subscription_update_payload(data, len);
}

void BleEsp32Transport::send_subscription_update() {
  if (!connected_ || !node_table_subscribe_char_ || core_.subscription_update_len() == 0) {
    return;
  }
  node_table_subscribe_char_->setValue(
      const_cast<uint8_t*>(core_.subscription_update_data()),
      core_.subscription_update_len());
  node_table_subscribe_char_->notify();
}

void BleEsp32Transport::set_node_name_value(const uint8_t* data, size_t len) {
  core_.set_node_name_value(data, len);
}

bool BleEsp32Transport::has_node_name_write_request() const {
  return core_.has_node_name_write_request();
}

const uint8_t* BleEsp32Transport::node_name_write_request_data() const {
  return core_.node_name_write_request_data();
}

size_t BleEsp32Transport::node_name_write_request_len() const {
  return core_.node_name_write_request_len();
}

void BleEsp32Transport::clear_node_name_write_request() {
  core_.clear_node_name_write_request();
}

void BleEsp32Transport::set_profiles_list(const uint8_t* data, size_t len) {
  core_.set_profiles_list(data, len);
}

const uint8_t* BleEsp32Transport::profiles_list_data() const {
  return core_.profiles_list_data();
}

size_t BleEsp32Transport::profiles_list_len() const {
  return core_.profiles_list_len();
}

bool BleEsp32Transport::get_profile_read_request(uint8_t* type, uint32_t* id) const {
  return core_.get_profile_read_request(type, id);
}

bool BleEsp32Transport::has_profile_read_request() const {
  return core_.has_profile_read_request();
}

void BleEsp32Transport::set_profile_read_response(const uint8_t* data, size_t len) {
  core_.set_profile_read_response(data, len);
}

const uint8_t* BleEsp32Transport::profile_read_response_data() const {
  return core_.profile_read_response_data();
}

size_t BleEsp32Transport::profile_read_response_len() const {
  return core_.profile_read_response_len();
}

void BleEsp32Transport::clear_profile_read_request() {
  core_.clear_profile_read_request();
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

void BleEsp32Transport::set_subscription_update_payload(const uint8_t* data, size_t len) {
  core_.set_subscription_update_payload(data, len);
}

void BleEsp32Transport::send_subscription_update() {}

void BleEsp32Transport::set_node_name_value(const uint8_t* data, size_t len) {
  core_.set_node_name_value(data, len);
}

bool BleEsp32Transport::has_node_name_write_request() const {
  return core_.has_node_name_write_request();
}

const uint8_t* BleEsp32Transport::node_name_write_request_data() const {
  return core_.node_name_write_request_data();
}

size_t BleEsp32Transport::node_name_write_request_len() const {
  return core_.node_name_write_request_len();
}

void BleEsp32Transport::clear_node_name_write_request() {
  core_.clear_node_name_write_request();
}

void BleEsp32Transport::set_profiles_list(const uint8_t* data, size_t len) {
  core_.set_profiles_list(data, len);
}

const uint8_t* BleEsp32Transport::profiles_list_data() const {
  return core_.profiles_list_data();
}

size_t BleEsp32Transport::profiles_list_len() const {
  return core_.profiles_list_len();
}

bool BleEsp32Transport::get_profile_read_request(uint8_t* type, uint32_t* id) const {
  return core_.get_profile_read_request(type, id);
}

bool BleEsp32Transport::has_profile_read_request() const {
  return core_.has_profile_read_request();
}

void BleEsp32Transport::set_profile_read_response(const uint8_t* data, size_t len) {
  core_.set_profile_read_response(data, len);
}

const uint8_t* BleEsp32Transport::profile_read_response_data() const {
  return core_.profile_read_response_data();
}

size_t BleEsp32Transport::profile_read_response_len() const {
  return core_.profile_read_response_len();
}

void BleEsp32Transport::clear_profile_read_request() {
  core_.clear_profile_read_request();
}

bool BleEsp32Transport::connected() const {
  return connected_;
}

void BleEsp32Transport::set_connected(bool connected) {
  connected_ = connected;
}

} // namespace naviga

#endif
