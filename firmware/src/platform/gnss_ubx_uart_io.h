#pragma once

#include <cstddef>
#include <cstdint>

#include <Arduino.h>

#include "services/gnss_ublox_service.h"

namespace naviga::platform {

class GnssUbxUartIo : public IGnssUbxIo {
 public:
  explicit GnssUbxUartIo(uint8_t uart_index);

  bool begin(uint32_t baud, int8_t rx_pin, int8_t tx_pin) override;
  int available() override;
  int read_byte() override;
  size_t write_bytes(const uint8_t* data, size_t len) override;

 private:
  HardwareSerial uart_;
};

} // namespace naviga::platform
