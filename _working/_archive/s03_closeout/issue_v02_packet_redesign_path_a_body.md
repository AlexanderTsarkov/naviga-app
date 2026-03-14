# Post-P0: v0.2 packet redesign (Path A) — tracked phase

**Context:** This is a **follow-up** after #422 was completed via **Path B** (throttle/merge, v0.1 lock). It is **not** a continuation of #422. Path A (canonical v0.2 packet family) is tracked here as a separate phase.

**Prior:** P0 chain #417–#422 complete; #407 packetization planning; design note `_working/v02_packet_redesign_design_note.md`.

---

## Goal

Move to the **canonical v0.2 packet family** for the beacon/OOTB path:

- **Node_Pos_Full** — position + quality in one packet; one seq16 per position update (replaces Core_Pos + Core_Tail single-packet path).
- **Node_Status** — full snapshot of operational + informative status (replaces Node_Operational 0x04 + Node_Informative 0x05).
- **Alive** — no-fix liveness; identity + seq16 (retained if kept in v0.2 registry).

---

## Staged migration policy

| Phase | TX (what we send) | RX (what we accept) |
|-------|-------------------|---------------------|
| **Transition** | v0.2 packets only (Node_Pos_Full, Node_Status, Alive) | v0.2 **and** v0.1 (Core_Pos, Core_Tail, Operational, Informative) |
| **Post-cutover** | v0.2 only | v0.2 only; v0.1 optionally log & drop |

- **TX:** Send v0.2 only once implementation is in place.
- **RX:** Accept v0.1 + v0.2 during transition; later cutover to v0.2-only RX (explicit cutover criterion or version flag).
- **No dual path:** Sending both v0.1 and v0.2 is not in scope (would double airtime).

---

## Constraints (preserved)

- **NodeTable** remains normalized product truth; no packed radio blobs in product fields.
- **Active-values only** in payloads and NodeTable.
- **hw_profile_id / fw_version_id** stay **uint16** unless a documented protocol decision changes them.
- **uptime_10m** naming preserved at canon/product layer (wire may use `uptime10m`).
- **M1Runtime** single composition point; domain stays **radio-agnostic**.

---

## Definition of done

- [ ] **v0.2 truth table** — packet set, field composition, sizes; doc or contract.
- [ ] **Explicit compatibility policy** — what is sent (v0.2 only), what is accepted during transition (v0.1 + v0.2), cutover criteria; short compatibility memo.
- [ ] **TX formation** — Node_Pos_Full, Node_Status, Alive per v0.2 table; Node_Status lifecycle (bootstrap, triggers, min_status_interval_ms, T_status_max, no hitchhiking).
- [ ] **RX decode/apply** — dispatch v0.2 msg_type/payloadVersion; apply to NodeTable; compatibility layer for v0.1 during transition; single-packet apply for Node_Pos_Full and Node_Status.
- [ ] **Tests** — unit and/or integration for encode/decode, apply rules, lifecycle, size budgets.
- [ ] **Devkit build** passes.
- [ ] **Docs/inventory** updated (packet sets, encoding contract, compatibility memo, seq_ref deprecation where applicable).

---

## Scope boundaries

- **In scope:** Design → v0.2 truth table + encoding contract; implementation of TX/RX, compatibility layer, tests, docs.
- **Out of scope for this issue:** Reopening #417–#422; changing P0 deliverables; implementation work that belongs in follow-up PRs (this issue tracks the phase; PRs implement it).
- **Design source:** `_working/v02_packet_redesign_design_note.md` (and linked product/radio policy docs).

---

## Related

- #422 (Path B — closed); #407 (packetization); #417–#421 (P0 chain).
- Policy: `packet_context_tx_rules_v0`, `packet_sets_v0`, `packet_to_nodetable_mapping_v0`, `seq_ref_version_link_metrics_v0`.
