#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace naviga {
namespace domain {

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
  // Global per-node sequence counter: last accepted seq16 from ANY Node_* packet type
  // (Core, Alive, Tail-1, Tail-2, Info). Used as the (nodeId48, seq16) dedupe key
  // per rx_semantics_v0 §1. MUST NOT be interpreted as "last Core seq" or "last
  // position freshness" — those roles belong to last_core_seq16 and pos_age_s.
  uint16_t last_seq = 0;
  uint32_t last_seen_ms = 0;
  bool in_use = false;

  // Core_Tail binding: seq16 of the last accepted Node_OOTB_Core_Pos for this node.
  // Compared against ref_core_seq16 from incoming Core_Tail frames to enforce
  // the CoreRef-lite gate (tail1_packet_encoding_v0 §4.1).
  uint16_t last_core_seq16 = 0;
  bool     has_core_seq16  = false;  ///< false until first Core received.

  // Variant 2 invariant: at most one Core_Tail per Core_Pos sample.
  // Tracks the ref_core_seq16 of the last successfully applied Core_Tail.
  // If a new Core_Tail arrives with the same ref_core_seq16 (even with a
  // different seq16), it is treated as an unexpected duplicate and ignored.
  uint16_t last_applied_tail_ref_core_seq16 = 0;
  bool     has_applied_tail_ref_core_seq16  = false;  ///< false until first Tail-1 applied.

  // Tail-1 optional fields (position quality for last Core sample).
  bool    has_pos_flags = false;
  uint8_t pos_flags     = 0x00;
  bool    has_sats      = false;
  uint8_t sats          = 0x00;

  // Node_OOTB_Operational (0x04) fields — dynamic runtime.
  bool     has_battery       = false;
  uint8_t  battery_percent   = 0xFF;    ///< 0xFF = not present.
  bool     has_uptime        = false;
  uint32_t uptime_sec        = 0xFFFFFFFFu; ///< 0xFFFFFFFF = not present.

  // Node_OOTB_Informative (0x05) fields — static / user-config.
  bool     has_max_silence   = false;
  uint8_t  max_silence_10s   = 0;       ///< 0 = absent/unknown; unit = 10 s.
  bool     has_hw_profile    = false;
  uint16_t hw_profile_id     = 0xFFFF;  ///< 0xFFFF = not present.
  bool     has_fw_version    = false;
  uint16_t fw_version_id     = 0xFFFF;  ///< 0xFFFF = not present.
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

  /**
   * Apply Tail-1 (Node_OOTB_Core_Tail) fields to an existing NodeTable entry.
   *
   * Enforces the ref_core_seq16 match rule per tail1_packet_encoding_v0 §4.1:
   * - Returns false (silent drop) if no Core has been received for node_id.
   * - Returns false (silent drop) if ref_core_seq16 != last_core_seq16 for this node.
   * - Returns true and applies posFlags/sats only on match.
   * - MUST NOT update position fields.
   *
   * Per rx_semantics_v0: dedupe key is (nodeId48, seq16). If a second Tail-1
   * arrives with the same seq16 for this node, it is treated as a duplicate
   * and ignored (seq16 check happens before ref_core_seq16 check).
   *
   * @param node_id         NodeID48 from the Tail-1 payload.
   * @param seq16           Tail-1's own global per-node counter (for dedupe).
   * @param ref_core_seq16  ref_core_seq16 from the Tail-1 payload (Core linkage key).
   * @param has_pos_flags   Whether posFlags is present.
   * @param pos_flags       posFlags value (ignored if !has_pos_flags).
   * @param has_sats        Whether sats is present.
   * @param sats            sats value (ignored if !has_sats).
   * @param rssi_dbm        Link metric from the received frame.
   * @param now_ms          Current time for lastRxAt update.
   * @return true if applied; false if dropped (duplicate, mismatch, or no prior Core).
   */
  bool apply_tail1(uint64_t node_id,
                   uint16_t seq16,
                   uint16_t ref_core_seq16,
                   bool has_pos_flags, uint8_t pos_flags,
                   bool has_sats, uint8_t sats,
                   int8_t rssi_dbm,
                   uint32_t now_ms);

  /**
   * Apply Tail-2 (Node_OOTB_Operational) fields to a NodeTable entry (create if not found).
   *
   * Deduplication by (nodeId48, seq16) per rx_semantics_v0.
   * MUST NOT update position fields.
   *
   * @param node_id          NodeID48 from the Tail-2 payload.
   * @param seq16            Global per-node counter from Common prefix (for dedupe).
   * @param has_battery      Whether batteryPercent is present.
   * @param battery_percent  batteryPercent value (ignored if !has_battery).
   * @param has_uptime       Whether uptimeSec is present.
   * @param uptime_sec       uptimeSec value (ignored if !has_uptime).
   * @param rssi_dbm         Link metric from the received frame.
   * @param now_ms           Current time for lastRxAt update.
   * @return true on success; false only on table-full eviction failure.
   */
  bool apply_tail2(uint64_t node_id,
                   uint16_t seq16,
                   bool has_battery, uint8_t battery_percent,
                   bool has_uptime, uint32_t uptime_sec,
                   int8_t rssi_dbm,
                   uint32_t now_ms);

  /**
   * Apply Info (Node_OOTB_Informative, 0x05) fields to a NodeTable entry (create if not found).
   *
   * Deduplication by (nodeId48, seq16) per rx_semantics_v0.
   * MUST NOT update position fields.
   *
   * @param node_id          NodeID48 from the Info payload.
   * @param seq16            Global per-node counter from Common prefix (for dedupe).
   * @param has_max_silence  Whether maxSilence10s is present.
   * @param max_silence_10s  maxSilence10s value (ignored if !has_max_silence).
   * @param has_hw_profile   Whether hwProfileId is present.
   * @param hw_profile_id    hwProfileId value (ignored if !has_hw_profile).
   * @param has_fw_version   Whether fwVersionId is present.
   * @param fw_version_id    fwVersionId value (ignored if !has_fw_version).
   * @param rssi_dbm         Link metric from the received frame.
   * @param now_ms           Current time for lastRxAt update.
   * @return true on success; false only on table-full eviction failure.
   */
  bool apply_info(uint64_t node_id,
                  uint16_t seq16,
                  bool has_max_silence, uint8_t max_silence_10s,
                  bool has_hw_profile, uint16_t hw_profile_id,
                  bool has_fw_version, uint16_t fw_version_id,
                  int8_t rssi_dbm,
                  uint32_t now_ms);

#if defined(NAVIGA_TEST)
  /** Test-only: copy the NodeEntry for node_id into *out. Returns false if not found.
   *  Not compiled into production firmware (guarded by NAVIGA_TEST). */
  bool find_entry_for_test(uint64_t node_id, NodeEntry* out) const;
#endif

  size_t size() const;

  size_t get_page(uint32_t now_ms,
                  size_t page_index,
                  size_t page_size,
                  uint8_t* out_buffer,
                  size_t out_capacity) const;

  /** Format one peer for instrumentation dump; peer_index 0-based (0 = first non-self). Returns length or 0. */
  size_t get_peer_dump_line(uint32_t now_ms, size_t peer_index, char* buf, size_t cap) const;

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

} // namespace domain
} // namespace naviga
