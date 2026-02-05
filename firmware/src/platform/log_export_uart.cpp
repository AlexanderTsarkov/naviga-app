#include "platform/log_export_uart.h"

#include <Arduino.h>

namespace naviga {
namespace platform {

namespace {

constexpr size_t kMaxBytesPerDrain = 512;

size_t count_digits_u32(uint32_t value) {
  size_t digits = 1;
  while (value >= 10U) {
    value /= 10U;
    ++digits;
  }
  return digits;
}

size_t count_digits_u16(uint16_t value) {
  size_t digits = 1;
  while (value >= 10U) {
    value /= 10U;
    ++digits;
  }
  return digits;
}

size_t count_digits_u8(uint8_t value) {
  size_t digits = 1;
  while (value >= 10U) {
    value /= 10U;
    ++digits;
  }
  return digits;
}

void print_hex_byte(uint8_t value) {
  const char* digits = "0123456789ABCDEF";
  char out[3] = {digits[(value >> 4) & 0x0F], digits[value & 0x0F], 0};
  Serial.print(out);
}

size_t record_line_len(const domain::LogRecordView& record) {
  // "LOG t_ms=" + t_ms + " event=" + event_id + " level=" + level +
  // " len=" + len + " payload=" + payload_or_dash + "\n"
  size_t len = 0;
  len += 9;  // "LOG t_ms="
  len += count_digits_u32(record.t_ms);
  len += 7;  // " event="
  len += count_digits_u16(static_cast<uint16_t>(record.event_id));
  len += 7;  // " level="
  len += count_digits_u8(static_cast<uint8_t>(record.level));
  len += 5;  // " len="
  len += count_digits_u8(record.len);
  len += 9;  // " payload="
  if (record.len == 0 || !record.payload) {
    len += 2;  // "-\n"
  } else {
    len += static_cast<size_t>(record.len) * 2;
    len += 1;  // "\n"
  }
  return len;
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
  size_t total_len = 0;
  logger.for_each_record(
      [](void* ctx, const domain::LogRecordView& record) {
        auto* total = static_cast<size_t*>(ctx);
        *total += record_line_len(record);
      },
      &total_len);

  if (total_len == 0 || total_len > kMaxBytesPerDrain) {
    return;
  }
  if (Serial.availableForWrite() < total_len) {
    return;
  }

  logger.drain(emit_record, nullptr);
}

} // namespace platform
} // namespace naviga
