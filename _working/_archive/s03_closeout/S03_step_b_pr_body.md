## S03 WIP → Canon promotion — Step (b) only

**Refs:** [#412](https://github.com/AlexanderTsarkov/naviga-app/issues/412) (step b); umbrella [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351).

### Summary

- **14 docs** from the step-(a) inventory are promoted to canon at the exact target paths; each original WIP path now has a **redirect stub** per layout policy.
- **field_map:** Promoted as a **separate** canon doc (`nodetable_field_map_v0.md`). No merge with `nodetable_fields_inventory_v0.md`; field_map is the index to the master table; inventory remains the worksheet.
- **traffic_model:** S03 utilization/calculations doc promoted as canon `traffic_model_v0.md`. The older WIP `traffic_model_v0.md` (channel/frame policy: OOTB vs Session/Group, ProductMaxFrameBytes=96) was **folded in** as §9 "Channel and frame policy (v0)" so one canon doc holds both utilization and frame policy. Both WIP traffic_model files now redirect to the same canon doc.
- **Firmware semantics** (`fw_version_id_semantics_v0`, `hw_profile_id_semantics_v0`) link only to **canon registries** (`../registry/...`). `provisioning_interface_v0.md` updated to point to the new canon semantics docs.
- **Steps (c)–(e)** (spec_map/navigation sweep, reference-impl disclaimer, integrity pass) are **not** done in this PR.

### Files changed

- **14 new canon docs** under `docs/product/areas/{nodetable,radio,firmware}/policy/` (and `s03_nodetable_slice_v0.md` under nodetable/policy).
- **15 WIP files** replaced with redirect stubs (14 promoted sources + WIP `traffic_model_v0.md`).
- **1 canon doc updated:** `provisioning_interface_v0.md` — links to provisioning_baseline and fw/hw semantics now point to canon paths.

### Quality

- Docs-only; no code or firmware changes.
- No "Keep in WIP" items were promoted.
- Canon docs are self-contained; internal links point to canon or explicit WIP paths where the target is still WIP.
