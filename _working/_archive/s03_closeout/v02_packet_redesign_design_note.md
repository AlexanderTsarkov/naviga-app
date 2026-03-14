# v0.2 packet redesign — design note (pre-implementation)

**Status:** Design (required before coding).  
**Context:** Post-P0 Path A / v0.2 packet redesign as a **separate tracked issue** [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435) after #422 (closed via Path B).  
**Rule:** No coding before this note is written down and agreed.

**Sources:** [packet_context_tx_rules_v0](../docs/product/areas/radio/policy/packet_context_tx_rules_v0.md) §2/§2a, [packet_sets_v0](../docs/product/areas/nodetable/policy/packet_sets_v0.md), [beacon_payload_encoding_v0](../docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md), [gnss_tail_completeness_and_budgets_s03](../docs/product/wip/areas/nodetable/contract/gnss_tail_completeness_and_budgets_s03.md), [traffic_model_v0](../docs/product/areas/radio/policy/traffic_model_v0.md), product truth #419 ([product_truth_s03_v1](../docs/product/wip/areas/nodetable/product_truth_s03_v1.md)).

---

## 1) Exact v0.2 packet set

| Packet           | Role                                      | Replaces (v0.1)                    |
|------------------|-------------------------------------------|------------------------------------|
| **Node_Pos_Full** | Position + quality in one packet; one seq16 per position update | Core_Pos + Core_Tail (single packet path) |
| **Node_Status**   | Full snapshot of operational + informative status | Node_Operational (0x04) + Node_Informative (0x05) |
| **Alive**        | No-fix liveness; identity + seq16        | Unchanged (retained)               |

- **Canonical v0.2 family:** exactly these three packet types for the beacon/OOTB path.
- **Wire:** New `msg_type` values (or payloadVersion-based dispatch) for Node_Pos_Full and Node_Status; Alive can keep existing `msg_type` or be explicitly part of v0.2 registry. Exact registry is a contract follow-up; this note fixes the **set** and **roles**.

---

## 2) Field composition per packet

### 2.1 Node_Pos_Full

- **Common prefix:** 9 B (payloadVersion, nodeId48, seq16) — unchanged.
- **Useful payload:**
  - **Position (same as current Core):** lat_u24 (3 B), lon_u24 (3 B).
  - **Pos_Quality:** 2 B (single uint16 LE). Bit layout per [gnss_tail_completeness_and_budgets_s03](../docs/product/wip/areas/nodetable/contract/gnss_tail_completeness_and_budgets_s03.md): fix_type (3 b), pos_sats (6 b), pos_accuracy_bucket (3 b), pos_flags_small (4 b). Exact bit positions and ordering to be fixed in the v0.2 encoding contract.
- **Total payload:** 9 + 6 + 2 = **17 B**. On-air: 2 (header) + 17 = **19 B**.
- **No** `ref_core_seq16`; no second packet. One seq16 per position update.

### 2.2 Node_Status

- **Common prefix:** 9 B (payloadVersion, nodeId48, seq16).
- **Useful payload (full snapshot):** All of the following in one packet; order and optionality/sentinels TBD in encoding contract.
  - **Operational:** batteryPercent (1 B), battery_Est_Rem_Time (encoding TBD, e.g. 1–2 B), TX_power_Ch_throttle (e.g. 1 B: 4+4 bits or equivalent), uptime10m (1 B, 10-minute units).
  - **Informative:** role_id (1 B or as defined), maxSilence10s (1 B), hwProfileId (2 B, uint16 LE), fwVersionId (2 B, uint16 LE).
- **Product/canon names:** `uptime_10m` at product layer; wire may use `uptime10m`. `hw_profile_id` / `fw_version_id` remain **uint16** unless a documented protocol decision changes them.
- **Total payload (estimate):** 9 + 1 + (1–2) + 1 + 1 + 1 + 1 + 2 + 2 = **19–20 B** useful; exact size in encoding contract. On-air with header: **21–22 B**. Must fit Default budget (32 B payload).

### 2.3 Alive

- Unchanged from current v0: Common 9 B; optional aliveStatus 1 B. Min 9 B payload, 11 B on-air; with aliveStatus 10 B payload, 12 B on-air.

---

## 3) Compatibility strategy

**Chosen strategy: staged migration with explicit compatibility window.**

| Phase        | TX (what we send)     | RX (what we accept)                    | Duration / criterion   |
|-------------|------------------------|----------------------------------------|------------------------|
| **Transition** | v0.2 packets only (Node_Pos_Full, Node_Status, Alive) | v0.2 packets **and** v0.1 (Core_Pos, Core_Tail, Operational, Informative) | Until fleet/config cutover or N releases |
| **Post-cutover** | v0.2 only             | v0.2 only; v0.1 optionally log & drop   | After explicit cutover |

- **Dual path (sending both v0.1 and v0.2) not chosen:** Would double airtime and complexity; v0.2 is the target truth.
- **Accept-old-only (receive v0.1, send v0.2):** Possible as a sub-phase during transition so new FW still reads old nodes; send path is v0.2-only.
- **Where compatibility lives:** RX decode/dispatch layer: recognize both v0.1 `msg_type`s and v0.2 `msg_type`s (or payloadVersion); apply to same NodeTable normalized fields. Compatibility logic is **clearly scoped** (e.g. one compatibility module or flag) and documented as temporary; removal is a later cleanup PR.

**Explicit policy doc required:** Short compatibility memo stating (1) what is sent (v0.2 only after implementation), (2) what is accepted during transition (v0.1 + v0.2), (3) that v0.1 is compatibility-only and not canon, (4) cutover criteria or version flag for “v0.1 accept” off.

---

## 4) RX / apply consequences

- **Node_Pos_Full:** Single-packet apply. On accept: update node_id, seq16 (last_seq), last_core_seq16 := seq16 (this packet is the position sample), position (lat/lon), and Pos_Quality fields (fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small) into NodeTable. **Obsolete:** ref_core_seq16, last_applied_tail_ref_core_seq16 for the position path; no Tail to match.
- **Node_Status:** On accept: update node_id, seq16, and full status snapshot (battery, uptime_10m, tx_power/channel_throttle, role_id, max_silence_10s, hw_profile_id, fw_version_id, battery_est_rem_time if present) into NodeTable. Single apply; no merge of Op vs Info from two packet types.
- **Alive:** Unchanged: update node_id, seq16, last_seen_ms; do not update position or status.
- **Presence / self:** Self last_seen_ms updated on TX of Node_Pos_Full, Alive (and optionally no longer on a separate “Tail” send). Node_Status remains non–presence-bearing (do not update self last_seen_ms on Node_Status TX). Same as current policy: only position-bearing and alive-bearing packets update self presence.
- **Seq/ref cleanup:** last_core_seq16 is updated only on Node_Pos_Full (or, during compat, on Core_Pos). last_applied_tail_ref_core_seq16 is **removed** from product NodeTable when v0.2 is canonical (or kept only in compat path for v0.1 Tail apply). Product truth and [seq_ref_version_link_metrics_v0](../docs/product/areas/nodetable/policy/seq_ref_version_link_metrics_v0.md) to be updated to state v0.2 rules and deprecate Tail ref state.

---

## 5) Packet-size / airtime budget implications

| Packet           | Payload (B) | On-air (B) | LongDist (24) | Default (32) | Fast (40) |
|------------------|------------|------------|---------------|--------------|-----------|
| Node_Pos_Full    | 17         | 19         | ✓             | ✓            | ✓         |
| Node_Status      | ~19–20     | ~21–22     | ✓             | ✓            | ✓         |
| Alive            | 9–10       | 11–12      | ✓             | ✓            | ✓         |

- **Node_Pos_Full vs Core+Tail:** One packet 19 B vs two packets (17 + 15) = 32 B. Saves one packet per position update and removes Tail loss/ordering issues; airtime per position update **reduced** (traffic_model_v0 §4: merged option ~218 ms vs split 198+198 ms).
- **Node_Status:** Single packet replaces Op + Info; similar or slightly lower total than two separate packets; one slot, one snapshot, simpler lifecycle.
- **Checks required:** (1) Encoding contract must fix Node_Status payload size and confirm ≤ 32 B payload for Default. (2) Add unit or integration checks that payload sizes do not exceed profile budgets (LongDist 24, Default 32, Fast 40). (3) Document in a short “v0.2 size budget” memo or appendix; optional: add runtime assertion in encode path.

---

## 6) Node_Status lifecycle (recap)

Per [packet_context_tx_rules_v0](../docs/product/areas/radio/policy/packet_context_tx_rules_v0.md) §2a — unchanged in v0.2:

- **Bootstrap:** status_bootstrap_pending; up to 2 sends; second not earlier than min_status_interval_ms after first.
- **Triggers:** Urgent (TX_power_Ch_throttle, maxSilence10s, role_id); threshold (batteryPercent, battery_Est_Rem_Time); hitchhiker-only (uptime10m, hwProfileId, fwVersionId) included when Status is sent for other reasons.
- **Timing:** min_status_interval_ms = 30 s; T_status_max = 300 s; periodic refresh when no Status sent within T_status_max.
- **No hitchhiking:** Node_Status not enqueued in same formation pass as Node_Pos_Full; standalone only.

---

## 7) Constraints (from issue and product truth)

- **Product truth [#419]:** NodeTable stores normalized product fields only; no packed radio blobs; service-only radio fields (e.g. payloadVersion) not product NodeTable truth; runtime-only state stays runtime-only.
- **Active-values plane:** Payloads and NodeTable use **active values only**; no profile refs inside the product field set.
- **Widths:** hw_profile_id and fw_version_id remain **uint16** unless a documented protocol decision says otherwise.
- **Naming:** uptime_10m and normalized field names at product/canon layer.
- **M1Runtime:** Single composition point; domain stays radio-agnostic.

---

## 8) Definition of done (from issue) — design coverage

- [ ] v0.2 packet truth table written → **this note §1–§2**; full truth table in separate doc or contract.
- [ ] Canonical packet family chosen and documented → **§1**.
- [ ] Migration / compatibility policy documented → **§3**; explicit compatibility memo to be added in implementation.
- [ ] TX formation for v0.2 → implementation PR.
- [ ] RX decode / dispatch / apply for v0.2 → implementation PR.
- [ ] Old packet family retired or compatibility-only → **§3** (compatibility-only, then optional drop).
- [ ] Node_Status lifecycle implemented and tested → **§6**; implementation PR.
- [ ] Node_Pos_Full payload contract implemented and tested → **§2.1**; encoding contract + tests.
- [ ] Width/packing checks for hw_profile_id / fw_version_id → implementation; no width change.
- [ ] Packet-size / airtime-budget checks or justification → **§5**; implementation adds checks/memo.
- [ ] Unit tests updated/added → implementation PR.
- [ ] Devkit build passes → implementation PR.
- [ ] Docs/inventory updated → implementation + compatibility cleanup PR.

---

## 9) Suggested PR split

1. **Design / truth-table PR:** This design note + v0.2 packet truth table (and, if ready, encoding contract stubs or links). No firmware changes.
2. **Implementation PR(s):** TX formation (Node_Pos_Full, Node_Status, Alive), encode/decode, RX dispatch/apply, Node_Status lifecycle, compatibility layer (accept v0.1 during transition), tests, docs updates.
3. **Compatibility cleanup PR (optional, later):** Remove v0.1 accept path and last_applied_tail_ref_core_seq16 from product surface once cutover is decided.

---

## 10) Related

- [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435) (this phase); #422 (Path B closure); #407 (packetization); #417–#421 (P0 chain).
- [packet_context_tx_rules_v0](../docs/product/areas/radio/policy/packet_context_tx_rules_v0.md), [packet_sets_v0](../docs/product/areas/nodetable/policy/packet_sets_v0.md), [tx_priority_and_arbitration_v0](../docs/product/areas/nodetable/policy/tx_priority_and_arbitration_v0.md), [seq_ref_version_link_metrics_v0](../docs/product/areas/nodetable/policy/seq_ref_version_link_metrics_v0.md), [packet_to_nodetable_mapping_v0](../docs/product/areas/nodetable/policy/packet_to_nodetable_mapping_v0.md).
