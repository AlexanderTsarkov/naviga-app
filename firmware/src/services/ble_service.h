#pragma once

#include <cstdint>
#include <cstddef>

#include "domain/logger.h"
#include "naviga/platform/log.h"

namespace naviga {

struct DeviceInfoData {
  const char* fw_version;
  const char* hw_profile_name;
  uint16_t band_id;
  uint8_t public_channel_id;
  uint64_t full_id_u64;
  uint16_t short_id;
};

class NodeTable;

class BleService {
 public:
  void init(const DeviceInfoData& info,
            const NodeTable* node_table,
            platform::ILogger* logger,
            domain::Logger* event_logger);

 private:
  const NodeTable* node_table_ = nullptr;
  platform::ILogger* logger_ = nullptr;
  domain::Logger* event_logger_ = nullptr;
};

} // namespace naviga
