# Docs cleanup after S03 closeout — plan and audit

**Date:** 2026-03-14  
**Scope:** Canon/archive structure; remove duplicate legacy structure outside `docs/product/`.  
**Non-scope:** No `_working` reorg; no BLE canon promotion; no S04; no implementation.

---

## 1. Current layout (pre-cleanup)

- **Canon:** `docs/product/areas/**` (nodetable, radio, firmware, domain, etc.) — source of truth per current_state and docs_layout_policy_v0.
- **WIP:** `docs/product/wip/**` — BLE contract WIP remains here; not promoted.
- **Archive:** `docs/product/archive/` — had only README; will receive moved legacy/deprecated docs.
- **Old structure (competing/reference):** `docs/firmware/*.md` (specs), `docs/protocols/*.md` (OOTB radio/BLE/presets), and some `docs/product/*.md` root files (OOTB analysis, scope, presets) — referenced by architecture index and docs_map as "Reference" or "Seed" source of truth, but canon is now in `docs/product/areas/`.

---

## 2. Candidates for move to archive

### 2.1 From `docs/firmware/` → `docs/product/archive/legacy_firmware/`

| File | Rationale |
|------|------------|
| ootb_node_table_v0.md | NodeTable spec superseded by docs/product/areas/nodetable/*. |
| ootb_firmware_arch_v0.md | Boot/arch superseded by areas/firmware/policy (boot_pipeline_v0, module_boot_config_v0, radio_adapter_boundary_v1a). |
| hal_contracts_v0.md | HAL/contract superseded by areas/firmware policy and M1Runtime boundary. |
| gnss_stub_v0.md, gnss_v0.md, gnss_capabilities_map_v0.md | GNSS semantics in areas/firmware (gnss_scenario_emulator_v0, module_boot_config). |
| geo_utils_v0.md | Geo encoding in areas/nodetable contracts. |
| ublox_uart_driver_plan_v0.md | Driver plan; legacy. |

**Keep in docs/firmware/:** README.md (build/BLE test — operational), e220_radio_implementation_status.md (impl status), poc_e220_evidence.md (evidence), stand_tests/ (test reports).

### 2.2 From `docs/protocols/` → `docs/product/archive/legacy_protocols/`

| File | Rationale |
|------|------------|
| ootb_radio_v0.md | OOTB radio frame/packet spec; canon for packets/TX is in areas/nodetable (packet_truth_table_v02, beacon_payload_encoding_v0, etc.) and areas/radio (packet_context_tx_rules_v0). Still linked from canon for §3 frame header; links will be updated to archive path. |
| ootb_ble_v0.md | BLE GATT/NodeTable snapshot; BLE contract WIP is in wip/areas/mobile/ble_contract_s04_v0.md. |
| radio_profile_presets_v0.md | Presets superseded by areas/radio (registry_radio_profiles_v0, radio_profiles_policy_v0). |

**Neutralize docs/protocols/:** Replace with README pointing to canon (areas/) and archive (legacy_protocols/).

### 2.3 From `docs/product/` root → `docs/product/archive/legacy_product/`

| File | Rationale |
|------|------------|
| OOTB_v0_analysis_and_plan.md | Planning doc; OOTB scope/execution now in areas/firmware and current_state. |
| ootb_scope_v0.md | Scope superseded by boot_pipeline_v0, provisioning_baseline_v0, ootb_autonomous_start_v0 in areas. |
| ootb_radio_preset_v0.md | Presets in areas/radio. |
| ootb_test_plan_v0.md | Test plan; historical. |
| ootb_gap_analysis_v0.md | Gap analysis; historical. |
| radio_channel_mapping_v0.md | Channel mapping in areas/radio policy/registry. |

**Keep in product root:** README.md, current_state.md, glossary.md, naviga_product_core.md, naviga_vision_decision_edition.md, naviga_ootb_and_activation_scenarios.md, naviga_join_session_logic_v1_2.md, naviga_mesh_protocol_concept_v1_4.md (vision/future), logging_v0.md, poc_gaps_risks.md, policy/, areas/, wip/, archive/.

---

## 3. Link updates (canon → moved files)

- **Canon docs** that reference `docs/protocols/ootb_radio_v0.md`: update to `docs/product/archive/legacy_protocols/ootb_radio_v0.md` (or relative from areas: `../../archive/legacy_protocols/ootb_radio_v0.md`).
  - Affected: rx_semantics_v0.md, tail1_packet_encoding_v0.md, beacon_payload_encoding_v0.md (under areas/nodetable).
- **ADR** adr/ootb_position_source_v0.md references hal_contracts_v0, gnss_v0 → update to archive paths.
- **Other references** (poc_gaps_risks, spec_map, OOTB_v0_analysis, wip research, firmware docs) — update to archive paths where they point to moved files.

---

## 4. Meta-docs to update

- **docs/project/docs_map.md:** Canon = product/areas + current_state; Reference = architecture, adr; legacy specs = product/archive (and _archive/ootb_v1 for OOTB evidence).
- **docs/START_HERE.md:** Point to docs/product/README and current_state as product truth; mention archive for legacy.
- **docs/architecture/index.md:** Source-of-truth table: primary docs = docs/product/areas/* and current_state; legacy/archived = docs/product/archive/*; remove or qualify references to docs/firmware and docs/protocols as primary.
- **docs/firmware/README.md:** Add line: Product canon for firmware/radio/node table lives in docs/product/areas/; legacy specs in docs/product/archive/legacy_firmware/.
- **docs/product/archive/README.md:** Extend with subsections legacy_firmware, legacy_protocols, legacy_product and deprecation dates.

---

## 5. Intentionally untouched

- **docs/product/areas/** — No moves out of areas/; canon stays. Only link edits.
- **docs/product/wip/** — Unchanged; BLE WIP remains WIP.
- **_working/** — No reorg or archiving in this pass.
- **CLEAN_SLATE.md, design/, backend/, mobile/, adr/** — Out of scope.

---

## 6. Execution order

1. Create `docs/product/archive/legacy_firmware/`, `legacy_protocols/`, `legacy_product/`.
2. Move files (git mv) from firmware/, protocols/, product root to archive.
3. Update links in canon and other docs to archive paths.
4. Add README in docs/protocols; update README in docs/firmware; update docs/product/archive/README.
5. Update docs_map, START_HERE, architecture index.
6. Verify no broken links; open PR.

---

## 7. Outcome (done)

- **Moved to archive:** legacy_firmware (8 files), legacy_protocols (3), legacy_product (6). See archive/README subsections.
- **Meta-docs updated:** docs_map, START_HERE (product canon + archive row), architecture index (table links + section E layout), firmware/README, protocols/README, product/archive/README.
- **Links updated:** canon (rx_semantics, tail1, beacon_payload), adr/ootb_position_source, poc_gaps_risks, poc_e220_evidence, s02_229 audit, hardware/radio_modules_naviga; internal links inside archived files.
- **Intentionally untouched:** WIP research docs (geo_encoding_audit, seq_audit, nodeid_audit, tail2_split) still mention old paths in audit tables — historical context; docs now live in archive. BLE WIP remains under wip/; no canon promotion.
- **PR:** docs-only; branch `docs/post-s03-canon-cleanup`.
