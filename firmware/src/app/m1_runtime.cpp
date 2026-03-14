#include "app/m1_runtime.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "platform/ble_esp32_transport.h"
#include "platform/timebase.h"

namespace naviga {

namespace {

constexpr uint32_t kBleUpdateIntervalMs = 1000U;
// msg_type byte used in event log payloads (instrumentation only).
constexpr uint8_t kGeoBeaconMsgType = protocol::kGeoBeaconMsgType;
// RX buffer must fit the largest current on-air frame.
// kMaxFrameSize = kHeaderSize(2) + kMaxPayloadLen(63) = 65 B — covers all packet types.
constexpr size_t kRxBufSize = protocol::kMaxFrameSize;
constexpr uint32_t kSenseTimeoutMs = 20U;
constexpr size_t kMaxRxPerTick = 4;

const char* packet_log_type_str(domain::PacketLogType t) {
  switch (t) {
    case domain::PacketLogType::CORE:     return "CORE";
    case domain::PacketLogType::TAIL1:   return "TAIL1";
    case domain::PacketLogType::TAIL2:   return "TAIL2";
    case domain::PacketLogType::ALIVE:   return "ALIVE";
    case domain::PacketLogType::INFO:    return "INFO";
    case domain::PacketLogType::POS_FULL: return "POS_FULL";
    case domain::PacketLogType::STATUS:   return "STATUS";
  }
  return "?";
}

bool is_tail_type(domain::PacketLogType t) {
  return t == domain::PacketLogType::TAIL1 ||
         t == domain::PacketLogType::TAIL2 ||
         t == domain::PacketLogType::INFO;
}

static void increment_tx_sent_by_type(domain::TrafficCounters& c, domain::PacketLogType t) {
  switch (t) {
    case domain::PacketLogType::POS_FULL: c.tx_sent_pos_full++; break;
    case domain::PacketLogType::ALIVE:   c.tx_sent_alive++;   break;
    case domain::PacketLogType::STATUS:  c.tx_sent_status++;  break;
    default: break;
  }
}

static void increment_rx_ok_by_type(domain::TrafficCounters& c, domain::PacketLogType t) {
  switch (t) {
    case domain::PacketLogType::POS_FULL: c.rx_ok_pos_full++; break;
    case domain::PacketLogType::ALIVE:   c.rx_ok_alive++;   break;
    case domain::PacketLogType::STATUS:  c.rx_ok_status++;  break;
    default: break;
  }
}

} // namespace

void M1Runtime::init(uint64_t self_id,
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
                     IChannelSense* channel_sense) {
  radio_ = radio;
  event_logger_ = event_logger;
  channel_sense_ = channel_sense;
  radio_ready_ = radio_ready;
  rssi_available_ = rssi_available;
  last_ble_update_ms_ = 0;

  stats_ = {};
  stats_.radio_ready = radio_ready_;
  stats_.rssi_available = rssi_available_;

  node_table_.set_expected_interval_s(expected_interval_s);
  node_table_.init_self(self_id, now_ms);

  beacon_logic_.set_min_interval_ms(min_interval_ms);
  beacon_logic_.set_max_silence_ms(max_silence_ms);
  // #422 Path B: Status (Op/Info) lifecycle per packet_context_tx_rules_v0 §2a.
  beacon_logic_.set_min_status_interval_ms(30000);   // 30s anti-burst
  beacon_logic_.set_T_status_max_ms(300000);         // 300s = 10 min bounded refresh
  beacon_logic_.set_traffic_counters(&traffic_counters_);

  self_fields_ = {};
  self_fields_.node_id = self_id;
  self_fields_.pos_valid = 0;
  self_fields_.lat_deg = 0.0;
  self_fields_.lon_deg = 0.0;
  self_fields_.seq = 0;

  device_info_ = device_info;
  device_info_.node_id = self_id;
  device_info_.short_id = short_id;

  send_policy_.init(static_cast<uint32_t>(self_id));
  send_policy_.set_jitter_ms(250);
  send_policy_.set_backoff_ms(200, 2000);
  send_policy_.enable_sense(false);

  ble_transport_.init();
  ble_transport_.set_request_handler(this);
}

void M1Runtime::set_self_position(bool pos_valid,
                                  int32_t lat_e7,
                                  int32_t lon_e7,
                                  uint16_t pos_age_s,
                                  GNSSFixState fix_state,
                                  uint32_t now_ms) {
  if (pos_valid) {
    node_table_.update_self_position(lat_e7, lon_e7, pos_age_s, now_ms);
    self_fields_.pos_valid = 1;
    self_fields_.lat_deg = static_cast<double>(lat_e7) * 1e-7;
    self_fields_.lon_deg = static_cast<double>(lon_e7) * 1e-7;

    gnss_snapshot_.fix_state = fix_state;
    gnss_snapshot_.pos_valid = true;
    gnss_snapshot_.lat_e7 = lat_e7;
    gnss_snapshot_.lon_e7 = lon_e7;
    const uint32_t pos_age_ms = static_cast<uint32_t>(pos_age_s) * 1000U;
    gnss_snapshot_.last_fix_ms = (now_ms >= pos_age_ms) ? (now_ms - pos_age_ms) : 0U;
  } else {
    node_table_.touch_self(now_ms);
    self_fields_.pos_valid = 0;
    self_fields_.lat_deg = 0.0;
    self_fields_.lon_deg = 0.0;

    gnss_snapshot_.fix_state = GNSSFixState::NO_FIX;
    gnss_snapshot_.pos_valid = false;
    gnss_snapshot_.lat_e7 = 0;
    gnss_snapshot_.lon_e7 = 0;
    gnss_snapshot_.last_fix_ms = 0;
  }
  log_event(now_ms, domain::LogEventId::NODETABLE_UPDATE, domain::LogLevel::kInfo);
}

void M1Runtime::set_allow_core_send(bool allow) {
  allow_core_send_ = allow;
}

void M1Runtime::set_self_telemetry(const domain::SelfTelemetry& telemetry) {
  self_telemetry_ = telemetry;
}

void M1Runtime::tick(uint32_t now_ms) {
  if (!radio_ || !radio_ready_) {
    update_ble(now_ms);
    return;
  }

  handle_rx(now_ms);
  handle_tx(now_ms);
  update_ble(now_ms);
}

const RadioSmokeStats& M1Runtime::stats() const {
  return stats_;
}

void M1Runtime::reset_traffic_counters() {
  traffic_counters_ = domain::TrafficCounters{};
}

size_t M1Runtime::node_count() const {
  return node_table_.size();
}

uint16_t M1Runtime::geo_seq() const {
  return beacon_logic_.seq();
}

void M1Runtime::set_initial_seq16(uint16_t value) {
  beacon_logic_.set_initial_seq16(value);
}

bool M1Runtime::get_last_sent_seq16(uint16_t* out) const {
  if (!out || !has_last_sent_seq16_) {
    return false;
  }
  *out = last_sent_seq16_;
  return true;
}

bool M1Runtime::ble_connected() const {
  return ble_transport_.connected();
}

void M1Runtime::set_instrumentation_logger(void (*log_line_fn)(const char* line, void* ctx), void* ctx) {
  instrumentation_log_fn_ = log_line_fn;
  instrumentation_ctx_ = ctx;
}

void M1Runtime::log_peer_dump(uint32_t now_ms) {
  if (!instrumentation_log_fn_ || !instrumentation_ctx_) {
    return;
  }
  constexpr size_t kMaxPeers = 3;
  constexpr size_t kLineCap = 80;
  char line[kLineCap];
  for (size_t i = 0; i < kMaxPeers; ++i) {
    const size_t len = node_table_.get_peer_dump_line(now_ms, i, line, sizeof(line));
    if (len == 0) {
      break;
    }
    instrumentation_log_fn_(line, instrumentation_ctx_);
  }
}

bool M1Runtime::nodetable_dirty() const {
  return node_table_.is_dirty();
}

void M1Runtime::clear_nodetable_dirty() {
  node_table_.clear_dirty();
}

size_t M1Runtime::build_nodetable_snapshot(uint8_t* out, size_t out_cap) {
  if (!out || out_cap == 0) return 0;
  return domain::build_nodetable_snapshot(node_table_, out, out_cap);
}

bool M1Runtime::restore_nodetable_snapshot(const uint8_t* data, size_t len) {
  if (!data || len == 0) return false;
  const uint64_t self_id = device_info_.node_id;
  size_t n = domain::restore_from_nodetable_snapshot(
      data, len, self_id, restore_scratch_, domain::NodeTable::kMaxNodes);
  if (n == 0) return false;
  node_table_.restore_from_entries(restore_scratch_, n);
  return true;
}

void M1Runtime::get_self_node_name(char* out, size_t len) const {
  if (!out || len == 0) return;
  out[0] = '\0';
  node_table_.for_each_used_entry([out, len](const domain::NodeEntry& e) {
    if (e.is_self && e.node_name[0] != '\0') {
      const size_t n = (len - 1) < domain::kNodeTableNodeNameMaxLen ? (len - 1) : domain::kNodeTableNodeNameMaxLen;
      std::strncpy(out, e.node_name, n);
      out[n] = '\0';
    }
  });
}

void M1Runtime::handle_tx(uint32_t now_ms) {
  if (!send_policy_.has_pending()) {
    // Formation pass: update the TX queue with current self state and telemetry.
    beacon_logic_.update_tx_queue(now_ms, self_fields_, self_telemetry_, allow_core_send_);
    if (allow_core_send_) {
      allow_core_send_ = false;  // consumed; next CORE only after next position update
    }

    // Dequeue the best pending frame.
    size_t out_len = 0;
    domain::PacketLogType tx_type = domain::PacketLogType::CORE;
    uint16_t tx_core_seq = 0;
    if (!beacon_logic_.dequeue_tx(pending_payload_, sizeof(pending_payload_),
                                  &out_len, &tx_type, &tx_core_seq)) {
      return;
    }
    pending_len_ = out_len;
    last_tx_type_ = tx_type;
    last_tx_core_seq_ = tx_core_seq;
    send_policy_.on_payload_built(now_ms);
  }

  if (!send_policy_.ready_to_attempt(now_ms)) {
    return;
  }

  if (send_policy_.should_sense(channel_sense_)) {
    const ChannelSenseResult result = channel_sense_->sense(kSenseTimeoutMs);
    if (result.state == ChannelSenseState::BUSY || result.state == ChannelSenseState::ERROR) {
      send_policy_.on_channel_busy(now_ms);
      traffic_counters_.tx_drop_channel_busy++;
      if (instrumentation_log_fn_ && instrumentation_ctx_) {
        char line[96];
        if (is_tail_type(last_tx_type_)) {
          std::snprintf(line, sizeof(line), "pkt drop t_ms=%lu type=%s reason=CHANNEL_BUSY core_seq=%u",
                        static_cast<unsigned long>(now_ms), packet_log_type_str(last_tx_type_),
                        static_cast<unsigned>(last_tx_core_seq_));
        } else {
          std::snprintf(line, sizeof(line), "pkt drop t_ms=%lu type=%s reason=CHANNEL_BUSY",
                        static_cast<unsigned long>(now_ms), packet_log_type_str(last_tx_type_));
        }
        instrumentation_log_fn_(line, instrumentation_ctx_);
      }
      log_event(now_ms, domain::LogEventId::RADIO_TX_ERR, domain::LogLevel::kWarn, &kGeoBeaconMsgType,
                1);
      return;
    }
  }

  // tx_event_seq is an instrumentation TX event counter (uint32); not the on-air seq16 (uint16) from BeaconCore/Alive.
  const uint32_t next_seq = stats_.tx_event_seq + 1;
  const bool ok = radio_->send(pending_payload_, pending_len_);
  send_policy_.on_send_result(ok, now_ms);
  stats_.tx_event_seq = next_seq;
  if (ok) {
    stats_.tx_count++;
    stats_.last_tx_ms = now_ms;
    increment_tx_sent_by_type(traffic_counters_, last_tx_type_);
    // #422: Notify BeaconLogic when Node_Status was sent (status lifecycle: min_status_interval, T_status_max, bootstrap).
    if (last_tx_type_ == domain::PacketLogType::STATUS) {
      beacon_logic_.on_status_sent(now_ms);
    }
    // Persist the seq16 that was actually sent (#417): Common prefix at payload offset 7-8, frame = header(2) + payload.
    // Store value and validity separately so seq16 0 (wraparound) is persisted.
    if (pending_len_ >= 11) {
      last_sent_seq16_ = static_cast<uint16_t>(pending_payload_[9])
          | (static_cast<uint16_t>(pending_payload_[10]) << 8);
      has_last_sent_seq16_ = true;
    }
    if (instrumentation_log_fn_ && instrumentation_ctx_) {
      char line[96];
      if (is_tail_type(last_tx_type_)) {
        std::snprintf(line, sizeof(line), "pkt tx t_ms=%lu type=%s seq=%u core_seq=%u",
                      static_cast<unsigned long>(now_ms), packet_log_type_str(last_tx_type_),
                      static_cast<unsigned>(stats_.tx_event_seq), static_cast<unsigned>(last_tx_core_seq_));
      } else {
        std::snprintf(line, sizeof(line), "pkt tx t_ms=%lu type=%s seq=%u",
                      static_cast<unsigned long>(now_ms), packet_log_type_str(last_tx_type_),
                      static_cast<unsigned>(stats_.tx_event_seq));
      }
      instrumentation_log_fn_(line, instrumentation_ctx_);
    }
    log_event(now_ms, domain::LogEventId::RADIO_TX_OK, domain::LogLevel::kInfo, &kGeoBeaconMsgType, 1);
    pending_len_ = 0;
  } else {
    traffic_counters_.tx_drop_send_fail++;
    if (instrumentation_log_fn_ && instrumentation_ctx_) {
      char line[96];
      if (is_tail_type(last_tx_type_)) {
        std::snprintf(line, sizeof(line), "pkt drop t_ms=%lu type=%s reason=SEND_FAIL core_seq=%u",
                      static_cast<unsigned long>(now_ms), packet_log_type_str(last_tx_type_),
                      static_cast<unsigned>(last_tx_core_seq_));
      } else {
        std::snprintf(line, sizeof(line), "pkt drop t_ms=%lu type=%s reason=SEND_FAIL",
                      static_cast<unsigned long>(now_ms), packet_log_type_str(last_tx_type_));
      }
      instrumentation_log_fn_(line, instrumentation_ctx_);
    }
    log_event(now_ms, domain::LogEventId::RADIO_TX_ERR, domain::LogLevel::kWarn, &kGeoBeaconMsgType, 1);
  }
}

void M1Runtime::handle_rx(uint32_t now_ms) {
  for (size_t i = 0; i < kMaxRxPerTick; ++i) {
    uint8_t frame[kRxBufSize] = {};
    size_t out_len = 0;
    if (!radio_->recv(frame, sizeof(frame), &out_len)) {
      return;
    }
    if (out_len == 0 || out_len > sizeof(frame)) {
      log_event(now_ms, domain::LogEventId::RADIO_RX_ERR, domain::LogLevel::kWarn, &kGeoBeaconMsgType, 1);
      continue;
    }

    stats_.rx_count++;
    stats_.last_rx_ms = now_ms;
    if (rssi_available_) {
      stats_.last_rssi_dbm = radio_->last_rssi_dbm();
    }
    uint64_t rx_node_id = 0;
    uint16_t rx_seq = 0;
    bool rx_pos_valid = false;
    domain::PacketLogType rx_type = domain::PacketLogType::CORE;
    uint16_t rx_core_seq = 0;
    const bool updated = beacon_logic_.on_rx(now_ms, frame, out_len, stats_.last_rssi_dbm,
                                             node_table_, &rx_node_id, &rx_seq, &rx_pos_valid,
                                             &rx_type, &rx_core_seq);
    if (updated) {
      increment_rx_ok_by_type(traffic_counters_, rx_type);
    } else {
      traffic_counters_.rx_reject++;
    }
    if (instrumentation_log_fn_ && instrumentation_ctx_) {
      const uint16_t from_short = domain::NodeTable::compute_short_id(rx_node_id);
      char line[100];
      if (is_tail_type(rx_type)) {
        std::snprintf(line, sizeof(line), "pkt rx t_ms=%lu type=%s seq=%u core_seq=%u from=%u rssi=%d",
                      static_cast<unsigned long>(now_ms), packet_log_type_str(rx_type),
                      static_cast<unsigned>(rx_seq), static_cast<unsigned>(rx_core_seq),
                      static_cast<unsigned>(from_short), static_cast<int>(stats_.last_rssi_dbm));
      } else {
        std::snprintf(line, sizeof(line), "pkt rx t_ms=%lu type=%s seq=%u from=%u rssi=%d",
                      static_cast<unsigned long>(now_ms), packet_log_type_str(rx_type),
                      static_cast<unsigned>(rx_seq), static_cast<unsigned>(from_short),
                      static_cast<int>(stats_.last_rssi_dbm));
      }
      instrumentation_log_fn_(line, instrumentation_ctx_);
    }
    log_event(now_ms, domain::LogEventId::RADIO_RX_OK, domain::LogLevel::kInfo, &kGeoBeaconMsgType, 1);

    if (updated) {
      log_event(now_ms, domain::LogEventId::DECODE_OK, domain::LogLevel::kInfo);
      log_event(now_ms, domain::LogEventId::NODETABLE_UPDATE, domain::LogLevel::kInfo);
    } else {
      log_event(now_ms, domain::LogEventId::DECODE_ERR, domain::LogLevel::kWarn);
    }
  }
}

void M1Runtime::on_node_table_request(uint16_t snapshot_id, uint16_t page_index) {
  const uint32_t now_ms = uptime_ms();
  ble_bridge_.update_node_table(now_ms, node_table_, ble_transport_);
  (void)snapshot_id;
  (void)page_index;
}

void M1Runtime::on_targeted_read_request(uint64_t node_id) {
  const uint32_t now_ms = uptime_ms();
  ble_bridge_.update_targeted_read(now_ms, node_table_, ble_transport_);
  (void)node_id;
}

void M1Runtime::update_ble(uint32_t now_ms) {
  if (last_ble_update_ms_ != 0 && (now_ms - last_ble_update_ms_) < kBleUpdateIntervalMs) {
    return;
  }
  last_ble_update_ms_ = now_ms;
  // BLE request handling (snapshot + targeted read) runs here, not in GATT callback context.
  ble_bridge_.update_all(now_ms, device_info_, node_table_, ble_transport_);
  ble_bridge_.update_targeted_read(now_ms, node_table_, ble_transport_);
  ble_status_bridge_.update_status(now_ms, gnss_snapshot_, ble_transport_);

  char node_name_buf[domain::kNodeTableNodeNameMaxLen];
  get_self_node_name(node_name_buf, sizeof(node_name_buf));
  char display_identity[64];
  if (std::strlen(node_name_buf) > 0) {
    std::snprintf(display_identity, sizeof(display_identity), "%s %04X",
                  node_name_buf, device_info_.short_id);
  } else {
    std::snprintf(display_identity, sizeof(display_identity), "%04X", device_info_.short_id);
  }
  ble_transport_.set_advertising_content(
      display_identity, kBleContractVersionMajor, kBleContractVersionMinor);
}

void M1Runtime::log_event(uint32_t now_ms,
                          domain::LogEventId event_id,
                          domain::LogLevel level) {
  log_event(now_ms, event_id, level, nullptr, 0);
}

void M1Runtime::log_event(uint32_t now_ms,
                          domain::LogEventId event_id,
                          domain::LogLevel level,
                          const uint8_t* payload,
                          uint8_t len) {
  if (!event_logger_) {
    return;
  }
  event_logger_->log(now_ms, event_id, level, payload, len);
}

} // namespace naviga
