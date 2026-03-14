## S03 WIP → Canon promotion: steps (c) and (d) — [#412](https://github.com/AlexanderTsarkov/naviga-app/issues/412)

**Scope:** Canon navigation and source-of-truth disclaimer only. No new promotions, no code changes. Step (b) already merged via [#413](https://github.com/AlexanderTsarkov/naviga-app/pull/413).

### (c) spec_map and canon navigation

- **spec_map_v0.md**
  - **Inventory:** Updated/added rows for all 14 S03-promoted docs with correct **canon** paths (`../areas/...`) and **Promote = Promoted**:
    - NodeTable: `s03_nodetable_slice_v0`, `nodetable_field_map_v0`, `packet_to_nodetable_mapping_v0`, `packet_sets_v0`, `presence_and_age_semantics_v0`, `identity_naming_persistence_eviction_v0`, `seq_ref_version_link_metrics_v0`, `tx_priority_and_arbitration_v0`
    - Radio: `traffic_model_v0`, `packet_context_tx_rules_v0`
    - Firmware: `provisioning_baseline_v0`, `ootb_autonomous_start_v0`, `fw_version_id_semantics_v0`, `hw_profile_id_semantics_v0`
  - **Dashboard:** Radio policy `traffic_model_v0` link now points to canon (`../areas/radio/policy/traffic_model_v0.md`); note added that traffic_model is canon (S03 #412).
  - **§4 Canon → WIP:** traffic_model rows updated — target is now canon `areas/radio/policy/traffic_model_v0.md` (no longer WIP); notes reference S03 #412.
  - **§ Next issues:** traffic_model link updated to canon.
  - **Snapshot / Canonical specs:** Snapshot 2026-03-10; canonical specs line clarified (promoted in `docs/product/areas/`, remainder in `docs/product/wip/areas/`).
  - **Changelog:** Entry for 2026-03-10 documenting steps (c)+(d) and the 14 doc inventory updates.

- **Navigation:** No normative reader is routed to WIP for the promoted S03 material; all relevant links point to canon.

### (d) current_state — source-of-truth disclaimer

- **current_state.md**
  - New section **Source of truth (canon vs implementation):**
    - Canon docs under `docs/product/areas/` define semantics; policy and implementation must align to canon.
    - Current firmware implementation is **reference / previous implementation only**; not source of truth.
    - OOTB and UI are non-normative (examples/indicators only). Links to [docs_promotion_policy_v0](docs/product/policy/docs_promotion_policy_v0.md) §6 and [ai_model_policy](docs/dev/ai_model_policy.md).
  - **What changed:** One row for S03 promotion (c)+(d) with links to #412 and #413.
  - **Last updated / Iteration tag:** 2026-03-10; S02 / S03 promotion.

### Not in this PR

- Step (e) final integrity sweep — still open.
- No new promotions, no redirect stubs, no code/firmware changes.
- No semantic rewrites of promoted policy docs.
