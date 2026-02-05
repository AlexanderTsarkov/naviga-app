#include "platform/e220_radio.h"

#include <algorithm>
#include <cstring>

namespace naviga {

namespace {

constexpr UART_BPS_RATE kE220BaudRate = UART_BPS_RATE_9600;

} // namespace

E220Radio::E220Radio(const Pins& pins)
    : pins_(pins),
      radio_(static_cast<byte>(pins.lora_tx), static_cast<byte>(pins.lora_rx), &serial_,
             static_cast<byte>(pins.lora_aux), static_cast<byte>(pins.lora_m0),
             static_cast<byte>(pins.lora_m1), kE220BaudRate, SERIAL_8N1) {}

bool E220Radio::begin() {
  ready_ = radio_.begin();
  if (!ready_) {
    return false;
  }

  ResponseStructContainer config = radio_.getConfiguration();
  if (config.status.code == E220_SUCCESS && config.data) {
    Configuration* cfg = reinterpret_cast<Configuration*>(config.data);
    cfg->TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
    ResponseStatus status = radio_.setConfiguration(*cfg, WRITE_CFG_PWR_DWN_LOSE);
    if (status.code == E220_SUCCESS) {
      rssi_enabled_ = true;
    }
    config.close();
  }

  return ready_;
}

bool E220Radio::is_ready() const {
  return ready_;
}

bool E220Radio::rssi_available() const {
  return rssi_enabled_;
}

bool E220Radio::send(const uint8_t* data, size_t len) {
  if (!ready_ || !data || len == 0 || len > MAX_SIZE_TX_PACKET) {
    return false;
  }
  ResponseStatus status = radio_.sendMessage(data, static_cast<uint8_t>(len));
  return status.code == E220_SUCCESS;
}

bool E220Radio::recv(uint8_t* out, size_t max_len, size_t* out_len) {
  if (!ready_ || !out_len || max_len == 0) {
    return false;
  }
  if (radio_.available() <= 0) {
    *out_len = 0;
    return false;
  }

  ResponseStructContainer response = rssi_enabled_
                                         ? radio_.receiveMessageRSSI(static_cast<uint8_t>(max_len))
                                         : radio_.receiveMessage(static_cast<uint8_t>(max_len));
  if (response.status.code != E220_SUCCESS || !response.data) {
    if (response.data) {
      response.close();
    }
    *out_len = 0;
    return false;
  }

  const size_t copy_len = max_len;
  std::memcpy(out, response.data, copy_len);
  *out_len = copy_len;
  if (rssi_enabled_) {
    last_rssi_dbm_ = static_cast<int8_t>(response.rssi);
  }
  response.close();
  return true;
}

int8_t E220Radio::last_rssi_dbm() const {
  return last_rssi_dbm_;
}

} // namespace naviga
