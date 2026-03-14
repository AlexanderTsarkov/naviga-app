# #418 Implementation Summary — NodeTable persistence snapshot + restore

## 1. Final snapshot schema

- **Format:** Binary blob with 5-byte header + N × 40-byte records.
- **Header:** `magic0='N', magic1='T', version=1` (3 bytes), `record_count` (2 bytes LE).
- **Record (40 bytes):** Persisted subset only — `node_id` (8), `short_id` (2), flags (1: pos_valid, has_core_seq16), `lat_e7` (4), `lon_e7` (4), `pos_age_s` (2), `last_seen_ms` (4), `last_core_seq16` (2), optional flags + pos_flags/sats (3), telemetry flags + battery/uptime/max_silence/hw_profile/fw_version (10). **Not** the 26-byte BLE export record.

## 2. Persisted / derived / prohibited

| Category | Fields |
|----------|--------|
| **Persisted** | node_id, short_id, pos_valid, lat_e7, lon_e7, pos_age_s, last_seen_ms, last_core_seq16, has_core_seq16, has_pos_flags, pos_flags, has_sats, sats, has_battery, battery_percent, has_uptime, uptime_sec, has_max_silence, max_silence_10s, has_hw_profile, hw_profile_id, has_fw_version, fw_version_id |
| **Derived on restore** | is_self (from local identity match), short_id_collision (recomputed), freshness/grey/stale/age |
| **Prohibited (not persisted)** | last_rx_rssi, last_seq, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16, node_name (self node_name is separate #368 concern) |

## 3. Restore timing / boot flow

1. `runtime_.init(...)` — local identity (device_info_.node_id) set, NodeTable init_self.
2. Seq16 restore (#417).
3. Load NodeTable snapshot from NVS (`load_nodetable_snapshot`); if present and valid, `runtime_.restore_nodetable_snapshot(buf, len)`.
4. Restore unpacks records, sets `is_self = (node_id == self_node_id)`, clears derived/prohibited; `node_table_.restore_from_entries(...)`; `recompute_collisions()`.
5. Normal RX/TX flow continues.

## 4. Write policy

- **Dirty tracking:** NodeTable sets `dirty_` on every mutation (upsert_remote, apply_tail1/2/info, init_self, update_self_position, touch_self).
- **Debounce:** Full snapshot save only when `nodetable_dirty()` and `(now_ms - last_nodetable_save_ms_) >= 30_000` (30 s min interval).
- **No per-update NVS writes;** single full-snapshot write per debounced save.

## 5. Tests added

- `test_nodetable_snapshot_restore_is_self_derived` — build snapshot from table with self + remote, restore, assert is_self derived from self_node_id and entries present.
- `test_nodetable_snapshot_excluded_fields_not_authoritative` — after restore, last_seq, last_rx_rssi, has_applied_tail_ref_core_seq16 are 0/false.
- `test_nodetable_snapshot_corrupt_returns_zero` — wrong magic => restore returns 0 (clean start).
- `test_nodetable_dirty_cleared_after_clear` — dirty set after init/upsert, clear_dirty clears.

## 6. Deviations from preferred subset

- None. Snapshot persists only the canon-safe subset; is_self, short_id_collision, last_rx_rssi, last_seq, last_applied_tail_ref* are excluded and derived/cleared on restore.
