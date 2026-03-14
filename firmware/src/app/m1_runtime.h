#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/beacon_logic.h"
#include "domain/beacon_send_policy.h"
#include "domain/logger.h"
#include "domain/node_table.h"
#include "domain/nodetable_snapshot.h"
#include "domain/traffic_counters.h"
#include "naviga/hal/interfaces.h"
#include "platform/ble_esp32_transport.h"
#include "../../protocol/ble_status_bridge.h"
#include "../../protocol/ble_node_table_bridge.h"
#include "../../protocol/geo_beacon_codec.h"
#include "services/radio_smoke_service.h"

namespace naviga {

class M1Runtime : public IBleRequestHandler {
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
  /** #425: traffic validation counters (enqueue/sent/drop by type, RX accept/reject). */
  const domain::TrafficCounters& traffic_counters() const { return traffic_counters_; }
  void reset_traffic_counters();

  size_t node_count() const;
  uint16_t geo_seq() const;
  bool ble_connected() const;

  /** Set initial seq16 before first formation (#417 restore). Forwards to BeaconLogic. */
  void set_initial_seq16(uint16_t value);

  /** If a packet was successfully sent, set *out to its seq16 and return true; else return false. Valid for seq16 0 (wraparound). (#417) */
  bool get_last_sent_seq16(uint16_t* out) const;

  /** Optional instrumentation: when set, TX/RX and peer dump are logged. */
  void set_instrumentation_logger(void (*log_line_fn)(const char* line, void* ctx), void* ctx);
  void log_peer_dump(uint32_t now_ms);

  // #418: NodeTable snapshot persistence (narrow format; restore derives is_self, etc.).
  bool nodetable_dirty() const;
  void clear_nodetable_dirty();
  /** Build snapshot blob into out; returns bytes written, 0 on error. */
  size_t build_nodetable_snapshot(uint8_t* out, size_t out_cap);
  /** Restore table from snapshot blob; uses self identity to set is_self. Returns true on success. */
  bool restore_nodetable_snapshot(const uint8_t* data, size_t len);

  /** #450: Copy self entry node_name into out (null-terminated). If no self or empty name, out is empty. */
  void get_self_node_name(char* out, size_t len) const;

  /** #450: Current GNSS fix state from last set_self_position. */
  GNSSFixState get_gnss_fix_state() const { return gnss_snapshot_.fix_state; }

  void on_node_table_request(uint16_t snapshot_id, uint16_t page_index) override;
  void on_targeted_read_request(uint64_t node_id) override;

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
  domain::TrafficCounters traffic_counters_{};

  // TX frame buffer: sized for the largest possible on-air frame.
  uint8_t pending_payload_[protocol::kMaxFrameSize] = {};
  size_t pending_len_ = 0;
  domain::PacketLogType last_tx_type_ = domain::PacketLogType::CORE;
  uint16_t last_tx_core_seq_ = 0;
  uint16_t last_sent_seq16_ = 0;   ///< Seq16 of last successfully sent frame (#417); valid iff has_last_sent_seq16_.
  bool has_last_sent_seq16_ = false;  ///< True after at least one successful TX (so seq16 0 after wrap is valid).
  bool allow_core_send_ = false;
  domain::SelfTelemetry self_telemetry_{};

  void (*instrumentation_log_fn_)(const char* line, void* ctx) = nullptr;
  void* instrumentation_ctx_ = nullptr;

  domain::NodeEntry restore_scratch_[domain::NodeTable::kMaxNodes] = {};
};

} // namespace naviga
