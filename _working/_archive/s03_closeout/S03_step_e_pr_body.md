## S03 #412 step (e) — Final documentation integrity sweep

**Scope:** Fix remaining canon→WIP normative links for the 14 promoted S03 docs. No new promotions, no code, no semantic rewrites.

### Issues found and fixed

1. **Canon NodeTable policy → traffic_model (was WIP):**
   - [field_cadence_v0.md](docs/product/areas/nodetable/policy/field_cadence_v0.md): All 4 references to `../../../wip/areas/radio/policy/traffic_model_v0.md` → `../../radio/policy/traffic_model_v0.md` (canon).
   - [activity_state_v0.md](docs/product/areas/nodetable/policy/activity_state_v0.md): 2 references → same canon path.
   - [nodetable_fields_inventory_v0.md](docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md): 1 reference → same canon path.

2. **Canon firmware registries → semantics (were WIP):**
   - [fw_version_id_registry_v0.md](docs/product/areas/firmware/registry/fw_version_id_registry_v0.md): Link to `../../wip/areas/firmware/policy/fw_version_id_semantics_s03.md` → `../policy/fw_version_id_semantics_v0.md`; text "S03 WIP policy" → "canon policy".
   - [hw_profile_id_registry_v0.md](docs/product/areas/firmware/registry/hw_profile_id_registry_v0.md): Link to `../../wip/areas/firmware/policy/hw_profile_id_semantics_s03.md` → `../policy/hw_profile_id_semantics_v0.md`; text "S03 WIP policy" → "canon policy".

3. **spec_map_v0.md:** Changelog entry for step (e); confirms no remaining normative canon→WIP refs for the 14 promoted S03 docs.

### Verified unchanged / out of scope

- **Other canon→WIP links** (selection_policy_v0, autopower_policy_v0, pairing_flow_v0, source-precedence, snapshot-semantics, restore-merge-rules, etc.): Those targets were **not** in the 14 promoted; they remain WIP by design (post-V1A or supporting). No change.
- **_working/ references:** None in the canon/spec_map/current_state layer touched by #412; WIP research docs that mention _working/ were left as-is (not user-facing canon).
- **Stale pre-promotion paths:** No remaining references to the old WIP filenames (e.g. s03_nodetable_slice_v0_1, provisioning_baseline_s03) in the canon navigation layer; redirect stubs handle old URLs.

### Exit criteria

- [x] Final integrity sweep completed for the #412 docs layer.
- [x] Canon→WIP normative leaks for promoted material fixed (5 canon files).
- [x] spec_map and current_state consistent with canon; changelog added.
- [x] Docs-only PR; step (e) complete; #412 checklist complete.
