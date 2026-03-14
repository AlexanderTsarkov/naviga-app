# S03 WIP → Canon Promotion — Step (a) Inventory (authoritative)

**Purpose:** Step (a) of [#412](https://github.com/AlexanderTsarkov/naviga-app/issues/412): enumerate S03 WIP docs and classify each as **promote to canon** (with exact target path) or **keep in WIP** (with explicit "WIP (not canon)" banner + one-line reason). No promotion/move/merge performed in this pass.

**Policies:** [docs_promotion_policy_v0.md](../docs/product/policy/docs_promotion_policy_v0.md), [docs_layout_policy_v0.md](../docs/product/policy/docs_layout_policy_v0.md). Canon paths: `docs/product/areas/<area>/{contract,policy}/`; no `slices/` in layout — use `policy/` for slice index.

**Verified:** All WIP paths below exist in repo. Canon targets checked for layout compliance and merge/supersede candidates.

---

## 1) Promote to canon (exact target path)

Each row: WIP path (exists) → **exact canon target path**. Step (b) will perform move/merge + redirect stub.

| # | WIP doc (path under `docs/product/wip/areas/`) | Exact canon target path | Notes |
|---|-----------------------------------------------|-------------------------|--------|
| **NodeTable** | | | |
| 1 | nodetable/policy/nodetable_field_map_v0_1.md | areas/nodetable/policy/nodetable_field_map_v0.md | Index to master table (#352). **Merge candidate:** canon already has `nodetable_fields_inventory_v0.md` (worksheet); field_map is index to CSV/XLSX. Step (b): promote as separate doc or merge into inventory per owner. |
| 2 | nodetable/policy/packet_to_nodetable_mapping_s03.md | areas/nodetable/policy/packet_to_nodetable_mapping_v0.md | Packet → NodeTable mapping (#358). No collision. |
| 3 | nodetable/policy/packet_sets_v0_1.md | areas/nodetable/policy/packet_sets_v0.md | Core/Tail/Operational/Informative. No collision. |
| 4 | nodetable/policy/presence_and_age_semantics_v0_1.md | areas/nodetable/policy/presence_and_age_semantics_v0.md | is_stale, last_seen_age_s, pos_age_s. No collision. |
| 5 | nodetable/policy/identity_naming_persistence_eviction_v0_1.md | areas/nodetable/policy/identity_naming_persistence_eviction_v0.md | Identity, naming, persistence, eviction. No collision. |
| 6 | nodetable/policy/seq_ref_version_link_metrics_v0_1.md | areas/nodetable/policy/seq_ref_version_link_metrics_v0.md | Seq, ref, payload version, link metrics. No collision. |
| 7 | nodetable/policy/tx_priority_and_arbitration_v0_1.md | areas/nodetable/policy/tx_priority_and_arbitration_v0.md | P0–P3, expired_counter. No collision. |
| 8 | nodetable/slices/s03_nodetable_slice_v0_1.md | areas/nodetable/policy/s03_nodetable_slice_v0.md | S03 slice index. Layout has no `slices/`; target under `policy/`. Step (b): update internal links to canon policy paths. |
| **Radio** | | | |
| 9 | radio/policy/packet_context_tx_rules_v0_1.md | areas/radio/policy/packet_context_tx_rules_v0.md | #407 outcome; packet purpose, creation, send rules. No collision. |
| 10 | radio/policy/traffic_model_s03_v0_1.md | areas/radio/policy/traffic_model_v0.md | S03 traffic model. **Note:** WIP also has `traffic_model_v0.md` (older?). Step (b): promote S03 content; replace or merge with existing WIP traffic_model_v0 if needed. |
| **Firmware** | | | |
| 11 | firmware/policy/provisioning_baseline_s03.md | areas/firmware/policy/provisioning_baseline_v0.md | #385 / #353. No collision. |
| 12 | firmware/policy/ootb_autonomous_start_s03.md | areas/firmware/policy/ootb_autonomous_start_v0.md | #354. No collision. |
| 13 | firmware/policy/fw_version_id_semantics_s03.md | areas/firmware/policy/fw_version_id_semantics_v0.md | #405; registry already in canon. No collision. |
| 14 | firmware/policy/hw_profile_id_semantics_s03.md | areas/firmware/policy/hw_profile_id_semantics_v0.md | #406; registry already in canon. No collision. |

**Registries (already canon):** `areas/firmware/registry/fw_version_id_registry_v0.md`, `areas/firmware/registry/hw_profile_id_registry_v0.md`. Step (b): ensure canon policy docs link only to these; no WIP semantics links.

---

## 2) Keep in WIP ("WIP (not canon)" banner + reason)

Each doc remains in WIP; add visible banner and one-line reason. No move.

| # | WIP doc (path under `docs/product/wip/areas/`) | Banner-ready reason (one line) |
|---|-----------------------------------------------|----------------------------------|
| 1 | radio/policy/tx_power_contract_s03.md | Under review; not yet agreed stable for canon. |
| 2 | radio/policy/radio_profiles_model_s03.md | Needs consolidation with canon radio_profiles_policy_v0; do not promote until aligned. |
| 3 | radio/policy/e220_radio_profile_mapping_s03.md | E220-specific mapping; keep WIP until radio policy promotion batch. |
| 4 | radio/policy/channel_utilization_budgeting_and_profile_strategy_s03_v0_1.md | Preliminary; assumption-driven; defer to later tuning. |
| 5 | radio/policy/radio_user_settings_review_s03.md | Audit/review doc; not normative; keep WIP unless folded into canon policy. |
| 6 | nodetable/contract/gnss_tail_completeness_and_budgets_s03.md | GNSS/tail budgets; keep WIP until tail encoding is fully canon. |
| 7 | domain/policy/node_role_type_review_s03.md | Review/audit; keep WIP or fold into role_profiles_policy_v0 in later step. |
| 8 | mobile/ble_snapshot_s04_gate.md | **WIP gate only.** S04 BLE snapshot design deferred; gating note and S04 entry checklist; not a BLE spec. Banner: "WIP (not canon) — gating note; BLE not specified." |

---

## 3) Canon-only updates (deferred to step (b) or later)

- **Reference-impl disclaimer:** Add or confirm in `docs/product/current_state.md` (or policy): *"Current firmware implementation is a reference/previous implementation; semantics are defined by canon docs."*
- **spec_map_v0.md:** Set promotion status and promoted paths for each promoted doc; ensure no canonical doc links to WIP for normative definitions. (Step (c).)
- **provisioning_interface_v0.md** (and any canon that links to WIP): Replace links to `fw_version_id_semantics_s03` / `hw_profile_id_semantics_s03` with canon paths after promotion. (Step (b).)

---

## 4) Summary (step (a) only)

| Classification | Count | Next |
|----------------|-------|------|
| **Promote to canon** | 14 | Step (b): move/merge, redirect stub, Status: Canon. |
| **Keep in WIP** | 8 | Add "WIP (not canon)" banner + reason; no move. |
| **Canon already** | 2 (registries) | No move; link fixes in step (b). |
| **Merge/collision notes** | 2 | field_map vs fields_inventory; traffic_model_s03 vs traffic_model_v0 in WIP. Resolve in step (b). |

**Deviations from draft inventory:** None. All listed WIP files verified present. S03 slice target set to `policy/s03_nodetable_slice_v0.md` (layout has no `slices/`). Traffic model target set to `traffic_model_v0.md` with note to reconcile with existing WIP `traffic_model_v0.md` in step (b).

**Ambiguities deferred to step (b):** (1) Whether to promote nodetable_field_map_v0 as separate doc or merge into nodetable_fields_inventory_v0. (2) How to handle WIP `traffic_model_v0.md` when promoting `traffic_model_s03_v0_1.md`.

---

*No promotion, move, merge, or redirect stubs were performed in this pass. Step (a) complete.*
