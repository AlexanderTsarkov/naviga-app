#include "services/radio_smoke_service.h"

#include <cstring>

namespace naviga {

namespace {

constexpr uint32_t kPingIntervalMs = 1000U;

enum class RadioMsgType : uint8_t {
  Ping = 0x01,
  Pong = 0x02,
};

struct RadioMsg {
  uint8_t type;
  uint32_t seq;
} __attribute__((packed));

static_assert(sizeof(RadioMsg) == 5, "RadioMsg must be packed to 5 bytes.");

} // namespace

void RadioSmokeService::init(IRadio* radio, RadioRole role, bool radio_ready, bool rssi_available) {
  radio_ = radio;
  role_ = role;
  next_ping_ms_ = 0;
  stats_ = {};
  stats_.radio_ready = radio_ready;
  stats_.rssi_available = rssi_available;
}

void RadioSmokeService::tick(uint32_t now_ms) {
  if (!radio_) {
    return;
  }
  if (!stats_.radio_ready) {
    return;
  }

  handle_rx(now_ms);

  if (role_ == RadioRole::INIT && now_ms >= next_ping_ms_) {
    send_ping(now_ms);
    next_ping_ms_ = now_ms + kPingIntervalMs;
  }
}

const RadioSmokeStats& RadioSmokeService::stats() const {
  return stats_;
}

RadioRole RadioSmokeService::role() const {
  return role_;
}

void RadioSmokeService::send_ping(uint32_t now_ms) {
  RadioMsg msg{static_cast<uint8_t>(RadioMsgType::Ping), stats_.last_seq + 1};
  if (radio_->send(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg))) {
    stats_.last_seq = msg.seq;
    stats_.tx_count++;
    stats_.last_tx_ms = now_ms;
  }
}

void RadioSmokeService::send_pong(uint32_t seq, uint32_t now_ms) {
  RadioMsg msg{static_cast<uint8_t>(RadioMsgType::Pong), seq};
  if (radio_->send(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg))) {
    stats_.last_seq = msg.seq;
    stats_.tx_count++;
    stats_.last_tx_ms = now_ms;
  }
}

void RadioSmokeService::handle_rx(uint32_t now_ms) {
  RadioMsg msg{};
  size_t out_len = 0;
  if (!radio_->recv(reinterpret_cast<uint8_t*>(&msg), sizeof(msg), &out_len)) {
    return;
  }
  if (out_len != sizeof(msg)) {
    return;
  }

  if (msg.type == static_cast<uint8_t>(RadioMsgType::Ping)) {
    stats_.last_rx_was_pong = false;
    stats_.rx_count++;
    stats_.last_rx_ms = now_ms;
    if (stats_.rssi_available) {
      stats_.last_rssi_dbm = radio_->last_rssi_dbm();
    }

    if (role_ == RadioRole::RESP) {
      send_pong(msg.seq, now_ms);
    }
  } else if (msg.type == static_cast<uint8_t>(RadioMsgType::Pong)) {
    stats_.last_rx_was_pong = true;
    stats_.rx_count++;
    stats_.last_rx_ms = now_ms;
    stats_.last_seq = msg.seq;
    if (stats_.rssi_available) {
      stats_.last_rssi_dbm = radio_->last_rssi_dbm();
    }
  }
}

} // namespace naviga
