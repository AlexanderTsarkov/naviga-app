#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {
namespace domain {

struct NodeEntry;
class NodeTable;

/** Persistence record size v3 (no node_name). #418. */
constexpr size_t kNodeTableSnapshotRecordBytesV3 = 35;
/** Persistence record size v4 (#419): v3 + node_name (1 byte len + 32 bytes). */
constexpr size_t kNodeTableSnapshotRecordBytes = 68;

/** Build snapshot blob from live table (narrow persisted subset only). Returns bytes written or 0 on error. */
size_t build_nodetable_snapshot(const NodeTable& table,
                                uint8_t* out,
                                size_t out_cap);

/**
 * Restore from snapshot blob. Accepts version 3 (35-byte record) or 4 (68-byte, with node_name).
 * Sets derived/runtime and legacy ref fields to 0/false; is_self from self_node_id.
 */
size_t restore_from_nodetable_snapshot(const uint8_t* data,
                                       size_t len,
                                       uint64_t self_node_id,
                                       NodeEntry* out_entries,
                                       size_t max_entries);

}  // namespace domain
}  // namespace naviga
