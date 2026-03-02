#pragma once

#include <cstddef>
#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {

class MockRadio : public IRadio {
 public:
  bool send(const uint8_t* data, size_t len) override;
  bool recv(uint8_t* out, size_t max_len, size_t* out_len) override;
  int8_t last_rssi_dbm() const override;
  bool rssi_available() const override { return false; }
  RadioBootConfigResult boot_config_result() const override {
    return RadioBootConfigResult::Ok;
  }
  const char* boot_config_message() const override { return ""; }

  void inject_rx(const uint8_t* data, size_t len, int8_t rssi_dbm);
  size_t tx_count() const;
  size_t last_tx_len() const;

 private:
  uint8_t last_tx_[256] = {0};
  size_t last_tx_len_ = 0;
  size_t tx_count_ = 0;

  uint8_t rx_buf_[256] = {0};
  size_t rx_len_ = 0;
  int8_t last_rssi_dbm_ = 0;
  bool has_rx_ = false;
};

} // namespace naviga
