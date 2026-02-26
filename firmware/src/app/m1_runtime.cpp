#include "app/m1_runtime.h"

#include <algorithm>
#include <cstdio>

namespace naviga {

namespace {

constexpr uint32_t kBleUpdateIntervalMs = 1000U;
// msg_type byte used in event log payloads (instrumentation only).
constexpr uint8_t kGeoBeaconMsgType = protocol::kGeoBeaconMsgType;
// RX buffer must fit the largest current on-air frame (BeaconCore = 17 B).
constexpr size_t kRxBufSize = protocol::kGeoBeaconFrameSize;
constexpr uint32_t kSenseTimeoutMs = 20U;
constexpr size_t kMaxRxPerTick = 4;

const char* packet_log_type_str(domain::PacketLogType t) {
  switch (t) {
    case domain::PacketLogType::CORE: return "CORE";
    case domain::PacketLogType::TAIL1: return "TAIL1";
    case domain::PacketLogType::TAIL2: return "TAIL2";
    case domain::PacketLogType::ALIVE: return "ALIVE";
  }
  return "?";
}

bool is_tail_type(domain::PacketLogType t) {
  return t == domain::PacketLogType::TAIL1 || t == domain::PacketLogType::TAIL2;
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

size_t M1Runtime::node_count() const {
  return node_table_.size();
}

uint16_t M1Runtime::geo_seq() const {
  return beacon_logic_.seq();
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

void M1Runtime::handle_tx(uint32_t now_ms) {
  if (!send_policy_.has_pending()) {
    size_t out_len = 0;
    domain::PacketLogType tx_type = domain::PacketLogType::CORE;
    uint16_t tx_core_seq = 0;
    if (!beacon_logic_.build_tx(now_ms, self_fields_, pending_payload_, sizeof(pending_payload_),
                                &out_len, &tx_type, &tx_core_seq, allow_core_send_)) {
      return;
    }
    pending_len_ = out_len;
    last_tx_type_ = tx_type;
    last_tx_core_seq_ = tx_core_seq;
    allow_core_send_ = false;  // consumed; next CORE only after next position update
    send_policy_.on_payload_built(now_ms);
  }

  if (!send_policy_.ready_to_attempt(now_ms)) {
    return;
  }

  if (send_policy_.should_sense(channel_sense_)) {
    const ChannelSenseResult result = channel_sense_->sense(kSenseTimeoutMs);
    if (result.state == ChannelSenseState::BUSY || result.state == ChannelSenseState::ERROR) {
      send_policy_.on_channel_busy(now_ms);
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

void M1Runtime::update_ble(uint32_t now_ms) {
  if (last_ble_update_ms_ != 0 && (now_ms - last_ble_update_ms_) < kBleUpdateIntervalMs) {
    return;
  }
  last_ble_update_ms_ = now_ms;
  ble_bridge_.update_all(now_ms, device_info_, node_table_, ble_transport_);
  ble_status_bridge_.update_status(now_ms, gnss_snapshot_, ble_transport_);
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
