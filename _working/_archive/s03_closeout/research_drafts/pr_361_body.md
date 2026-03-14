## Summary

Add a **docs-only** gate/deferral note for the BLE snapshot (S04). This PR **references [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361)** but **does not close it**.

**Accepted decision:** The current #361 deliverable is reframed from “draft the BLE snapshot outline now” into an **S04 gate/defer note**. We do **not** produce a detailed BLE snapshot spec in S03; we record what must be true before S04 designs it.

## What this PR adds

- **New doc:** `docs/product/wip/areas/mobile/ble_snapshot_s04_gate.md`
  - States that **detailed BLE snapshot design is deferred** until S03 canon and code convergence are complete.
  - Lists required S03 inputs (accepted NodeTable canon, field map, packet→NodeTable mapping, packet context/TX rules, code parity).
  - Provides an S04 entry checklist and open questions; does **not** define BLE fields, format, cadence, GATT, or transport.
  - Cross-links to [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358), [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407), [#230](https://github.com/AlexanderTsarkov/naviga-app/issues/230), NodeTable hub, and packet context/TX rules.

**Detailed BLE snapshot design** (scope, fields, format, cadence/trigger) remains a **post-S03 / S04** task, to be started when the conditions in the gate doc are satisfied. This PR does not close #361; the issue stays open until that later design work is scoped and delivered (or the issue is updated to reflect the gate outcome as the S03 completion of this planning slice).

## Scope

- Docs-only; no code changes.
- No expansion of the gate doc into an actual BLE snapshot spec.
