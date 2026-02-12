# Docs map: canonical vs reference vs working

This document categorizes docs so readers land on canon first. **PR2 (Issue #119) does not move, rename, archive, or delete any files.** PR3 will archive working artifacts into `_archive/ootb_v1/`.

## Canonical (entrypoint and snapshot)

- **[../START_HERE.md](../START_HERE.md)** — Entrypoint: what Naviga is, what works now, where to look next.
- **[state_after_ootb_v1.md](state_after_ootb_v1.md)** — Snapshot after OOTB v1 (2026-02-12): scope, invariants, evidence links.
- **This file** — Map of doc categories.

## Reference (stable specs; link from canon or architecture index)

- [../architecture/index.md](../architecture/index.md) — Architecture index, layer map, source-of-truth table.
- [../adr/](../adr/) — ADRs (position source, radio band, etc.).
- [../protocols/](../protocols/) — OOTB Radio v0, BLE v0, presets.
- [../firmware/](../firmware/) — HAL contracts, NodeTable spec, GNSS, firmware arch.
- [../product/](../product/) — Product core, scope, test plan, OOTB analysis (where still accurate).

## Working (trail / planning; to be archived in PR3)

- [ootb_progress_inventory.md](ootb_progress_inventory.md) — Per-issue PR evidence; Mobile v1 tracking.
- [ootb_workmap.md](ootb_workmap.md) — Workmap, status table, phase rules.
- Other `docs/dev/`, `docs/product/` planning and working docs as identified in PR3.

**Archive plan:** PR3 will move working artifacts into `_archive/ootb_v1/` and update links as needed. No file moves in PR2.
