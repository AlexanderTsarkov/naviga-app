## Goal

Deliver the **S04 BLE snapshot gate doc** (Variant A): a WIP note that defers detailed BLE snapshot design until S03 canon and code convergence are complete, and that lists required S03 inputs and the S04 entry checklist. No BLE implementation or BLE spec in S03.

## Outputs

- **Doc:** `docs/product/wip/areas/mobile/ble_snapshot_s04_gate.md` — gate/deferral note: why we defer, required S03 inputs (accepted NodeTable canon, field map, packet→NodeTable mapping, packet context/TX rules, code parity), S04 entry checklist, non-goals (no BLE fields/format/GATT/cadence defined here). Full BLE snapshot outline is S04 after WIP→Canon→Code→Test.

## Definition of Done

- [x] Gate doc exists at the path above.
- [x] Prerequisites (required S03 inputs) are listed in the doc.
- [x] S04 entry checklist is included.
- [x] Links to [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) and key docs (NodeTable hub, packet mapping, packet context/TX rules, [#230](https://github.com/AlexanderTsarkov/naviga-app/issues/230) audit) are present.
- [x] Doc explicitly states it is not a BLE spec (no fields/format/GATT/cadence).

## Links

- **Parent:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) (S03 Planning (WIP): NodeTable as Product Heart)
- **Gate doc:** [ble_snapshot_s04_gate.md](docs/product/wip/areas/mobile/ble_snapshot_s04_gate.md)
- [NodeTable hub](docs/product/areas/nodetable/index.md)
- [s02_230_mobile_nodetable_integration_audit](docs/product/areas/mobile/audit/s02_230_mobile_nodetable_integration_audit.md) — [#230](https://github.com/AlexanderTsarkov/naviga-app/issues/230)
