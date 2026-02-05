#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/log_events.h"

namespace naviga {
namespace domain {

enum class LogLevel : uint8_t {
  kDebug = 0,
  kInfo = 1,
  kWarn = 2,
  kError = 3,
};

struct LogRecordView {
  uint32_t t_ms;
  LogEventId event_id;
  LogLevel level;
  const uint8_t* payload;
  uint8_t len;
};

using RecordCallback = void (*)(void* ctx, const LogRecordView& record);

class Logger {
 public:
  static constexpr size_t kDefaultRingSize = 2048;

  Logger();
  Logger(uint8_t* storage, size_t capacity);

  bool log(uint32_t t_ms, LogEventId event_id, LogLevel level);
  bool log(uint32_t t_ms,
           LogEventId event_id,
           LogLevel level,
           const uint8_t* payload,
           uint8_t len);

  size_t size() const;
  size_t capacity() const;
  void clear();

  void for_each_record(RecordCallback cb, void* ctx) const;
  void drain(RecordCallback cb, void* ctx);
  size_t copy_raw(uint8_t* out, size_t max_len) const;

 private:
  static constexpr size_t kHeaderSize = 8;

  bool ensure_space(size_t record_size);
  void drop_oldest();

  uint8_t read_byte(size_t index) const;
  void read_bytes(size_t index, uint8_t* out, size_t len) const;
  void write_bytes(const uint8_t* data, size_t len);

  void write_u32_le(uint8_t* out, uint32_t value);
  void write_u16_le(uint8_t* out, uint16_t value);
  uint32_t read_u32_le(const uint8_t* in) const;
  uint16_t read_u16_le(const uint8_t* in) const;

  uint8_t* buffer_ = nullptr;
  size_t capacity_ = 0;
  size_t head_ = 0;
  size_t tail_ = 0;
  size_t size_ = 0;
  uint8_t storage_[kDefaultRingSize] = {};
};

} // namespace domain
} // namespace naviga
