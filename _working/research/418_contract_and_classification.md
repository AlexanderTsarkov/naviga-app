# #418 Contract and classification (re-verified from repo)

**Issue:** #418 — NodeTable persistence snapshot format + restore policy  
**Canon:** product_truth_s03_v1, seq_ref_version_link_metrics_v0, issue_realignment_after_canon_correction  
**Verified:** 2026-03-11 from current repo (no assumptions from prior partial work).

---

## 1) #418 contract (restated)

- **Snapshot stores:** Identity (node_id, short_id), position block (pos_valid, lat_e7, lon_e7, pos_age_s), Tail-1 quality (has_pos_flags, pos_flags, has_sats, sats), telemetry (has_battery, battery_percent, has_uptime, uptime_sec, has_max_silence, max_silence_10s, has_hw_profile, hw_profile_id, has_fw_version, fw_version_id). One record per in-use node; blob = header (5 B) + N × record (35 B). Format version 3 only.
- **Restore reconstructs:** The above persisted fields into NodeEntry; is_self from (node_id == self_node_id); in_use = true for restored slots.
- **Must not be restored from blob:** last_seen_ms, last_seq, last_rx_rssi, last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16, is_self, short_id_collision. These are set to 0/false (or is_self derived) after unpack.
- **Recomputed / start stale after restore:** last_seen_ms = 0 ⇒ no presence anchor ⇒ restored entries start stale; last_seen_age_s / is_stale derived at runtime until first RX/TX. Legacy ref fields remain 0/false until decoder/runtime set them from live RX.

---

## 2) Classification table (with file-path evidence)

| Field / group | Persisted | Restored (from blob) | Set on restore (0/derived) | BLE | Canon | Evidence |
|---------------|-----------|----------------------|----------------------------|-----|--------|----------|
| node_id | Yes | Yes | — | Yes | §5 | pack u64 @ 0, unpack same — `nodetable_snapshot.cpp` pack_record/unpack_record |
| short_id | Yes | Yes | — | Yes | §5 | pack u16 @ 8 |
| pos_valid | Yes | Yes | — | — | §1 | out[10] bit0 |
| lat_e7, lon_e7 | Yes | Yes | — | — | §1 | put_u32_le 11–18 |
| pos_age_s | Yes | Yes | — | Yes | §4 | put_u16_le 19–20 |
| has_pos_flags, pos_flags, has_sats, sats | Yes | Yes | — | — | §1 | flags2 @ 21, 22–23 |
| has_battery, battery_percent | Yes | Yes | — | — | §1 | flags3 @ 24, 25 |
| has_uptime, uptime_sec | Yes | Yes | — | — | §1 | 26–29 |
| has_max_silence, max_silence_10s | Yes | Yes | — | — | §1 | 30 |
| has_hw_profile, hw_profile_id, has_fw_version, fw_version_id | Yes | Yes | — | — | §1 | 31–34 |
| last_seen_ms | **No** | **No** | 0 | No | §3 | unpack: e->last_seen_ms = 0 |
| last_seq | **No** | **No** | 0 | No | §3 | unpack: e->last_seq = 0 |
| last_rx_rssi | **No** | **No** | 0 | Yes | §3 | unpack: e->last_rx_rssi = 0 |
| last_core_seq16, has_core_seq16 | **No** | **No** | 0, false | No (legacy §7) | §7 removed | unpack: e->last_core_seq16 = 0; e->has_core_seq16 = false |
| last_applied_tail_ref*, has_applied_tail_ref* | **No** | **No** | 0, false | No | §7 removed | unpack: same |
| is_self | No | No | from self_node_id | Yes | §5 | restore_from_nodetable_snapshot: out_entries[n].is_self = (node_id == self_node_id) |
| short_id_collision | No | No | false | Yes | §5 | unpack: e->short_id_collision = false |

---

## 3) Snapshot schema (v3) — current implementation

- **Header:** 5 bytes — magic0, magic1, version (3), count_u16_le.  
- **Record:** 35 bytes — see pack_record in `firmware/src/domain/nodetable_snapshot.cpp`. No last_seen_ms, no last_core_seq16, no has_core_seq16.
- **Version policy:** Only version 3 accepted; v1/v2 rejected (clean start).  
- **Wiring:** Load/save in `app_services.cpp` (restore after runtime_.init(); save in tick with 30 s debounce when nodetable_dirty()). NVS keys `nt_snap_len`, `nt_snap` in `naviga_storage.cpp`.

---

## 4) Boundary vs #419

- **#418 (this issue):** Persistence snapshot format and restore policy only. Persisted set and “must not persist” set aligned with corrected canon. No legacy ref-state in blob. Restored entries start stale.
- **#419:** NodeTable canonical field map / master table alignment (e.g. uptime_10m naming, node_name, full product field set). No change to snapshot record layout in #418 for renames or new fields that #419 introduces.

---

## 5) Tests (evidence)

- `test_nodetable_snapshot_restore_is_self_derived` — round-trip, is_self derived, last_seen_ms 0, pos_age_s round-trip, has_core_seq16/last_core_seq16 false/0.  
- `test_nodetable_snapshot_excluded_fields_not_authoritative` — last_seen_ms, last_seq, last_rx_rssi, has_applied_tail_ref*, has_core_seq16, last_core_seq16 all 0/false after restore.  
- `test_nodetable_snapshot_corrupt_returns_zero` — bad magic ⇒ 0 entries.  
- `test_nodetable_snapshot_old_version_rejected` — v1 and v2 headers ⇒ 0 entries.  
- `test_nodetable_dirty_cleared_after_clear` — dirty flag behavior.  

File: `firmware/test/test_node_table_domain/test_node_table_domain.cpp`. All 20 tests in that file pass (including the five above).

---

## 6) Exit criteria checklist

- [x] #418 contract restated against corrected canon
- [x] Persisted/runtime/derived classification table produced (with file-path evidence)
- [x] Snapshot schema aligned to corrected canon (v3, 35-byte; no legacy ref-state)
- [x] Restore policy aligned to corrected canon (restored entries start stale; legacy ref 0/false)
- [x] Runtime-only and legacy ref fields excluded from persistence
- [x] Restored entries start stale until fresh contact (last_seen_ms = 0)
- [x] Tests added/updated and passing (20/20 in test_node_table_domain)
- [x] Docs updated narrowly: nodetable_snapshot_format_v0.md + index link
- [x] Clear note for #419 boundary (field-map / master table; snapshot layout not extended here)
