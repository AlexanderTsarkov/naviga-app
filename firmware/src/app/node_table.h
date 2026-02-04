#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {

struct NodeEntry {
  uint64_t full_id;
  uint16_t short_id;
  int32_t lat_e7;
  int32_t lon_e7;
  uint32_t last_seen_ms;
  uint8_t flags;
};

class NodeTable {
 public:
  static constexpr size_t kDefaultPageSize = 10;
  static constexpr size_t kEntryBytes = 23; // Little-endian packed fields.

  void init_self(uint64_t full_id, uint16_t short_id, uint32_t now_ms);
  void update_self_position(int32_t lat_e7, int32_t lon_e7, uint32_t now_ms);

  size_t size() const;

  // Returns number of bytes written into out_buffer.
  size_t get_page(size_t page_index,
                  size_t page_size,
                  uint8_t* out_buffer,
                  size_t out_capacity) const;

 private:
  bool has_self_ = false;
  NodeEntry self_{};
};

} // namespace naviga
