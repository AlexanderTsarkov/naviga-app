# NodeTable — Packet migration v0.1 → v0.2 (compatibility policy)

**Status:** Canon (policy).  
**Issue:** [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435).  
**Companion:** [packet_truth_table_v02](packet_truth_table_v02.md) — canonical v0.2 packet family and semantics.

This memo states the **explicit compatibility policy** for the transition from v0.1 to v0.2 packet family. **Cutover complete (#438):** RX is v0.2-only; v0.1 packets are dropped.

---

## 1) What we send (TX)

- **After v0.2 implementation is merged:** TX sends **v0.2 packets only** (Node_Pos_Full, Node_Status, Alive).
- **We do not send** Core_Pos, Core_Tail, Node_Operational (0x04), or Node_Informative (0x05) as the canonical path. Dual path (sending both v0.1 and v0.2) is **not** in scope — it would double airtime and complexity; v0.2 is the target truth.

---

## 2) What we accept (RX) — post-cutover (#438)

- **RX accepts v0.2 only:** Node_Pos_Full (0x06), Node_Status (0x07), Alive (0x02).
- **v0.1 packets (0x01, 0x03, 0x04, 0x05) are rejected** at decode_header; frames are dropped. No compatibility path remains.

---

## 3) v0.1 is obsolete on RX

- v0.1 packet types are **not** accepted. Canonical packet family is v0.2 only; see [packet_truth_table_v02](packet_truth_table_v02.md).

---

## 4) Cutover complete

- **#438:** v0.1 accept path removed; decode_header and BeaconLogic on_rx accept only 0x02, 0x06, 0x07. last_applied_tail_ref_core_seq16 removed from NodeTable.

---

## 5) Summary table

| Phase | TX (what we send) | RX (what we accept) |
|-------|-------------------|----------------------|
| **Post-cutover** | v0.2 only (Node_Pos_Full, Node_Status, Alive) | v0.2 only; v0.1 dropped |

---

## 6) Related

- [packet_truth_table_v02](packet_truth_table_v02.md) — v0.2 canon.
- [packet_context_tx_rules_v0](../../radio/policy/packet_context_tx_rules_v0.md) — TX rules.
- [seq_ref_version_link_metrics_v0](seq_ref_version_link_metrics_v0.md) — Seq/ref; Tail ref deprecated in v0.2.
- [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435).
