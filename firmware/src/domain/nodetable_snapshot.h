#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {
namespace domain {

struct NodeEntry;
class NodeTable;

/** Persistence-specific record size (#418 narrow subset). Not the 26-byte BLE record. last_seen_ms not persisted (uptime, not reboot-safe). */
constexpr size_t kNodeTableSnapshotRecordBytes = 37;

/** Build snapshot blob from live table (narrow persisted subset only). Returns bytes written or 0 on error. */
size_t build_nodetable_snapshot(const NodeTable& table,
                                uint8_t* out,
                                size_t out_cap);

/**
 * Restore from snapshot blob into pre-allocated entries; sets derived fields (is_self, short_id_collision,
 * last_seq, last_rx_rssi, last_applied_tail_ref* to 0/false). last_seen_ms set to 0 (no persisted presence anchor; canon: uptime not reboot-safe). Returns count of entries restored, or 0 on error.
 */
size_t restore_from_nodetable_snapshot(const uint8_t* data,
                                       size_t len,
                                       uint64_t self_node_id,
                                       NodeEntry* out_entries,
                                       size_t max_entries);

}  // namespace domain
}  // namespace naviga
