# PR #429 Final Review — #418 NodeTable persistence

## Verdict

**Mergeable.**

---

## What is correct

- **Snapshot schema:** Persisted set is the agreed narrow subset (node_id, short_id, pos_valid, lat_e7, lon_e7, pos_age_s, last_core_seq16, has_core_seq16, Tail-1 pos_flags/sats, telemetry). `last_seen_ms` is not in the blob; pack does not write it; record is 37 bytes with consistent offsets (0–36).
- **Prohibited/derived:** Unpack explicitly sets `last_seen_ms = 0` and clears is_self, short_id_collision, last_rx_rssi, last_seq, last_applied_tail_ref*; caller sets `is_self = (node_id == self_node_id)`. `restore_from_entries` calls `recompute_collisions()` so short_id_collision is derived. No prohibited field is read from the blob.
- **Format/version:** Version 2 used in build and in the reader check; v1 blobs fail the `data[2] != kSnapshotVersion` check and return 0. Length check uses `count * 37 + 5`; malformed/corrupt input (wrong magic, short length) returns 0.
- **Write policy:** Dirty is set only in NodeTable mutators (upsert_remote, apply_tail*, init_self, update_self_position, touch_self). `restore_from_entries` does not set dirty (comment and no set_dirty call). Save is gated by dirty and 30 s debounce (`kMinNodetableSaveIntervalMs = 30000U`).
- **Scope:** No node_name in NodeEntry or in the snapshot format. No #419 field-map or #421 RX-semantics changes. Persistence is a separate format from BLE export.
- **Tests:** Restore test asserts is_self derived and last_seen_ms == 0 for self and remote. Excluded-fields test asserts last_seen_ms, last_seq, last_rx_rssi, has_applied_tail_ref_core_seq16. Corrupt (bad magic) and old-version (v1 header) tests both expect 0 entries. Dirty test covers set/clear.

---

## Problems found

**None.**

---

## Nits / optional cleanup

**None.** No style or trivial nits that affect merge.

---

## Recommended next action

**Merge PR #429.**

---

## If a fix is needed

N/A — no fix required.

---

## Exit criteria checklist

- [x] Snapshot semantics checked (narrow persisted set; last_seen_ms absent; prohibited not serialized).
- [x] Restore invariants checked (is_self from identity; short_id_collision recomputed; prohibited cleared; last_seen_ms == 0).
- [x] Version/format safety checked (version 2; v1 rejected; record size/offsets consistent; corrupt/malformed fail safely).
- [x] Write policy checked (dirty on mutation only; restore does not set dirty; 30 s debounce).
- [x] Scope checked (no #419/#421/node_name creep).
- [x] Tests checked (restore, excluded fields, corrupt, old-version, dirty).
- [x] Clear merge recommendation: merge PR #429.
