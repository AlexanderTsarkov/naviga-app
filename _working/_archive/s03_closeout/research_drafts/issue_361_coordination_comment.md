**Accepted decision (coordination):** #361 is reframed so that the **current** deliverable is an **S04 gate/defer note**, not a detailed BLE snapshot outline.

**Delivered now (this PR):**
- WIP doc **`docs/product/wip/areas/mobile/ble_snapshot_s04_gate.md`** — states that detailed BLE snapshot design is **deferred** until S03 canon and code convergence are complete; lists why we defer, required S03 inputs (accepted NodeTable canon, field map, packet→NodeTable mapping, packet context/TX rules, code parity), S04 entry checklist, and non-goals so the doc is not mistaken for a BLE contract. Cross-links to [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358), [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407), [#230](https://github.com/AlexanderTsarkov/naviga-app/issues/230) and related NodeTable/packet docs.

**Explicitly deferred:**
- **Detailed BLE snapshot design** (scope, fields, format, cadence/trigger, GATT/transport) is a **post-S03 / S04** task. It is not cancelled; it is deferred until the inputs and checklist in the gate doc are satisfied.

**Conditions before starting detailed BLE snapshot design:**
- S03 planning WIP promoted to canon where agreed; NodeTable field map / master table accepted and stable for export subset; packet→NodeTable mapping and packet context/TX rules accepted and reflected in canon/code; code parity (FW and any BLE path aligned with accepted semantics); S04 BLE snapshot design task created and scoped. See the gate doc §5 and §7 for the full list.

This PR **references** #361 but **does not close** it. The issue remains open to track the gate outcome and, when appropriate, the later S04 design work.
