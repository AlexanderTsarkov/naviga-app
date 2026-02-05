#include "naviga/hal/mocks/mock_radio.h"

#include <algorithm>
#include <cstring>

namespace naviga {

bool MockRadio::send(const uint8_t* data, size_t len) {
  const size_t copy_len = std::min(len, sizeof(last_tx_));
  if (data && copy_len > 0) {
    std::memcpy(last_tx_, data, copy_len);
  }
  last_tx_len_ = copy_len;
  tx_count_++;
  return true;
}

bool MockRadio::recv(uint8_t* out, size_t max_len, size_t* out_len) {
  if (!has_rx_) {
    if (out_len) {
      *out_len = 0;
    }
    return false;
  }
  const size_t copy_len = std::min(rx_len_, max_len);
  if (out && copy_len > 0) {
    std::memcpy(out, rx_buf_, copy_len);
  }
  if (out_len) {
    *out_len = copy_len;
  }
  has_rx_ = false;
  rx_len_ = 0;
  return true;
}

int8_t MockRadio::last_rssi_dbm() const {
  return last_rssi_dbm_;
}

void MockRadio::inject_rx(const uint8_t* data, size_t len, int8_t rssi_dbm) {
  const size_t copy_len = std::min(len, sizeof(rx_buf_));
  if (data && copy_len > 0) {
    std::memcpy(rx_buf_, data, copy_len);
  }
  rx_len_ = copy_len;
  last_rssi_dbm_ = rssi_dbm;
  has_rx_ = true;
}

size_t MockRadio::tx_count() const {
  return tx_count_;
}

size_t MockRadio::last_tx_len() const {
  return last_tx_len_;
}

} // namespace naviga
