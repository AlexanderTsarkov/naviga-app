## Summary

Implements **#418**: NodeTable persistence snapshot format + restore policy for the S03 slice. Uses a narrow canon-safe persisted subset; derived/prohibited fields are not baked into persisted truth.

Closes #418.

**Related:** #416 (umbrella), #367, #368 (legacy inputs), #417 (seq16 persistence, already merged).

---

## Changes

- **Platform (`naviga_storage`):** `save_nodetable_snapshot` / `load_nodetable_snapshot` blob API (keys `nt_snap_len`, `nt_snap`). Max blob size 5 KB.
- **Domain:** New `nodetable_snapshot.h/cpp` — persistence-specific **37-byte** record format (not the 26-byte BLE export). `build_nodetable_snapshot`, `restore_from_nodetable_snapshot` with explicit persisted/derived/prohibited handling.
- **NodeTable:** `is_dirty` / `clear_dirty`, `for_each_used_entry`, `restore_from_entries`. Dirty set on every mutation; not set on restore.
- **Runtime:** `nodetable_dirty`, `clear_nodetable_dirty`, `build_nodetable_snapshot`, `restore_nodetable_snapshot` (uses `device_info_.node_id` for `is_self`).
- **App:** After `runtime_.init()` and seq16 restore, load NodeTable snapshot and restore; in `tick()`, save with 30 s debounce when dirty.

---

## Snapshot schema (narrow)

- **Header:** magic "NT", **version 2**, record_count (2 B LE). Version bumped when record layout changed (v1 = 40 B with last_seen_ms; v2 = 37 B without).
- **Record (37 B):** node_id, short_id, pos_valid, lat_e7, lon_e7, pos_age_s, last_core_seq16, has_core_seq16, Tail-1 (pos_flags, sats), telemetry (battery, uptime, max_silence, hw_profile, fw_version).
- **Not persisted:** **last_seen_ms** (canon: monotonic uptime of last presence event; not reboot-safe — restore sets 0; presence re-established by first RX/TX). is_self (derived from local identity on restore), short_id_collision, last_rx_rssi, last_seq, last_applied_tail_ref*.
- **Compatibility:** Reader accepts only v2. Legacy v1 snapshots are rejected (version mismatch → 0 restored entries, clean start). No dual-format read.

---

## Write policy

- Dirty tracking on every NodeTable mutation.
- Full snapshot save only when dirty and ≥30 s since last save (debounced).

---

## Tests

- `test_nodetable_snapshot_restore_is_self_derived` — restore, is_self from self_node_id; restored entries have `last_seen_ms == 0`.
- `test_nodetable_snapshot_excluded_fields_not_authoritative` — last_seen_ms, last_seq, last_rx_rssi, has_applied_tail_ref* are 0/false after restore.
- `test_nodetable_snapshot_corrupt_returns_zero` — bad magic => 0 entries (clean start).
- `test_nodetable_snapshot_old_version_rejected` — v1 blob rejected (version mismatch => 0 entries); no misparse of old layout.
- `test_nodetable_dirty_cleared_after_clear` — dirty flag behavior.

---

## Quality

- CI: firmware build (devkit_e22_oled_gnss) and native nodetable tests pass.
- No platform leakage into domain; no #419 / #421 scope creep.
