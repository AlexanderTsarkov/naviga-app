# NodeTable — Packet migration v0.1 → v0.2 (compatibility policy)

**Status:** Canon (policy).  
**Issue:** [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435).  
**Companion:** [packet_truth_table_v02](packet_truth_table_v02.md) — canonical v0.2 packet family and semantics.

This memo states the **explicit compatibility policy** for the transition from v0.1 to v0.2 packet family. Implementation must follow this policy; removal of v0.1 acceptance is a later cleanup PR.

---

## 1) What we send (TX)

- **After v0.2 implementation is merged:** TX sends **v0.2 packets only** (Node_Pos_Full, Node_Status, Alive).
- **We do not send** Core_Pos, Core_Tail, Node_Operational (0x04), or Node_Informative (0x05) as the canonical path. Dual path (sending both v0.1 and v0.2) is **not** in scope — it would double airtime and complexity; v0.2 is the target truth.

---

## 2) What we accept (RX) during transition

- **Transition phase:** RX **accepts** both:
  - **v0.2:** Node_Pos_Full, Node_Status, Alive.
  - **v0.1:** Core_Pos, Core_Tail, Node_Operational, Node_Informative.
- All accepted packets apply to the **same** NodeTable normalized fields. Compatibility logic (recognizing v0.1 `msg_type`s and v0.2 `msg_type`s or payloadVersion) lives in the RX decode/dispatch layer, **clearly scoped** (e.g. one compatibility module or flag) and documented as temporary.

---

## 3) v0.1 is compatibility-only

- v0.1 packet types are **not** canon during and after transition. They are accepted only for **backward compatibility** so that new firmware can still read old nodes (and vice versa during rollout).
- Canonical packet family is v0.2 only; see [packet_truth_table_v02](packet_truth_table_v02.md).

---

## 4) Cutover expectation

- **Duration / criterion:** Transition lasts until **fleet or config cutover** or **N releases** (to be decided at cutover time).
- **Post-cutover:** RX may **accept v0.2 only**; v0.1 packets optionally **log and drop**. A version flag or config can turn off “v0.1 accept” when cutover is decided.
- **Cleanup PR (later):** Remove v0.1 accept path and last_applied_tail_ref_core_seq16 from product surface; update seq/ref policy docs to v0.2-only.

---

## 5) Summary table

| Phase | TX (what we send) | RX (what we accept) |
|-------|-------------------|----------------------|
| **Transition** | v0.2 only (Node_Pos_Full, Node_Status, Alive) | v0.2 **and** v0.1 (Core_Pos, Core_Tail, Operational, Informative) |
| **Post-cutover** | v0.2 only | v0.2 only; v0.1 optionally log & drop |

---

## 6) Related

- [packet_truth_table_v02](packet_truth_table_v02.md) — v0.2 canon.
- [packet_context_tx_rules_v0](../../radio/policy/packet_context_tx_rules_v0.md) — TX rules.
- [seq_ref_version_link_metrics_v0](seq_ref_version_link_metrics_v0.md) — Seq/ref; Tail ref deprecated in v0.2.
- [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435).
