#pragma once

#include <cstdint>
#include <cstddef>

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
  void init(const DeviceInfoData& info, const NodeTable* node_table);

 private:
  const NodeTable* node_table_ = nullptr;
};

} // namespace naviga
