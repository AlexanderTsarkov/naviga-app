#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/beacon_logic.h"
#include "domain/beacon_send_policy.h"
#include "domain/logger.h"
#include "domain/node_table.h"
#include "naviga/hal/interfaces.h"
#include "platform/ble_esp32_transport.h"
#include "../../protocol/ble_status_bridge.h"
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
            uint16_t expected_interval_s,
            uint32_t min_interval_ms,
            uint32_t max_silence_ms,
            domain::Logger* event_logger,
            IChannelSense* channel_sense);

  void set_self_position(bool pos_valid,
                         int32_t lat_e7,
                         int32_t lon_e7,
                         uint16_t pos_age_s,
                         GNSSFixState fix_state,
                         uint32_t now_ms);

  /** Set true when SelfUpdatePolicy committed (position update); used for minDisplacement gating. */
  void set_allow_core_send(bool allow);

  /** Update self telemetry used for 0x04/0x05 formation. Call before tick() each cycle. */
  void set_self_telemetry(const domain::SelfTelemetry& telemetry);

  void tick(uint32_t now_ms);

  const RadioSmokeStats& stats() const;
  size_t node_count() const;
  uint16_t geo_seq() const;
  bool ble_connected() const;

  /** Optional instrumentation: when set, TX/RX and peer dump are logged. */
  void set_instrumentation_logger(void (*log_line_fn)(const char* line, void* ctx), void* ctx);
  void log_peer_dump(uint32_t now_ms);

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
  protocol::BleStatusBridge ble_status_bridge_{};
  BleEsp32Transport ble_transport_{};
  protocol::DeviceInfoModel device_info_{};
  protocol::GeoBeaconFields self_fields_{};
  GnssSnapshot gnss_snapshot_{GNSSFixState::NO_FIX, false, 0, 0, 0};
  domain::BeaconSendPolicy send_policy_{};

  IRadio* radio_ = nullptr;
  domain::Logger* event_logger_ = nullptr;
  IChannelSense* channel_sense_ = nullptr;
  bool radio_ready_ = false;
  bool rssi_available_ = false;
  uint32_t last_ble_update_ms_ = 0;

  RadioSmokeStats stats_{};
  // TX frame buffer: sized for the largest possible on-air frame.
  uint8_t pending_payload_[protocol::kMaxFrameSize] = {};
  size_t pending_len_ = 0;
  domain::PacketLogType last_tx_type_ = domain::PacketLogType::CORE;
  uint16_t last_tx_core_seq_ = 0;
  bool allow_core_send_ = false;
  domain::SelfTelemetry self_telemetry_{};

  void (*instrumentation_log_fn_)(const char* line, void* ctx) = nullptr;
  void* instrumentation_ctx_ = nullptr;
};

} // namespace naviga
