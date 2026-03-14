# Docs map: canonical vs reference vs working

This document categorizes docs so readers land on canon first. **Post-S03 cleanup:** Product canon lives under `docs/product/areas/`; legacy OOTB-era specs were moved to `docs/product/archive/` (legacy_firmware, legacy_protocols, legacy_product). **PR3** moved working artifacts into `_archive/ootb_v1/`.

## Canonical (entrypoint and product truth)

- **[../START_HERE.md](../START_HERE.md)** — Entrypoint: what Naviga is, what works now, where to look next.
- **[../product/README.md](../product/README.md)** — Product docs entry; canon = `docs/product/areas/`.
- **[../product/current_state.md](../product/current_state.md)** — Current product state and canon pointers.
- **[state_after_ootb_v1.md](state_after_ootb_v1.md)** — Snapshot after OOTB v1 (2026-02-12): scope, invariants, evidence links.
- **This file** — Map of doc categories.

## Reference (link from canon or architecture index)

- [../architecture/index.md](../architecture/index.md) — Architecture index, layer map, source-of-truth table.
- [../adr/](../adr/) — ADRs (position source, radio band, etc.).
- [../firmware/](../firmware/) — Build/BLE test instructions; implementation status. **Product canon** for firmware/radio/NodeTable: [../product/areas/](../product/areas/).
- [../product/archive/](../product/archive/README.md) — Deprecated OOTB-era specs (legacy_firmware, legacy_protocols, legacy_product); historical reference only.

## Archived

- **Legacy product/firmware/protocol specs (post-S03):** [../product/archive/](../product/archive/README.md) — legacy_firmware, legacy_protocols, legacy_product.
- **OOTB v1 working artifacts (PR3):** [ootb_progress_inventory.md](../../_archive/ootb_v1/project/ootb_progress_inventory.md) — Per-issue PR evidence; Mobile v1 tracking.
- [ootb_workmap.md](../../_archive/ootb_v1/project/ootb_workmap.md) — Workmap, status table, phase rules.

See [_archive/ootb_v1/README.md](../../_archive/ootb_v1/README.md) for what was archived and why; canon remains above.

Process artifacts / issue research notes live under [_archive/research/](../../_archive/research/) (issue_###/ and misc/).
