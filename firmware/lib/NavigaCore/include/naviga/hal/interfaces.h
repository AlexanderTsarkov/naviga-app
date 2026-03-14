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

enum class RadioBootConfigResult : uint8_t {
  Ok          = 0,
  Repaired    = 1,
  RepairFailed = 2,
};

class IRadio {
 public:
  virtual ~IRadio() = default;
  virtual bool send(const uint8_t* data, size_t len) = 0;
  virtual bool recv(uint8_t* out, size_t max_len, size_t* out_len) = 0;
  virtual int8_t last_rssi_dbm() const = 0;
  virtual bool rssi_available() const = 0;
  virtual RadioBootConfigResult boot_config_result() const = 0;
  virtual const char* boot_config_message() const = 0;
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
  virtual void set_status(const uint8_t* data, size_t len) = 0;
  virtual bool get_node_table_request(uint16_t* snapshot_id, uint16_t* page_index) const = 0;
  virtual void set_targeted_read_response(const uint8_t* data, size_t len) = 0;
  virtual void set_targeted_read_request(uint64_t node_id) = 0;
  virtual bool get_targeted_read_request(uint64_t* node_id) const = 0;
  virtual const uint8_t* targeted_read_response_data() const = 0;
  virtual size_t targeted_read_response_len() const = 0;
  /** S04 #465: Subscription batch payload (1 byte count + N × 72-byte records). Set then send. */
  virtual void set_subscription_update_payload(const uint8_t* data, size_t len) = 0;
  virtual void send_subscription_update() = 0;

  /** S04 #467: Profiles list payload (n_radio, radio_ids, n_user, user_ids). Single read. */
  virtual void set_profiles_list(const uint8_t* data, size_t len) = 0;
  virtual const uint8_t* profiles_list_data() const = 0;
  virtual size_t profiles_list_len() const = 0;
  /** S04 #467: Profile read request (type + id); response (one profile). Write-then-read. */
  virtual bool get_profile_read_request(uint8_t* type, uint32_t* id) const = 0;
  virtual bool has_profile_read_request() const = 0;
  virtual void set_profile_read_response(const uint8_t* data, size_t len) = 0;
  virtual const uint8_t* profile_read_response_data() const = 0;
  virtual size_t profile_read_response_len() const = 0;
  virtual void clear_profile_read_request() = 0;
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
