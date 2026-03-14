# Canon archive

**Purpose:** Store deprecated or previous versions of **canon** docs (policy, spec, contract) for historical reference. Tracked in GitHub; linkable from canon and WIP only as "historical reference" (explicitly labeled).

## What belongs here

- **YES:** Old canon versions superseded by a newer promoted doc (e.g. `field_cadence_v0` replaced by a revised version).
- **YES:** Superseded policies/specs and short migration notes (what changed, where the new canon lives).
- **YES:** Versioned or date-stamped copies when a canon doc is replaced in place (e.g. `nodetable/policy/foo_v0_2025-02.md`).

## What does NOT belong here

- **NO:** Bench logs, HW transcripts, scratch notes, working reports — those go to **`/archive/**`** (sandbox/evidence) or `_working/`.
- **NO:** Current canon or WIP content — those stay under `docs/product/areas/` and `docs/product/wip/`.

## Deprecation workflow

1. **Add banner** to the old canon doc: "Deprecated"; link to the replacement (new canon path).
2. **Move** the old doc into `docs/product/archive/<area>/<doc>_vX_Y.md` (or date-stamped name). Keep naming predictable so links can be updated.
3. **Update inbound links:** Canon and WIP should point to the **new** doc. Links to the archived doc only as "historical reference" (see below).

## Linking rules

- **Canon and WIP** may link to docs under `docs/product/archive/` only when the intent is **historical reference** (e.g. "Previous version: [archived doc](archive/nodetable/policy/foo_v0_2025-02.md)."). Label such links explicitly so they are not treated as normative.
- **Normative** references must point to the current canon path under `docs/product/areas/`, not to the archive.

## Subsections (post-S03 cleanup, 2026-03)

| Subfolder | Contents |
|-----------|----------|
| **legacy_firmware/** | OOTB-era firmware specs moved from `docs/firmware/`: NodeTable, HAL, GNSS, arch, geo_utils, ublox driver plan. Canon for firmware/boot/NodeTable is in `docs/product/areas/`. |
| **legacy_protocols/** | OOTB-era protocol specs moved from `docs/protocols/`: ootb_radio_v0, ootb_ble_v0, radio_profile_presets_v0. Canon for packets/TX/radio is in `docs/product/areas/nodetable/` and `docs/product/areas/radio/`. |
| **legacy_product/** | OOTB planning/scope docs moved from `docs/product/` root: OOTB_v0_analysis_and_plan, ootb_scope_v0, ootb_radio_preset_v0, ootb_test_plan_v0, ootb_gap_analysis_v0, radio_channel_mapping_v0. |

---

## Related

- Layout and archive split: [docs_layout_policy_v0.md](../policy/docs_layout_policy_v0.md) (§ Archive locations).
- Sandbox/evidence archive: repo root **`/archive/**`** (may be gitignored; not canon/wip). Iteration snapshots: **`archive/iterations/`** (canonical); **`_archive/iterations`** is deprecated.
