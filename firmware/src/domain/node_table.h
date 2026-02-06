#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace naviga {

struct NodeEntry {
  uint64_t node_id = 0;
  uint16_t short_id = 0;
  bool is_self = false;
  bool pos_valid = false;
  bool short_id_collision = false;
  int32_t lat_e7 = 0;
  int32_t lon_e7 = 0;
  uint16_t pos_age_s = 0;
  int8_t last_rx_rssi = 0;
  uint16_t last_seq = 0;
  uint32_t last_seen_ms = 0;
  bool in_use = false;
};

class NodeTable {
 public:
  static constexpr size_t kMaxNodes = 100;
  static constexpr size_t kRecordBytes = 26;
  static constexpr size_t kDefaultPageSize = 10;

  static uint16_t compute_short_id(uint64_t node_id);

  void set_expected_interval_s(uint16_t expected_interval_s);

  void init_self(uint64_t node_id, uint32_t now_ms);
  void update_self_position(int32_t lat_e7, int32_t lon_e7, uint16_t pos_age_s, uint32_t now_ms);
  void touch_self(uint32_t now_ms);

  bool upsert_remote(uint64_t node_id,
                     bool pos_valid,
                     int32_t lat_e7,
                     int32_t lon_e7,
                     uint16_t pos_age_s,
                     int8_t last_rx_rssi,
                     uint16_t last_seq,
                     uint32_t now_ms);

  size_t size() const;

  size_t get_page(uint32_t now_ms,
                  size_t page_index,
                  size_t page_size,
                  uint8_t* out_buffer,
                  size_t out_capacity) const;

  uint16_t create_snapshot(uint32_t now_ms);
  size_t get_snapshot_page(uint16_t snapshot_id,
                           size_t page_index,
                           size_t page_size,
                           uint8_t* out_buffer,
                           size_t out_capacity) const;

 private:
  uint16_t expected_interval_s_ = 0;
  uint16_t grace_s_ = 0;

  std::array<NodeEntry, kMaxNodes> entries_{};
  size_t size_ = 0;
  int self_index_ = -1;

  uint16_t snapshot_id_ = 0;
  uint32_t snapshot_time_ms_ = 0;
  std::array<size_t, kMaxNodes> snapshot_indices_{};
  size_t snapshot_count_ = 0;

  int find_entry_index(uint64_t node_id) const;
  int find_free_index() const;
  bool evict_oldest_grey(uint32_t now_ms);
  void recompute_collisions();
  bool is_grey(const NodeEntry& entry, uint32_t now_ms) const;
  static uint16_t compute_grace_s(uint16_t expected_interval_s);
  size_t build_ordered_indices(std::array<size_t, kMaxNodes>& out_indices) const;
};

} // namespace naviga
