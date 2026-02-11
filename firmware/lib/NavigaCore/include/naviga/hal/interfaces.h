#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {

enum class GNSSFixState : uint8_t {
  NO_FIX = 0,
  FIX_2D = 1,
  FIX_3D = 2,
};

struct GnssSnapshot {
  GNSSFixState fix_state;
  bool pos_valid;       // true when fix_state is FIX_2D or FIX_3D
  int32_t lat_e7;
  int32_t lon_e7;
  uint32_t last_fix_ms; // monotonic uptime ms of last valid fix; 0 when never fixed
};

class IRadio {
 public:
  virtual ~IRadio() = default;
  virtual bool send(const uint8_t* data, size_t len) = 0;
  virtual bool recv(uint8_t* out, size_t max_len, size_t* out_len) = 0;
  virtual int8_t last_rssi_dbm() const = 0;
};

enum class ChannelSenseState : uint8_t {
  IDLE = 0,
  BUSY = 1,
  UNSUPPORTED = 2,
  ERROR = 3,
};

struct ChannelSenseResult {
  ChannelSenseState state;
  int16_t noise_dbm;
};

class IChannelSense {
 public:
  virtual ~IChannelSense() = default;
  virtual bool can_sense() const = 0;
  virtual ChannelSenseResult sense(uint32_t timeout_ms) = 0;
};

class IBleTransport {
 public:
  virtual ~IBleTransport() = default;
  virtual void set_device_info(const uint8_t* data, size_t len) = 0;
  virtual void set_node_table_response(const uint8_t* data, size_t len) = 0;
  virtual bool get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const = 0;
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
