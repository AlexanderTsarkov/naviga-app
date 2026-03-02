# PR-2 Legacy/OOTB docs inventory (#278)

**Branch:** `docs/278-pr2-legacy-ootb-banners`  
**Scope:** Add header banners + Canon link per [docs_coexistence_policy_v0.md](../../docs/product/policy/docs_coexistence_policy_v0.md). No moves/renames.

---

## Inventory

| path | class | canon_link | action |
|------|-------|------------|--------|
| docs/protocols/ootb_radio_v0.md | OOTB | beacon_payload_encoding_v0, NodeTable index | banner-only ✓ |
| docs/protocols/ootb_ble_v0.md | OOTB | (none yet) — spec_map / #15 | banner-only ✓ |
| docs/protocols/radio_profile_presets_v0.md | OOTB | registry_radio_profiles_v0 | banner-only ✓ |
| docs/product/ootb_radio_preset_v0.md | OOTB | registry_radio_profiles_v0, module_boot_config_v0 | banner-only ✓ |
| docs/product/ootb_test_plan_v0.md | OOTB | field_cadence_v0; spec_map | banner-only ✓ |
| docs/product/ootb_scope_v0.md | OOTB | (none yet) — spec_map | banner-only ✓ |
| docs/product/ootb_gap_analysis_v0.md | OOTB | (none yet) — spec_map | banner-only ✓ |
| docs/product/OOTB_v0_analysis_and_plan.md | OOTB | (none yet) — spec_map | banner-only ✓ |
| docs/firmware/ootb_firmware_arch_v0.md | OOTB | boot_pipeline_v0 | banner-only ✓ |
| docs/firmware/ootb_node_table_v0.md | OOTB | nodetable index | banner-only ✓ |

**Excluded (per scope):** docs/product/areas/**, docs/product/wip/**, _working/**, docs/product/policy/ (canon process).

**Note:** docs/product/ootb/** and docs/product/legacy/** are empty; in-scope files are root-level legacy/OOTB under docs/, docs/product/, docs/protocols/, docs/firmware/.

---

## Actions taken

- **Banners added:** 10 (all OOTB).
- **Skipped (already compliant):** 0.
- **Needs decision:** 0 (all have either a canon link or explicit "none yet" + spec_map).
- Banner template used: `**Status:** Non-normative (OOTB). As-implemented v0 reference. May diverge from canon. Canon: [link or (none yet) — see spec_map].`
