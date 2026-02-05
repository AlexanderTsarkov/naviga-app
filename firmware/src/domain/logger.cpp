#include "domain/logger.h"

#include <cstring>

namespace naviga {
namespace domain {

Logger::Logger() : buffer_(storage_), capacity_(kDefaultRingSize) {}

Logger::Logger(uint8_t* storage, size_t capacity)
    : buffer_(storage), capacity_(capacity) {}

bool Logger::log(uint32_t t_ms, LogEventId event_id, LogLevel level) {
  return log(t_ms, event_id, level, nullptr, 0);
}

bool Logger::log(uint32_t t_ms,
                 LogEventId event_id,
                 LogLevel level,
                 const uint8_t* payload,
                 uint8_t len) {
  if (!buffer_ || capacity_ == 0) {
    return false;
  }
  if (len > 0 && !payload) {
    return false;
  }

  const size_t record_size = kHeaderSize + len;
  if (!ensure_space(record_size)) {
    return false;
  }

  uint8_t header[kHeaderSize] = {};
  write_u32_le(header, t_ms);
  write_u16_le(header + 4, static_cast<uint16_t>(event_id));
  header[6] = static_cast<uint8_t>(level);
  header[7] = len;

  write_bytes(header, sizeof(header));
  if (len > 0) {
    write_bytes(payload, len);
  }

  size_ += record_size;
  return true;
}

size_t Logger::size() const {
  return size_;
}

size_t Logger::capacity() const {
  return capacity_;
}

void Logger::clear() {
  head_ = 0;
  tail_ = 0;
  size_ = 0;
}

void Logger::for_each_record(RecordCallback cb, void* ctx) const {
  if (!cb || size_ == 0) {
    return;
  }

  size_t offset = 0;
  size_t index = tail_;
  size_t remaining = size_;

  while (remaining >= kHeaderSize) {
    uint8_t header[kHeaderSize] = {};
    read_bytes(index, header, kHeaderSize);
    const uint8_t len = header[7];
    const size_t record_size = kHeaderSize + len;
    if (record_size > remaining) {
      return;
    }

    LogRecordView record{};
    record.t_ms = read_u32_le(header);
    record.event_id = static_cast<LogEventId>(read_u16_le(header + 4));
    record.level = static_cast<LogLevel>(header[6]);
    record.len = len;

    uint8_t payload[255] = {};
    if (len > 0) {
      read_bytes((index + kHeaderSize) % capacity_, payload, len);
      record.payload = payload;
    } else {
      record.payload = nullptr;
    }

    cb(ctx, record);

    offset += record_size;
    index = (index + record_size) % capacity_;
    remaining -= record_size;
  }
}

void Logger::drain(RecordCallback cb, void* ctx) {
  for_each_record(cb, ctx);
  clear();
}

size_t Logger::copy_raw(uint8_t* out, size_t max_len) const {
  if (!out || max_len == 0 || size_ == 0) {
    return 0;
  }
  const size_t to_copy = (size_ < max_len) ? size_ : max_len;
  read_bytes(tail_, out, to_copy);
  return to_copy;
}

bool Logger::ensure_space(size_t record_size) {
  if (record_size > capacity_) {
    return false;
  }
  while (size_ + record_size > capacity_) {
    drop_oldest();
  }
  return true;
}

void Logger::drop_oldest() {
  if (size_ < kHeaderSize) {
    clear();
    return;
  }
  const uint8_t len = read_byte((tail_ + 7) % capacity_);
  const size_t record_size = kHeaderSize + len;
  if (record_size > size_) {
    clear();
    return;
  }
  tail_ = (tail_ + record_size) % capacity_;
  size_ -= record_size;
}

uint8_t Logger::read_byte(size_t index) const {
  return buffer_[index % capacity_];
}

void Logger::read_bytes(size_t index, uint8_t* out, size_t len) const {
  if (len == 0) {
    return;
  }
  const size_t start = index % capacity_;
  const size_t first = (start + len <= capacity_) ? len : (capacity_ - start);
  std::memcpy(out, buffer_ + start, first);
  if (first < len) {
    std::memcpy(out + first, buffer_, len - first);
  }
}

void Logger::write_bytes(const uint8_t* data, size_t len) {
  if (len == 0) {
    return;
  }
  const size_t start = head_;
  const size_t first = (start + len <= capacity_) ? len : (capacity_ - start);
  std::memcpy(buffer_ + start, data, first);
  if (first < len) {
    std::memcpy(buffer_, data + first, len - first);
  }
  head_ = (head_ + len) % capacity_;
}

void Logger::write_u32_le(uint8_t* out, uint32_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  out[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  out[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void Logger::write_u16_le(uint8_t* out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

uint32_t Logger::read_u32_le(const uint8_t* in) const {
  return static_cast<uint32_t>(in[0]) |
         static_cast<uint32_t>(in[1]) << 8 |
         static_cast<uint32_t>(in[2]) << 16 |
         static_cast<uint32_t>(in[3]) << 24;
}

uint16_t Logger::read_u16_le(const uint8_t* in) const {
  return static_cast<uint16_t>(in[0]) |
         static_cast<uint16_t>(in[1]) << 8;
}

} // namespace domain
} // namespace naviga
