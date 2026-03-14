S04__2026-03__BLE_Promotion_and_Slicing.v1

Work Area: Planning / Design / Canon promotion prep (not Implementation by default)
Tech Area: BLE, Mobile contract, NodeTable export/read/update, Profiles/config, Discovery/versioning

Scope:
- Use merged BLE WIP as the source artifact for review: docs/product/wip/areas/mobile/ble_contract_s04_v0.md
- Verify readiness against promotion criteria stated in that doc
- Prepare canon promotion decision (do not promote in this setup)
- Prepare implementation slicing only after canon readiness is confirmed

Explicit non-scope:
- No final byte-level payload work in this setup
- No final GATT/UUID work in this setup
- No firmware/mobile implementation in this setup
- No JOIN/Mesh expansion
- Legacy BLE remains non-authoritative; not reintroduced unless explicitly planned

Links:
- Issue/Umbrella: #460
- BLE WIP (input artifact, not promoted yet): docs/product/wip/areas/mobile/ble_contract_s04_v0.md — #361

Order (strict):
1. BLE WIP integrity / canon-promotion review first
2. Canon-promotion decision second
3. Slicing of implementation tasks third (only after canon readiness)
4. Implementation work in later prompts/issues

Definition of Done (S04 setup phase):
- Active S04 iteration artifact exists (this file)
- S04 umbrella/planning issue exists or is updated
- BLE canon-promotion workstream is explicitly framed as first workstream
- Next execution prompts can proceed without ambiguity

Notes:
- S04 is not a coding iteration by default; it starts with design/promotion/slicing preparation.
- M1Runtime is the wiring/composition point; domain stays radio-agnostic.
- Product specs stay in docs/product/** and docs/product/wip/**; do not treat OOTB/UI as normative source.
