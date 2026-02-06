#include "app/m1_runtime.h"

#include <algorithm>

namespace naviga {

namespace {

constexpr uint32_t kBleUpdateIntervalMs = 1000U;
constexpr uint8_t kGeoBeaconMsgType = protocol::kGeoBeaconMsgType;
constexpr size_t kMaxRxPerTick = 4;

} // namespace

void M1Runtime::init(uint64_t self_id,
                     uint16_t short_id,
                     uint32_t now_ms,
                     const protocol::DeviceInfoModel& device_info,
                     IRadio* radio,
                     bool radio_ready,
                     bool rssi_available,
                     domain::Logger* event_logger) {
  radio_ = radio;
  event_logger_ = event_logger;
  radio_ready_ = radio_ready;
  rssi_available_ = rssi_available;
  last_ble_update_ms_ = 0;

  stats_ = {};
  stats_.radio_ready = radio_ready_;
  stats_.rssi_available = rssi_available_;

  node_table_.set_expected_interval_s(10);
  node_table_.init_self(self_id, now_ms);

  self_fields_ = {};
  self_fields_.node_id = self_id;
  self_fields_.pos_valid = 0;
  self_fields_.lat_e7 = 0;
  self_fields_.lon_e7 = 0;
  self_fields_.pos_age_s = 0;
  self_fields_.seq = 0;

  device_info_ = device_info;
  device_info_.node_id = self_id;
  device_info_.short_id = short_id;

  ble_transport_.init();
}

void M1Runtime::set_self_position(bool pos_valid,
                                  int32_t lat_e7,
                                  int32_t lon_e7,
                                  uint16_t pos_age_s,
                                  uint32_t now_ms) {
  if (pos_valid) {
    node_table_.update_self_position(lat_e7, lon_e7, pos_age_s, now_ms);
    self_fields_.pos_valid = 1;
    self_fields_.lat_e7 = lat_e7;
    self_fields_.lon_e7 = lon_e7;
    self_fields_.pos_age_s = pos_age_s;
  } else {
    node_table_.touch_self(now_ms);
    self_fields_.pos_valid = 0;
    self_fields_.lat_e7 = 0;
    self_fields_.lon_e7 = 0;
    self_fields_.pos_age_s = 0;
  }
  log_event(now_ms, domain::LogEventId::NODETABLE_UPDATE, domain::LogLevel::kInfo);
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

void M1Runtime::handle_tx(uint32_t now_ms) {
  uint8_t payload[protocol::kGeoBeaconSize] = {};
  size_t out_len = 0;
  if (!beacon_logic_.build_tx(now_ms, self_fields_, payload, sizeof(payload), &out_len)) {
    return;
  }

  const uint32_t next_seq = stats_.last_seq + 1;
  if (radio_->send(payload, out_len)) {
    stats_.tx_count++;
    stats_.last_tx_ms = now_ms;
    stats_.last_seq = next_seq;
    log_event(now_ms, domain::LogEventId::RADIO_TX_OK, domain::LogLevel::kInfo, &kGeoBeaconMsgType, 1);
  } else {
    stats_.last_seq = next_seq;
    log_event(now_ms, domain::LogEventId::RADIO_TX_ERR, domain::LogLevel::kWarn, &kGeoBeaconMsgType, 1);
  }
}

void M1Runtime::handle_rx(uint32_t now_ms) {
  for (size_t i = 0; i < kMaxRxPerTick; ++i) {
    uint8_t payload[protocol::kGeoBeaconSize] = {};
    size_t out_len = 0;
    if (!radio_->recv(payload, sizeof(payload), &out_len)) {
      return;
    }
    if (out_len == 0 || out_len > sizeof(payload)) {
      log_event(now_ms, domain::LogEventId::RADIO_RX_ERR, domain::LogLevel::kWarn, &kGeoBeaconMsgType, 1);
      continue;
    }

    stats_.rx_count++;
    stats_.last_rx_ms = now_ms;
    if (rssi_available_) {
      stats_.last_rssi_dbm = radio_->last_rssi_dbm();
    }
    log_event(now_ms, domain::LogEventId::RADIO_RX_OK, domain::LogLevel::kInfo, &kGeoBeaconMsgType, 1);

    const bool updated = beacon_logic_.on_rx(now_ms, payload, out_len, stats_.last_rssi_dbm, node_table_);
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
