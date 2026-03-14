#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace naviga {
namespace domain {

/** Max length for canonical node_name (self + remote); single field per product truth. Struct/persistence size. */
constexpr size_t kNodeTableNodeNameMaxLen = 32;
/** S04 #466: Short display label limit: max 24 UTF-8 bytes. Write path must not exceed this to avoid OOB. */
constexpr size_t kNodeTableNodeNameDisplayMaxBytes = 24;

/** Sentinel for snr_last when SNR is not available (e.g. E22 unsupported). Canon: link_metrics_v0. */
constexpr int8_t kSnrLastNa = 127;

struct NodeEntry {
  // ─── Identity / canonical product fields (#419 master table) ─────────────────
  uint64_t node_id = 0;
  uint16_t short_id = 0;
  bool is_self = false;
  bool short_id_collision = false;
  char node_name[kNodeTableNodeNameMaxLen] = {};  ///< Single canonical name (self + remote); BLE yes, persisted yes.

  // ─── Position (normalized product block) ───────────────────────────────────
  bool pos_valid = false;
  int32_t lat_e7 = 0;
  int32_t lon_e7 = 0;
  uint16_t pos_age_s = 0;
  bool    has_pos_flags = false;
  uint8_t pos_flags     = 0x00;
  bool    has_sats      = false;
  uint8_t sats          = 0x00;

  // ─── Receiver-injected / runtime (not persisted; last_seq not in BLE per canon) ─
  int8_t last_rx_rssi = 0;
  int8_t snr_last = kSnrLastNa;  ///< 127 = NA when unsupported; BLE yes, not persisted.
  uint16_t last_seq = 0;         ///< Dedupe key; in NodeTable only, NOT in BLE.
  uint32_t last_seen_ms = 0;
  bool in_use = false;

  // ─── Runtime-local decoder state only (NOT canonical product truth; NOT in BLE; NOT persisted) ─
  // last_core_seq16 set by apply_pos_full (v0.2); used for seq tracking only (#438: no Tail).
  uint16_t last_core_seq16 = 0;
  bool     has_core_seq16  = false;

  // ─── Battery / survivability ──────────────────────────────────────────────
  bool     has_battery       = false;
  uint8_t  battery_percent   = 0xFF;    ///< 0xFF = not present.
  bool     has_uptime        = false;
  uint32_t uptime_sec        = 0xFFFFFFFFu; ///< 0xFFFFFFFF = not present; canon name uptime_10m.

  // ─── Role / profile-driven on-air state ────────────────────────────────────
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

  /** S04 #466: Set authoritative self node_name; marks table dirty for existing persistence. No-op if self entry not present. */
  bool set_self_node_name(const char* name);

  bool upsert_remote(uint64_t node_id,
                     bool pos_valid,
                     int32_t lat_e7,
                     int32_t lon_e7,
                     uint16_t pos_age_s,
                     int8_t last_rx_rssi,
                     uint16_t last_seq,
                     uint32_t now_ms);

  /**
   * Apply Node_Pos_Full v0.2 (#435): position + Pos_Quality in one step.
   * Sets last_core_seq16 := seq16; no Tail ref. Single-packet apply.
   */
  bool apply_pos_full(uint64_t node_id,
                      uint16_t seq16,
                      int32_t lat_e7, int32_t lon_e7,
                      uint8_t fix_type, uint8_t pos_sats,
                      uint8_t pos_accuracy_bucket, uint8_t pos_flags_small,
                      int8_t rssi_dbm,
                      uint32_t now_ms);

  /**
   * Apply Node_Status v0.2 (#435): full status snapshot (operational + informative).
   * Does not update position. Single-packet apply.
   */
  bool apply_status(uint64_t node_id,
                    uint16_t seq16,
                    uint8_t battery_percent, uint8_t battery_est_rem_time,
                    uint8_t tx_power_ch_throttle, uint8_t uptime10m,
                    uint8_t role_id, uint8_t max_silence_10s,
                    uint16_t hw_profile_id, uint16_t fw_version_id,
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

  /** Snapshot time for the current snapshot (for BLE export age/stale). Returns 0 if id mismatch. */
  uint32_t get_snapshot_time_ms(uint16_t snapshot_id) const;
  /** Fill out[] with entries for the given snapshot page. Returns count. Used by BLE bridge for canon export. */
  size_t get_snapshot_page_entries(uint16_t snapshot_id,
                                  size_t page_index,
                                  size_t page_size,
                                  NodeEntry* out,
                                  size_t max_count) const;
  /** Copy entry for node_id into *out. Returns true if found. For BLE targeted read. */
  bool find_entry_by_node_id(uint64_t node_id, NodeEntry* out) const;
  /** Whether entry is stale at snapshot_time_ms. For BLE export is_stale. */
  bool is_stale(const NodeEntry& entry, uint32_t snapshot_time_ms) const { return is_grey(entry, snapshot_time_ms); }

  // #418: persistence dirty tracking and restore.
  bool is_dirty() const { return dirty_; }
  void clear_dirty() { dirty_ = false; }
  /** Call fn for each in-use entry (order unspecified). Used by snapshot build. */
  void for_each_used_entry(std::function<void(const NodeEntry&)> fn) const;
  /** Replace table with entries (e.g. from restore). Does not set dirty. */
  void restore_from_entries(const NodeEntry* entries, size_t count);

 private:
  void set_dirty() { dirty_ = true; }
  uint16_t expected_interval_s_ = 0;
  uint16_t grace_s_ = 0;

  std::array<NodeEntry, kMaxNodes> entries_{};
  size_t size_ = 0;
  int self_index_ = -1;
  bool dirty_ = false;  ///< #418: set on any mutation; cleared after snapshot save.

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
