# #418 NodeTable snapshot — persisted / runtime / derived classification (corrected canon)

**Issue:** #418 NodeTable persistence snapshot format + restore policy  
**Canon:** product_truth_s03_v1, seq_ref_version_link_metrics_v0, issue_realignment_after_canon_correction  
**Purpose:** Strict classification for snapshot schema; no legacy ref-state in persisted payload.

---

## Persisted (in NVS snapshot blob only)

| Field | NodeEntry | Snapshot | Notes |
|-------|-----------|----------|--------|
| node_id | ✓ | ✓ | Canonical identity. |
| short_id | ✓ | ✓ | Product-facing representation. |
| pos_valid | ✓ | ✓ | Position block. |
| lat_e7, lon_e7 | ✓ | ✓ | Position. |
| pos_age_s | ✓ | ✓ | Node-owned; canon §4. |
| has_pos_flags, pos_flags, has_sats, sats | ✓ | ✓ | Tail-1 quality (position block). |
| has_battery, battery_percent | ✓ | ✓ | Telemetry. |
| has_uptime, uptime_sec | ✓ | ✓ | Telemetry (canon name uptime_10m is #419). |
| has_max_silence, max_silence_10s | ✓ | ✓ | Role/profile. |
| has_hw_profile, hw_profile_id, has_fw_version, fw_version_id | ✓ | ✓ | Identity/build. |

---

## Runtime-only / receiver-injected — NOT persisted

| Field | NodeEntry | BLE | Persisted | Notes |
|-------|-----------|-----|-----------|--------|
| last_seq | ✓ | No | **No** | Dedupe; set 0 on restore. |
| last_seen_ms | ✓ | No | **No** | Internal anchor; restored entries start stale. |
| last_rx_rssi | ✓ | Yes | **No** | Set 0 on restore. |
| snr_last | ✓ (if present) | Yes | **No** | Not in current snapshot. |

---

## Legacy ref-state — NOT in canonical NodeTable; NOT persisted

| Field | Canon | Snapshot | Restore |
|-------|--------|----------|---------|
| last_core_seq16 | §7 removed | **Must not persist** | Set 0. |
| has_core_seq16 | §7 removed | **Must not persist** | Set false. |
| last_applied_tail_ref_core_seq16 | §7 removed | Not persisted | Set 0. |
| has_applied_tail_ref_core_seq16 | §7 removed | Not persisted | Set false. |

---

## Derived on restore — NOT in blob

| Field | Set on restore |
|-------|-----------------|
| is_self | From (node_id == self_node_id). |
| short_id_collision | Recomputed by runtime. |
| last_seen_ms | 0 (no presence anchor → entries start stale). |
| last_seq, last_rx_rssi | 0. |
| last_core_seq16, has_core_seq16 | 0, false. |
| last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16 | 0, false. |
| in_use | true for restored slots. |

---

## Restore semantics (canon-aligned)

- **Restored entries start stale:** last_seen_ms is not persisted; set to 0. last_seen_age_s / is_stale are derived from “no fresh contact” until first RX/TX.
- **No Tail/Core ref in snapshot:** Decoder may keep runtime-local state for Tail apply; snapshot does not store last_core_seq16 or last_applied_tail_ref*.
- **#419 boundary:** Field-map renames (e.g. uptime_10m), node_name, and full master-table alignment are #419. This doc and #418 implementation touch only persistence schema and restore policy.
