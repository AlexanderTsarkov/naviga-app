# NodeTable — TX priority & queue arbitration v0.1 (WIP)

**Status:** WIP. S03 packet sets and TX policy. **Parent:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352).

This document defines the **v0.1 intent** for TX priority levels, coalesce keys, and hybrid expired-counter semantics for NodeTable-related beacon packets. It does **not** modify existing v0 canon encoding contracts; it describes policy for queue arbitration and send eligibility.

---

## 1) Priority levels (P0–P3)

| Priority | Intent | Packet types (v0.1) |
|----------|--------|----------------------|
| **P0** | Highest; must be sent first (e.g. Core position, liveness). | Node_Core_Pos, Alive |
| **P1** | High; core-attached or critical. | — |
| **P2** | Medium; optional but valuable (e.g. position quality). | Node_Core_Tail |
| **P3** | Lower; slow/diagnostic, send when budget allows. | Node_Operational, Node_Informative |

Within the same priority, packets are ordered by **expired_counter DESC** (see §3).

---

## 2) Coalesce key

- **Coalesce key** identifies “the same logical update” for replacement and deduplication in the TX queue.
- When a new payload is produced for the same **coalesce_key**, it **replaces** any pending payload with that key (one slot per key per priority band).
- Coalesce key is defined per packet type (e.g. for Node_Operational: snapshot of (battery, radio, uptime) or a type-specific key; for Node_Core_Tail: `ref_core_seq16`).
- Replacement does **not** reset **expired_counter**; the counter is preserved across replacement (§3).

---

## 3) Hybrid expired_counter semantics

- **expired_counter** is per (priority, coalesce_key) or per queue slot.
- **Increments:**
  - **+1 on replacement by same coalesce_key:** When a newer payload replaces a pending one for the same key, increment the counter (the slot was “due” but not yet sent; replacement counts as another cycle where it wasn’t sent).
  - **+1 when due/eligible but not sent (starved):** When the packet is eligible (e.g. min_interval elapsed, changed-gated satisfied) but the scheduler does not send it in that tick (e.g. higher priority used the budget), increment the counter.
- **Reset:** On **send**, reset the counter to 0 for that slot/key.
- **Preserve across replacement:** When a payload is replaced by a newer one with the same coalesce_key, the existing expired_counter is **kept** (not reset); then +1 is applied for the replacement event.

**Intra-priority ordering:** Among packets of the same priority that are eligible to send, order by **expired_counter DESC** so that slots that have been starved longer are sent first.

---

## 4) Relation to v0 canon

- v0 canon ([field_cadence_v0](field_cadence_v0.md), [beacon_payload_encoding_v0](../../../../areas/nodetable/contract/beacon_payload_encoding_v0.md)) defines **what** is sent (Core / Tail-1 / Tail-2 / Informative) and byte layout. This doc defines **when** and **in what order** within a v0.1 TX policy.
- No change to on-air encoding or protocol; this is scheduler/queue policy only.

---

## 5) Related

- [packet_sets_v0_1.md](packet_sets_v0_1.md) — Packet definitions v0.1 and eligibility rules.
- [field_cadence_v0.md](field_cadence_v0.md) — Canon tiers and cadence.
- [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352).
