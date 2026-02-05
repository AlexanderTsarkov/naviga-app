#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {

struct GnssSnapshot {
  bool has_fix;
  int32_t lat_e7;
  int32_t lon_e7;
};

class IRadio {
 public:
  virtual ~IRadio() = default;
  virtual bool send(const uint8_t* data, size_t len) = 0;
  virtual bool recv(uint8_t* out, size_t max_len, size_t* out_len) = 0;
  virtual int8_t last_rssi_dbm() const = 0;
};

class IBleTransport {
 public:
  virtual ~IBleTransport() = default;
  virtual void set_device_info(const uint8_t* data, size_t len) = 0;
  virtual void set_node_table_page(uint8_t page_index, const uint8_t* data, size_t len) = 0;
};

class IGnss {
 public:
  virtual ~IGnss() = default;
  virtual bool get_snapshot(GnssSnapshot* out) = 0;
};

class ILog {
 public:
  virtual ~ILog() = default;
  virtual void log(const char* tag, const uint8_t* data, size_t len) = 0;
};

} // namespace naviga
