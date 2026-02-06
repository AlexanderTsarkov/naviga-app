#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/beacon_logic.h"
#include "domain/logger.h"
#include "domain/node_table.h"
#include "naviga/hal/interfaces.h"
#include "platform/ble_esp32_transport.h"
#include "../../protocol/ble_node_table_bridge.h"
#include "../../protocol/geo_beacon_codec.h"
#include "services/radio_smoke_service.h"

namespace naviga {

class M1Runtime {
 public:
  void init(uint64_t self_id,
            uint16_t short_id,
            uint32_t now_ms,
            const protocol::DeviceInfoModel& device_info,
            IRadio* radio,
            bool radio_ready,
            bool rssi_available,
            domain::Logger* event_logger);

  void set_self_position(bool pos_valid,
                         int32_t lat_e7,
                         int32_t lon_e7,
                         uint16_t pos_age_s,
                         uint32_t now_ms);

  void tick(uint32_t now_ms);

  const RadioSmokeStats& stats() const;
  size_t node_count() const;

 private:
  void handle_tx(uint32_t now_ms);
  void handle_rx(uint32_t now_ms);
  void update_ble(uint32_t now_ms);
  void log_event(uint32_t now_ms, domain::LogEventId event_id, domain::LogLevel level);
  void log_event(uint32_t now_ms,
                 domain::LogEventId event_id,
                 domain::LogLevel level,
                 const uint8_t* payload,
                 uint8_t len);

  domain::NodeTable node_table_{};
  domain::BeaconLogic beacon_logic_{};
  protocol::BleNodeTableBridge ble_bridge_{};
  BleEsp32Transport ble_transport_{};
  protocol::DeviceInfoModel device_info_{};
  protocol::GeoBeaconFields self_fields_{};

  IRadio* radio_ = nullptr;
  domain::Logger* event_logger_ = nullptr;
  bool radio_ready_ = false;
  bool rssi_available_ = false;
  uint32_t last_ble_update_ms_ = 0;

  RadioSmokeStats stats_{};
};

} // namespace naviga
