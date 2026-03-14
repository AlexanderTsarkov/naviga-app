# S04: BLE canon promotion and slicing (planning umbrella)

## Objective

S04 is the **post-S03 planning/design iteration**. It is **not** a coding iteration by default. This umbrella frames the S04 phase and makes the **BLE WIP → canon-promotion** path the first explicit workstream.

- **Canon promotion first:** Review the merged BLE WIP against its promotion criteria; decide promotion to canon only when ready.
- **Slicing second:** Implementation tasks are sliced only after the BLE contract is canon-ready (or explicitly deferred).
- **Implementation later:** Firmware/mobile implementation happens in later prompts/issues, not in this umbrella.

---

## Scope

| Workstream | Description | Order |
|------------|-------------|--------|
| **1 — BLE WIP → canon review** | Use [ble_contract_s04_v0.md](docs/product/wip/areas/mobile/ble_contract_s04_v0.md) as input artifact. Verify readiness against promotion criteria (§14 of that doc). Do **not** promote in S04 setup; promotion is a separate decision after review. | First |
| **2 — Canon promotion decision** | If review confirms readiness: promote BLE contract to canon per docs promotion policy. If not: document gaps and defer. | Second |
| **3 — Implementation slicing** | Only after canon readiness (or explicit deferral): slice implementation tasks for BLE/firmware/mobile. | Third |
| **4 — Implementation** | Carried out in later issues/PRs; not part of this planning umbrella. | Later |

---

## Input artifact (WIP, not promoted yet)

- **Doc:** [docs/product/wip/areas/mobile/ble_contract_s04_v0.md](docs/product/wip/areas/mobile/ble_contract_s04_v0.md)
- **Track:** [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361)
- **Status:** WIP / Candidate. Promotion to canon only after review and explicit decision.

---

## Non-scope (S04 setup and this umbrella)

- No final byte-level payload or GATT/UUID work in S04 setup
- No firmware/mobile implementation in this phase
- No JOIN/Mesh expansion
- No reopening of S03 issues/umbrellas
- Legacy BLE remains non-authoritative; not reintroduced unless explicitly planned

---

## Definition of done (S04 setup phase)

- [ ] S04 iteration artifact exists (_working/ITERATION.md)
- [ ] S04 umbrella (this issue) exists and is linked
- [ ] BLE canon-promotion workstream is explicitly framed as first workstream
- [ ] Next execution prompts can proceed without ambiguity

---

## References

- **Iteration artifact:** `_working/ITERATION.md` (first line: S04__2026-03__BLE_Promotion_and_Slicing.v1)
- **BLE gate (S03):** [ble_snapshot_s04_gate.md](docs/product/wip/areas/mobile/ble_snapshot_s04_gate.md) — S04 entry checklist
- **Constraints:** M1Runtime = wiring point; domain radio-agnostic; OOTB/UI not normative (see [ai_model_policy](docs/dev/ai_model_policy.md))
