# NodeTable — Canonical master field table (S03/V1)

**Status:** Canon. **Issue:** #419.  
**Source:** [product_truth_s03_v1](../../../wip/areas/nodetable/product_truth_s03_v1.md), [seq_ref_version_link_metrics_v0](seq_ref_version_link_metrics_v0.md), [nodetable_snapshot_format_v0](nodetable_snapshot_format_v0.md).

This table is the single authoritative classification for NodeTable-related fields. Implementation (NodeEntry, BLE export, persistence) must align with it.

---

## 1) Identity / local representation

| Canonical name | Source | In NodeTable | BLE | Persisted | Authoritative | Notes |
|----------------|--------|--------------|-----|-----------|---------------|--------|
| node_id | identity | Yes | Yes | Yes | Yes | Canonical identity (NodeID48). |
| short_id | identity | Yes | Yes | Yes | Yes | Product-facing; CRC16-derived. |
| is_self | derived | Yes | Yes | No | Derived | From (node_id == local identity). |
| short_id_collision | derived | Yes | Yes | No | Derived | UI safeguard; recomputed. |
| node_name | identity/name | Yes | Yes | Yes | Yes | Single canonical name (self + remote). |

---

## 2) Position (normalized product block)

| Canonical name | Source | In NodeTable | BLE | Persisted | Authoritative | Notes |
|----------------|--------|--------------|-----|-----------|---------------|--------|
| pos_Lat | radio | Yes (lat_e7) | Yes | Yes | Yes | WGS84; stored as lat_e7. |
| pos_Lon | radio | Yes (lon_e7) | Yes | Yes | Yes | WGS84; stored as lon_e7. |
| pos_fix_type | radio | Yes (stub) | Yes | Yes | Yes | 0=none, 1=2d, 2=3d; S03 stub. |
| pos_sats | radio | Yes (sats) | Yes | Yes | Yes | Tail-1; has_sats + sats. |
| pos_accuracy_bucket | radio | Yes (stub) | Yes | Yes | Yes | S03 stub. |
| pos_flags_small | radio | Yes (pos_flags) | Yes | Yes | Yes | Tail-1; has_pos_flags + pos_flags. |
| pos_age_s | derived | Yes | Yes | Yes | Yes | Node-owned position freshness. |

pos_valid: internal; when true, position block is valid. Not a separate canonical “field” name in product truth; implied by position block presence.

---

## 3) Battery / survivability

| Canonical name | Source | In NodeTable | BLE | Persisted | Authoritative | Notes |
|----------------|--------|--------------|-----|-----------|---------------|--------|
| battery_percent | radio | Yes | Yes | Yes | Yes | 0–100; 0xFF = not present. |
| charging | radio | Yes (stub) | Yes | Yes | Yes | S03 stub. |
| battery_est_rem_time | radio | Yes (stub) | Yes | Yes | Yes | S03 stub. |
| uptime | radio | Yes (uptime_sec) | Yes | Yes | Yes | Canon name uptime_10m; stored as uptime_sec; product exposure in 10m units. |

---

## 4) Radio control / role (on-air state)

| Canonical name | Source | In NodeTable | BLE | Persisted | Authoritative | Notes |
|----------------|--------|--------------|-----|-----------|---------------|--------|
| tx_power_step | radio | Yes (stub) | Yes | Yes | Yes | S03 stub; from active radio profile. |
| channel_throttle_step | radio | Yes (stub) | Yes | Yes | Yes | Default 0; S03 stub. |
| role_id | profile | Yes (stub) | Yes | Yes | Yes | From active user profile; S03 stub. |
| max_silence_10s | profile | Yes | Yes | Yes | Yes | From active user profile. |
| hw_profile_id | profile | Yes | Yes | Yes | Yes | Build-time identity. |
| fw_version_id | profile | Yes | Yes | Yes | Yes | Build-time identity. |

---

## 5) Receiver-injected / runtime (not persisted)

| Canonical name | Source | In NodeTable | BLE | Persisted | Authoritative | Notes |
|----------------|--------|--------------|-----|-----------|---------------|--------|
| last_seq | runtime | Yes | **No** | No | Yes | Dedupe; not exposed to BLE per canon. |
| last_seen_ms | runtime | Yes | No | No | Yes | Internal anchor; restored entries start stale. |
| last_rx_rssi | runtime | Yes | Yes | No | Yes | Link metric. |
| snr_last | runtime | Yes | Yes | No | Yes | Link metric; 127 = NA when unsupported. |
| last_payload_version_seen | runtime | Optional | No | No | Debug | Debug-only; not BLE. |

---

## 6) Derived / product-facing

| Canonical name | Source | In NodeTable | BLE | Persisted | Authoritative | Notes |
|----------------|--------|--------------|-----|-----------|---------------|--------|
| last_seen_age_s | derived | Yes | Yes | No | Derived | From last_seen_ms / now. |
| is_stale | derived | Yes | Yes | No | Derived | Activity; restored entries start stale. |
| pos_age_s | derived | Yes | Yes | Yes | Yes | See §2. |

---

## 7) Legacy fields — not in canonical NodeTable (runtime-local decoder only)

The following are **not** part of canonical product truth. They must **not** be in BLE export or persistence. Decoder may keep them in implementation for Tail–Core correlation only.

| Field | In implementation | BLE | Persisted | Notes |
|-------|-------------------|-----|-----------|--------|
| last_core_seq16 | Runtime-local only | No | No | Decoder apply_tail1 gate. |
| has_core_seq16 | Runtime-local only | No | No | Decoder apply_tail1 gate. |
| last_applied_tail_ref_core_seq16 | Runtime-local only | No | No | Decoder Tail dedupe. |
| has_applied_tail_ref_core_seq16 | Runtime-local only | No | No | Decoder Tail dedupe. |

---

## 8) Service-only radio (not required product NodeTable fields)

packet_header, payloadVersion — not stored as product NodeTable fields; packing/unpacking at radio boundary only.

---

## 9) Implementation status (post-#419)

- **Aligned:** node_id, short_id, is_self, short_id_collision, pos (lat_e7, lon_e7, pos_age_s, pos_flags, sats), battery_percent, uptime_sec, max_silence_10s, hw_profile_id, fw_version_id, last_rx_rssi, last_seen_ms, last_seq (in NodeTable only; not BLE).
- **BLE:** last_seq removed from BLE export; snr_last added (sentinel 127 = NA).
- **Legacy ref:** Kept in NodeEntry for decoder only; not in BLE, not in persistence.
- **node_name:** Added to NodeEntry; in persistence snapshot; BLE export per format.
- **Stubs:** pos_fix_type, pos_accuracy_bucket, charging, battery_est_rem_time, tx_power_step, channel_throttle_step, role_id — added as stubs where minimal.

---

## 10) Boundary (not in #419)

- **#420** — TX rule application / formation / scheduling. No change in this issue.
- **#421** — RX semantics / apply rules. apply_tail1/upsert/apply_tail2/apply_info unchanged except NodeEntry shape; decoder still uses runtime-local ref fields for Tail–Core correlation.
- **#422** — Packetization / throttle / queueing redesign. No change in this issue.
