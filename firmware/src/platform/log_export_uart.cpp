#include "platform/log_export_uart.h"

#include <Arduino.h>

namespace naviga {
namespace platform {

namespace {

void print_hex_byte(uint8_t value) {
  const char* digits = "0123456789ABCDEF";
  char out[3] = {digits[(value >> 4) & 0x0F], digits[value & 0x0F], 0};
  Serial.print(out);
}

void emit_record(void* /*ctx*/, const domain::LogRecordView& record) {
  Serial.print("LOG t_ms=");
  Serial.print(record.t_ms);
  Serial.print(" event=");
  Serial.print(static_cast<uint16_t>(record.event_id));
  Serial.print(" level=");
  Serial.print(static_cast<uint8_t>(record.level));
  Serial.print(" len=");
  Serial.print(record.len);
  Serial.print(" payload=");
  if (record.len == 0 || !record.payload) {
    Serial.println("-");
    return;
  }
  for (uint8_t i = 0; i < record.len; ++i) {
    print_hex_byte(record.payload[i]);
  }
  Serial.println();
}

} // namespace

void drain_logs_uart(domain::Logger& logger) {
  if (!Serial) {
    return;
  }
  logger.drain(emit_record, nullptr);
}

} // namespace platform
} // namespace naviga
