# NodeTable — v0.2 packet truth table (canon)

**Status:** Canon (policy).  
**Issue:** [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435) — Post-P0: v0.2 packet redesign (Path A).  
**Context:** Follow-up after #422 (Path B). P0 chain #417–#422 remains closed; this doc defines the **canonical v0.2** packet family and semantics so implementation PRs can follow without reopening protocol truth.

**#435 contract (short):** Move to canonical v0.2 packet family (Node_Pos_Full, Node_Status, Alive). Staged migration: TX sends v0.2 only; RX accepts v0.1 + v0.2 during transition; later cutover to v0.2-only RX. NodeTable stays normalized product truth; active-values only; hw_profile_id / fw_version_id remain uint16; uptime_10m at canon layer; M1Runtime single composition; domain radio-agnostic.

---

## 1) Canonical v0.2 packet family

Exactly **three** packet types for the beacon/OOTB path. No hybrid “old and new both canon” ambiguity: v0.2 is canon; v0.1 is compatibility-only during transition.

| Packet | Role | Replaces (v0.1) |
|--------|------|------------------|
| **Node_Pos_Full** | Position + quality in one packet; one seq16 per position update. | Core_Pos + Core_Tail (single-packet path). |
| **Node_Status** | Full snapshot of operational + informative status. | Node_Operational (0x04) + Node_Informative (0x05). |
| **Alive** | No-fix liveness; identity + seq16. | **Retained** — unchanged from current v0; explicitly part of v0.2 family. |

- **Wire:** New `msg_type` for Node_Pos_Full and Node_Status; Alive keeps existing. **Implemented:** msg_type 0x06 = Node_Pos_Full, 0x07 = Node_Status (firmware `packet_header.h`); implementation: see [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435) (implementation PR #436).

---

## 2) Per-packet field composition

### 2.1 Common prefix (all v0.2 packets)

- **9 B:** payloadVersion (1 B), nodeId48 (6 B), seq16 (2 B LE). Unchanged from v0.1.

### 2.2 Node_Pos_Full

- **Common prefix:** 9 B.
- **Useful payload:** lat_u24 (3 B), lon_u24 (3 B), **Pos_Quality** (2 B, single uint16 LE).
- **Pos_Quality bit layout:** fix_type (3 b), pos_sats (6 b), pos_accuracy_bucket (3 b), pos_flags_small (4 b). Exact bit positions in encoding contract; semantics per [gnss_tail_completeness_and_budgets_s03](../../../wip/areas/nodetable/contract/gnss_tail_completeness_and_budgets_s03.md).
- **No** ref_core_seq16; no second packet. One seq16 per position update.
- **Total payload:** 9 + 6 + 2 = **17 B**. On-air: 2 (header) + 17 = **19 B**.

### 2.3 Node_Status

- **Common prefix:** 9 B.
- **Useful payload (full snapshot):** Operational: batteryPercent (1 B), battery_Est_Rem_Time (encoding TBD, 1–2 B), TX_power_Ch_throttle (e.g. 1 B), uptime10m (1 B, 10-minute units). Informative: role_id (1 B), maxSilence10s (1 B), hwProfileId (2 B uint16 LE), fwVersionId (2 B uint16 LE). Order and optionality in encoding contract.
- **Product/canon names:** `uptime_10m` at product layer; wire may use `uptime10m`. `hw_profile_id` / `fw_version_id` remain **uint16** unless a documented protocol decision changes them.
- **Total payload (estimate):** 9 + 1 + (1–2) + 1 + 1 + 1 + 1 + 1 + 2 + 2 = **19–20 B**. On-air **21–22 B**. Must fit Default budget (32 B payload).

### 2.4 Alive

- Common 9 B; optional aliveStatus 1 B. Min payload 9 B (on-air 11 B); with aliveStatus 10 B payload (on-air 12 B). Unchanged from current v0.

---

## 3) Trigger and lifecycle (TX semantics)

### 3.1 Node_Pos_Full

- **Trigger:** pos_valid AND (min_interval AND allow_core) OR max_silence. One slot; one seq16 per position update. Earliest_at / deadline per [packet_context_tx_rules_v0](../../radio/policy/packet_context_tx_rules_v0.md) §2.

### 3.2 Node_Status (lifecycle)

Per [packet_context_tx_rules_v0](../../radio/policy/packet_context_tx_rules_v0.md) §2a — unchanged in v0.2:

- **Bootstrap:** status_bootstrap_pending; up to 2 sends; second not earlier than min_status_interval_ms after first.
- **Triggers:** Urgent (TX_power_Ch_throttle, maxSilence10s, role_id); threshold (batteryPercent, battery_Est_Rem_Time); hitchhiker-only (uptime10m, hwProfileId, fwVersionId) included when Status is sent for other reasons.
- **Timing:** min_status_interval_ms = 30 s; T_status_max = 300 s; periodic refresh when no Status sent within T_status_max.
- **No hitchhiking:** Node_Status not enqueued in same formation pass as Node_Pos_Full; standalone only.

### 3.3 Alive

- **Trigger:** !pos_valid AND time_for_silence. One slot; max_silence deadline.

---

## 4) RX / apply consequences

- **Node_Pos_Full:** Single-packet apply. Update: node_id, seq16 (last_seq), last_core_seq16 := seq16, position (lat/lon), Pos_Quality (fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small). **Obsolete in v0.2:** ref_core_seq16 (wire); last_applied_tail_ref_core_seq16 for position path; no Tail to match.
- **Node_Status:** Single apply. Update: node_id, seq16, full status snapshot (battery_percent, uptime_10m, tx_power/channel_throttle, role_id, max_silence_10s, hw_profile_id, fw_version_id, battery_est_rem_time if present). No merge of two packet types.
- **Alive:** Update node_id, seq16, last_seen_ms; do not update position or status.
- **Presence/self:** Self last_seen_ms updated on TX of Node_Pos_Full, Alive (and optionally no longer on a separate Tail send). Node_Status is **non–presence-bearing**: do not update self last_seen_ms on Node_Status TX.
- **Seq/ref:** last_core_seq16 updated only on Node_Pos_Full (or, during compat, on Core_Pos). last_applied_tail_ref_core_seq16 **removed** from product NodeTable when v0.2 is canonical (or kept only in compat path for v0.1 Tail apply). See [seq_ref_version_link_metrics_v0](seq_ref_version_link_metrics_v0.md) for v0.2 update rules.

---

## 5) Boundary: product truth vs wire / service

- **NodeTable** stores **normalized product fields only** per [nodetable_master_field_table_v0](nodetable_master_field_table_v0.md) (#419). No packed radio blobs in product fields.
- **Service-only:** payloadVersion, packet_header — not stored as product NodeTable fields; packing/unpacking at radio boundary only.
- **Active-values only:** Payloads and NodeTable use active values only; no profile refs inside the product field set.
- **Widths:** hw_profile_id and fw_version_id remain **uint16** unless a documented protocol decision says otherwise.
- **Naming:** uptime_10m at product/canon layer; wire may use uptime10m.
- **M1Runtime:** Single composition point; domain stays radio-agnostic.

---

## 6) Airtime / packet-size expectations

| Packet | Payload (B) | On-air (B) | LongDist (24) | Default (32) | Fast (40) |
|--------|-------------|------------|---------------|--------------|-----------|
| Node_Pos_Full | 17 | 19 | ✓ | ✓ | ✓ |
| Node_Status | ~19–20 | ~21–22 | ✓ | ✓ | ✓ |
| Alive | 9–10 | 11–12 | ✓ | ✓ | ✓ |

- **Node_Pos_Full vs Core+Tail:** One packet 19 B vs two (17+15)=32 B; saves one packet per position update; see [traffic_model_v0](../../radio/policy/traffic_model_v0.md).
- **Implementation:** Encoding contract must fix Node_Status payload size; unit or integration checks that payload sizes do not exceed profile budgets (LongDist 24, Default 32, Fast 40).

---

## 7) Migration / compatibility policy

See [packet_migration_v01_v02](packet_migration_v01_v02.md) for the explicit compatibility memo. Summary:

- **Transition:** TX sends v0.2 only; RX accepts v0.2 **and** v0.1 (Core_Pos, Core_Tail, Operational, Informative).
- **Post-cutover:** TX v0.2 only; RX v0.2 only; v0.1 optionally log & drop.
- **No dual path:** Sending both v0.1 and v0.2 is not in scope.

---

## 8) Later implementation PR(s) — scope

Implementation PR(s) **must** (this doc is design-only; no code in this step):

- Implement TX formation for Node_Pos_Full, Node_Status, Alive per this truth table and [packet_context_tx_rules_v0](../../radio/policy/packet_context_tx_rules_v0.md).
- Implement encode/decode and RX dispatch/apply; compatibility layer to accept v0.1 during transition; apply to same NodeTable normalized fields.
- Implement Node_Status lifecycle (bootstrap, triggers, min_status_interval_ms, T_status_max).
- Add encoding contract for exact byte order and optionality (Node_Status).
- Add unit/integration tests; packet-size budget checks; devkit build; docs/inventory update.

---

## 9) Later compatibility-cleanup PR — scope

After cutover, a **cleanup PR** may:

- Remove v0.1 accept path from RX decode/dispatch.
- Remove last_applied_tail_ref_core_seq16 from product surface (or confine to legacy compat only).
- Update [seq_ref_version_link_metrics_v0](seq_ref_version_link_metrics_v0.md) and related policy to state v0.2-only rules.

---

## 10) Related

- [packet_context_tx_rules_v0](../../radio/policy/packet_context_tx_rules_v0.md) — TX rules and Node_Status lifecycle §2/§2a.
- [packet_sets_v0](packet_sets_v0.md) — Packet sets v0.1; v0.2 supersedes for canon.
- [packet_migration_v01_v02](packet_migration_v01_v02.md) — Compatibility policy.
- [seq_ref_version_link_metrics_v0](seq_ref_version_link_metrics_v0.md) — Seq/ref update rules; v0.2 deprecates Tail ref.
- [nodetable_master_field_table_v0](nodetable_master_field_table_v0.md) — Product truth (#419); unchanged.
- [packet_to_nodetable_mapping_v0](packet_to_nodetable_mapping_v0.md) — Mapping; v0.2 mapping follows this truth table.
- [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435), #422 (Path B), #417–#421 (P0 chain).
