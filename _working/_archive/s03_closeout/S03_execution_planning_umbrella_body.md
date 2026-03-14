# S03 Execution planning: implement canon slice (code+tests plan)

## Objective

Convert post-#412 canon into a concrete implementation plan so that:

- **Code becomes conforming** to canon (NodeTable, packetization, provisioning, persistence).
- Implementation is delivered in **small, sequenced PRs** with clear quality gates.
- Domain stays radio-agnostic; M1Runtime is the wiring point; legacy BLE remains disabled.

This umbrella is **planning only**; actual coding happens in the implementation sub-issues linked below.

---

## Scope buckets

| Bucket | Description | Priority |
|--------|-------------|----------|
| **A — Storage & persistence foundation** | NVS adapter, persisted seq16, NodeTable snapshot format and restore policy. | P0 |
| **B — NodeTable completeness & wiring** | Fields match canon field map/master table; RX semantics (presence/age, core/tail ref) match canon apply rules. | P0 |
| **C — Packetization v0.2 + TX scheduler rules** | Packet formation per canon TX rules (trigger/earliest/deadline/coalesce/preemptible/budget_class); PosFull + Node_Ops_Info (or v0.1 + throttle/merge) per canon. | P0/P1 |
| **D — Provisioning / boot pipeline conformance** | Boot phases A/B/C reflect provisioning baseline; OOTB autonomous start without phone; radio profile baseline (FACTORY/current) aligned. | P1 |
| **E — Observability & sanity** | Debug counters/logs for traffic validation; doc updates to current_state when capabilities land. | P1/P2 |

---

## Non-goals

- **No BLE work** — S04 gate stays gated.
- **No backward-compat theatre** — still no release; do not enforce backward compatibility or protocol version proliferation.
- **No canon semantics changes** — implement canon as specified; do not redefine semantics in code.
- **No refactor+behavior in one PR** — keep PRs small and single-purpose.

---

## Sequencing plan (phases → small PRs)

1. **Phase 1 — Persistence foundation (P0)**  
   NVS adapter + persisted seq16 behavior; NodeTable snapshot format and restore policy. Enables reboot-safe state. Sub-issues: A1, A2.

2. **Phase 2 — NodeTable structure & RX (P0)**  
   NodeTable fields aligned to canon field map; RX apply/coalesce rules match canon (presence/age, core/tail ref). Sub-issues: B1, B2.

3. **Phase 3 — Packetization & TX rules (P0/P1)**  
   Packet formation per canon TX rules table; PosFull + Node_Ops_Info (or v0.1 + throttle/merge) per canon; tests and deterministic checks. Sub-issues: C1, C2.

4. **Phase 4 — Boot & provisioning (P1)**  
   Boot phases A/B/C conformance; OOTB autonomous start; radio profile baseline. Sub-issue: D1.

5. **Phase 5 — Observability & docs (P1/P2)**  
   Debug counters/logs; current_state updates after major capability merges. Sub-issue: E1.

Each sub-issue should be solvable in **1–3 PRs**. No mixing of layers (e.g. domain vs M1Runtime wiring) in a single PR without explicit approval.

---

## Quality gates

- **CI green** on main and on every PR.
- **Unit tests** for new or touched logic (restore invariants, apply/coalesce rules, NVS adapter).
- **Bench checks** where applicable (deterministic simulation or minimal HW bench checklist).
- **Docs/current_state** updated after each major capability merge (per exit criteria).

---

## Exit criteria (umbrella)

- [ ] All implementation sub-issues created and ordered (see list below).
- [ ] P0 sequencing is clear: storage → NodeTable → TX rules.
- [ ] Test strategy defined: unit + integration + minimal bench checklist.
- [ ] Project board updated (umbrella Ready; sub-issues Backlog/Ready as set).
- [ ] #351 points to this execution planning umbrella.
- [ ] Ready to open “S03 Execution” (coding) once planning sub-issues start moving.

---

## Implementation sub-issues (created)

| # | Issue | Title | Bucket | P |
|---|--------|--------|--------|---|
| 1 | [#417](https://github.com/AlexanderTsarkov/naviga-app/issues/417) | S03 P0: Persisted seq16 + NVS adapter (execution; links #296, #355) | A | P0 |
| 2 | [#418](https://github.com/AlexanderTsarkov/naviga-app/issues/418) | S03 P0: NodeTable persistence snapshot format + restore policy | A | P0 |
| 3 | [#419](https://github.com/AlexanderTsarkov/naviga-app/issues/419) | S03 P0: NodeTable fields match canon field map / master table | B | P0 |
| 4 | [#421](https://github.com/AlexanderTsarkov/naviga-app/issues/421) | S03 P0: RX semantics — apply rules match canon (presence/age, core/tail ref) | B | P0 |
| 5 | [#420](https://github.com/AlexanderTsarkov/naviga-app/issues/420) | S03 P0/P1: Packet formation per canon TX rules table | C | P0 |
| 6 | [#422](https://github.com/AlexanderTsarkov/naviga-app/issues/422) | S03 P0/P1: Packetization redesign (PosFull, Node_Ops_Info) or v0.1 + throttle/merge | C | P0/P1 |
| 7 | [#423](https://github.com/AlexanderTsarkov/naviga-app/issues/423) | S03 P1: Boot phases A/B/C + OOTB autonomous start conformance | D | P1 |
| 8 | [#424](https://github.com/AlexanderTsarkov/naviga-app/issues/424) | S03 P1: Radio profile baseline (FACTORY/current) alignment | D | P1 |
| 9 | [#425](https://github.com/AlexanderTsarkov/naviga-app/issues/425) | S03 P1/P2: Observability — debug counters/logs for traffic validation | E | P1/P2 |
| 10 | [#426](https://github.com/AlexanderTsarkov/naviga-app/issues/426) | S03 P2: current_state doc updates when major capabilities land | E | P2 |

---

## Links

- **Canon promotion (completed):** [#412](https://github.com/AlexanderTsarkov/naviga-app/issues/412) — S03 WIP → Canon Promotion.
- **Master planning umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) — S03 Planning (WIP); keep In Review; this execution planning umbrella is the “next phase” after promotion.
- **Key canon (post-#412):** `docs/product/areas/` (spec_map / current_state for navigation); [packet_context_tx_rules_v0](docs/product/areas/radio/policy/packet_context_tx_rules_v0.md), [packet_to_nodetable_mapping_v0](docs/product/areas/nodetable/policy/packet_to_nodetable_mapping_v0.md), [rx_semantics_v0](docs/product/areas/nodetable/policy/rx_semantics_v0.md), [provisioning_baseline_v0](docs/product/areas/firmware/policy/provisioning_baseline_v0.md), [ootb_autonomous_start_v0](docs/product/areas/firmware/policy/ootb_autonomous_start_v0.md).
- **Existing implementation trackers:** [#296](https://github.com/AlexanderTsarkov/naviga-app/issues/296) (persisted seq16/NVS), [#355](https://github.com/AlexanderTsarkov/naviga-app/issues/355) (S03 P0 wrapper for #296) — sub-issue A1 uses them as trackers; do not duplicate.
