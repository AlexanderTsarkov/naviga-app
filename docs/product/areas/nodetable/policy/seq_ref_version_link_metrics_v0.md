# NodeTable — Seq / ref / payload version / link metrics (S03 block v0)

**Status:** Canon (policy).  
**Parent:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352).

This document defines the **S03** semantics for per-node sequence and dedupe, Tail–Core reference, payload-version handling (avoid "trash field"), and receiver-injected link metrics. No firmware code or protocol layout changes; documentation only.

---

## 1) Definitions (sequence and ref)

- **last_seq16 (cross-type):** The last **seq16** value seen from this node on **any** accepted RX. Stored in NodeEntry as `last_seq`. Used for dedupe (nodeId48, seq16). **Update rule:** updated on **any** RX from that node (all packet types).
- **last_core_seq16:** The last **seq16** value from a **Core_Pos** packet from this node. Stored for Tail linkage. **Update rule:** updated **only on Core_Pos RX** (not on Alive/Tail/Operational/Informative).
- **ref_core_seq16:** The **Tail-only** reference field: in Node_Core_Tail payload, the 2-byte back-reference to the Core_Pos seq16 that this Tail extends. **Update rule:** sent by transmitter in Tail; receiver uses it to match/apply Tail to the correct Core; receiver's **last_core_seq16** is updated only when applying Core_Pos.
- **last_applied_tail_ref_core_seq16:** Receiver-injected; used to dedupe so at most one Tail is applied per Core sample. **Update rule:** updated **only on Tail RX** when a Tail is accepted (ref_core_seq16 match); prevents applying a second Tail for the same ref_core_seq16.

---

## 2) Link metrics (receiver-injected)

- **last_rx_rssi:** Receiver-injected; RSSI from the last accepted RX from this node. **Update rule:** updated on **any** RX from that node. **Units:** raw or dBm (TBD; document when fixed). **S03:** required; parse E22 RSSI append and store in NodeTable; export in BLE snapshot.
- **snrLast:** Receiver-injected; SNR from the last accepted RX. **Update rule:** updated on any RX when SNR is available. **E22:** UNSUPPORTED → store **NA/sentinel** (e.g. int8 with 127 or -128 = NA/UNSUPPORTED). **S03:** required field; plumb through NodeTable and snapshots with sentinel when unsupported.

**Sentinel rule for SNR:** Recommend **int8** with **127** (or -128) = NA/UNSUPPORTED so decoders and UI can distinguish "no value" from valid SNR.

---

## 3) payloadVersion handling

**Conflict note:** Avoid treating payload version as a "trash field" in NodeTable; either treat as last-seen (injected/debug) or remove from stored model if unused.

**Decision (Option 1 — recommended):** Keep a **receiver-injected** notion for decoder/debug use only:

- **last_payload_version_seen:** Injected; value of the **payloadVersion** (first byte) from the last accepted packet from this node. **Purpose:** decoder aid / debug; **not** user-facing; **not** persisted in NVS. Use for consistency checks or diagnostics only. If not used in implementation, can be omitted from NodeEntry; document as "debug only" in master table.

**On-air:** The first byte of every Node_* payload remains **payloadVersion** (e.g. 0x00 = v0). NodeTable/store may optionally keep **last_payload_version_seen** as the last seen value (injected, debug only).

---

## 4) Update rules (summary)

**v0.1 (current):**

| What | When it updates |
|------|------------------|
| last_seq16 (last_seq) | Any RX from that node (dedupe key) |
| last_core_seq16 | Core_Pos RX only |
| last_applied_tail_ref_core_seq16 | Tail RX only (when Tail applied) |
| last_rx_rssi | Any RX from that node |
| snrLast | Any RX (when available); E22 → NA sentinel |
| last_payload_version_seen | Any RX (optional; debug only) |

**v0.2 (canonical, [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435)):** last_core_seq16 updates **only on Node_Pos_Full RX** (or, during transition, on Core_Pos). **last_applied_tail_ref_core_seq16** is **deprecated** for v0.2 — no Tail packet; removal from product surface in a later compatibility-cleanup PR. See [packet_truth_table_v02](packet_truth_table_v02.md) §4, [packet_migration_v01_v02](packet_migration_v01_v02.md).

---

## 5) Related

- [presence_and_age_semantics_v0.md](presence_and_age_semantics_v0.md) — last_seen_ms, is_stale, presence.
- [packet_sets_v0.md](packet_sets_v0.md), [tx_priority_and_arbitration_v0.md](tx_priority_and_arbitration_v0.md) — Packets and TX.
- [packet_truth_table_v02.md](packet_truth_table_v02.md), [packet_migration_v01_v02.md](packet_migration_v01_v02.md) — v0.2 seq/ref rules and Tail deprecation (#435).
- [link_metrics_v0.md](link_metrics_v0.md) (canon) — rssiLast/snrLast policy.
- Master table: **seq16** (last_seq), **ref_core_seq16**, **last_applied_tail_ref_core_seq16** (v0.1 / compat only; deprecated in v0.2), **last_rx_rssi**, **snrLast**, **last_payload_version_seen**.
