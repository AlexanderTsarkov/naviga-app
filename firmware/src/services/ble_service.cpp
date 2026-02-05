#include "services/ble_service.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <algorithm>
#include <array>
#include <cstring>

#include "app/node_table.h"
#include "platform/timebase.h"

namespace naviga {

namespace {

constexpr char kServiceUuid[] = "6e4f0001-1b9a-4c3a-9a3b-000000000001";
constexpr char kDeviceInfoUuid[] = "6e4f0002-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTablePage0Uuid[] = "6e4f0003-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTablePage1Uuid[] = "6e4f0004-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTablePage2Uuid[] = "6e4f0005-1b9a-4c3a-9a3b-000000000001";
constexpr char kNodeTablePage3Uuid[] = "6e4f0006-1b9a-4c3a-9a3b-000000000001";

constexpr size_t kMaxDeviceInfoPayload = 64;
constexpr size_t kPageSize = 10;

enum class BleReadKind : uint8_t {
  DeviceInfo = 1,
  NodeTablePage0 = 2,
  NodeTablePage1 = 3,
  NodeTablePage2 = 4,
  NodeTablePage3 = 5,
};

constexpr const char* kLogTag = "ble";

inline void log_line(platform::ILogger* logger, const char* msg) {
  if (!logger || !msg) {
    return;
  }
  logger->log(platform::LogLevel::kInfo, kLogTag, msg);
}

inline void log_event(domain::Logger* logger,
                      domain::LogEventId event_id,
                      const uint8_t* payload,
                      uint8_t len) {
  if (!logger) {
    return;
  }
  logger->log(uptime_ms(), event_id, domain::LogLevel::kInfo, payload, len);
}

class NavigaServerCallbacks : public BLEServerCallbacks {
 public:
  NavigaServerCallbacks(platform::ILogger* logger, domain::Logger* event_logger)
      : logger_(logger), event_logger_(event_logger) {}

  void onConnect(BLEServer* /*server*/) override {
    log_line(logger_, "BLE: connected");
    log_event(event_logger_, domain::LogEventId::BLE_CONNECT, nullptr, 0);
  }
  void onDisconnect(BLEServer* server) override {
    log_line(logger_, "BLE: disconnected");
    log_event(event_logger_, domain::LogEventId::BLE_DISCONNECT, nullptr, 0);
    server->getAdvertising()->start();
  }

 private:
  platform::ILogger* logger_;
  domain::Logger* event_logger_;
};

class DeviceInfoCallbacks : public BLECharacteristicCallbacks {
 public:
  DeviceInfoCallbacks(const DeviceInfoData& info, domain::Logger* event_logger)
      : info_(info), event_logger_(event_logger) {}

  void onRead(BLECharacteristic* characteristic) override {
    const uint8_t kind = static_cast<uint8_t>(BleReadKind::DeviceInfo);
    log_event(event_logger_, domain::LogEventId::BLE_READ, &kind, 1);
    std::array<uint8_t, kMaxDeviceInfoPayload> buffer{};
    size_t offset = 0;

    auto append_u8 = [&](uint8_t value) {
      if (offset < buffer.size()) {
        buffer[offset++] = value;
      }
    };
    auto append_u16_le = [&](uint16_t value) {
      if (offset + 1 < buffer.size()) {
        buffer[offset++] = static_cast<uint8_t>(value & 0xFF);
        buffer[offset++] = static_cast<uint8_t>((value >> 8) & 0xFF);
      }
    };
    auto append_u64_le = [&](uint64_t value) {
      for (int i = 0; i < 8; ++i) {
        if (offset < buffer.size()) {
          buffer[offset++] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
        }
      }
    };
    auto append_str = [&](const char* value) {
      if (!value) {
        append_u8(0);
        return;
      }
      const size_t len = std::min<size_t>(255, std::strlen(value));
      append_u8(static_cast<uint8_t>(len));
      const size_t copy_len = std::min(len, buffer.size() - offset);
      std::memcpy(buffer.data() + offset, value, copy_len);
      offset += copy_len;
    };

    // Payload (little-endian): fw_version, hw_profile, band_id, public_channel_id,
    // full_id_u64, short_id.
    append_str(info_.fw_version);
    append_str(info_.hw_profile_name);
    append_u16_le(info_.band_id);
    append_u8(info_.public_channel_id);
    append_u64_le(info_.full_id_u64);
    append_u16_le(info_.short_id);

    characteristic->setValue(buffer.data(), offset);
  }

 private:
  DeviceInfoData info_;
  domain::Logger* event_logger_ = nullptr;
};

class NodeTablePageCallbacks : public BLECharacteristicCallbacks {
 public:
  NodeTablePageCallbacks(const NodeTable* node_table,
                         size_t page_index,
                         BleReadKind read_kind,
                         domain::Logger* event_logger)
      : node_table_(node_table),
        page_index_(page_index),
        read_kind_(read_kind),
        event_logger_(event_logger) {}

  void onRead(BLECharacteristic* characteristic) override {
    const uint8_t kind = static_cast<uint8_t>(read_kind_);
    log_event(event_logger_, domain::LogEventId::BLE_READ, &kind, 1);
    if (!node_table_) {
      characteristic->setValue(nullptr, 0);
      return;
    }
    std::array<uint8_t, NodeTable::kEntryBytes * kPageSize> buffer{};
    const size_t bytes = node_table_->get_page(page_index_, kPageSize, buffer.data(),
                                               buffer.size());
    characteristic->setValue(buffer.data(), bytes);
  }

 private:
  const NodeTable* node_table_;
  size_t page_index_;
  BleReadKind read_kind_;
  domain::Logger* event_logger_ = nullptr;
};

} // namespace

void BleService::init(const DeviceInfoData& info,
                      const NodeTable* node_table,
                      platform::ILogger* logger,
                      domain::Logger* event_logger) {
  node_table_ = node_table;
  logger_ = logger;
  event_logger_ = event_logger;

  BLEDevice::init("Naviga");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new NavigaServerCallbacks(logger_, event_logger_));

  BLEService* service = server->createService(kServiceUuid);

  auto* device_info = service->createCharacteristic(kDeviceInfoUuid, BLECharacteristic::PROPERTY_READ);
  device_info->setCallbacks(new DeviceInfoCallbacks(info, event_logger_));

  auto* page0 = service->createCharacteristic(kNodeTablePage0Uuid, BLECharacteristic::PROPERTY_READ);
  auto* page1 = service->createCharacteristic(kNodeTablePage1Uuid, BLECharacteristic::PROPERTY_READ);
  auto* page2 = service->createCharacteristic(kNodeTablePage2Uuid, BLECharacteristic::PROPERTY_READ);
  auto* page3 = service->createCharacteristic(kNodeTablePage3Uuid, BLECharacteristic::PROPERTY_READ);

  page0->setCallbacks(new NodeTablePageCallbacks(node_table_, 0, BleReadKind::NodeTablePage0,
                                                 event_logger_));
  page1->setCallbacks(new NodeTablePageCallbacks(node_table_, 1, BleReadKind::NodeTablePage1,
                                                 event_logger_));
  page2->setCallbacks(new NodeTablePageCallbacks(node_table_, 2, BleReadKind::NodeTablePage2,
                                                 event_logger_));
  page3->setCallbacks(new NodeTablePageCallbacks(node_table_, 3, BleReadKind::NodeTablePage3,
                                                 event_logger_));

  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();

  log_line(logger_, "BLE: started (Naviga service)");
}

} // namespace naviga
