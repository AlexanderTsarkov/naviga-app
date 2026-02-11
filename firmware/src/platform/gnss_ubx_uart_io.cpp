#include "platform/gnss_ubx_uart_io.h"

namespace naviga::platform {

GnssUbxUartIo::GnssUbxUartIo(uint8_t uart_index) : uart_(uart_index) {}

bool GnssUbxUartIo::begin(uint32_t baud, int8_t rx_pin, int8_t tx_pin) {
  uart_.begin(baud, SERIAL_8N1, rx_pin, tx_pin);
  return true;
}

int GnssUbxUartIo::available() {
  return uart_.available();
}

int GnssUbxUartIo::read_byte() {
  return uart_.read();
}

size_t GnssUbxUartIo::write_bytes(const uint8_t* data, size_t len) {
  if (!data || len == 0) {
    return 0;
  }
  return uart_.write(data, len);
}

} // namespace naviga::platform
