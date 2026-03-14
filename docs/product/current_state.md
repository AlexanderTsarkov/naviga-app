# Current product state

**Last updated:** 2026-03-14  
**Iteration tag:** S04 planning (BLE canon-promotion and slicing prep)  
**Scope:** Embedded-first (firmware → radio → domain → BLE bridge → mobile later).

---

## Source of truth (canon vs implementation)

**Canon docs define semantics.** Normative product and domain semantics are defined by documents under [docs/product/areas/](areas/). Policy and implementation must align to canon; canon overrides legacy and as-implemented behaviour when stated.

**Current firmware implementation is reference / previous implementation only.** It may illustrate current behaviour and inform defaults, but it is not the source of truth. OOTB and UI behaviour may be used as examples or indicators of current state only when explicitly marked non-normative; they must not define product truth. See [docs_promotion_policy_v0](policy/docs_promotion_policy_v0.md) §6 and [ai_model_policy](../dev/ai_model_policy.md). As of this update, merged S03 implementation is reconciled with corrected Product Truth; no open mismatch for S03 scope.

---

## Firmware

- **Boot pipeline:** Phase A (module boot / verify-and-repair), Phase B (provision role + radio profile from NVS), Phase C (start Alive/Beacon cadence). Canon: [boot_pipeline_v0](areas/firmware/policy/boot_pipeline_v0.md).
- **Module config:** E220/E22 UART modem and GNSS (u-blox M8N) verified on every boot; critical params (air rate, channel, RSSI, UART) apply + readback. Canon: [module_boot_config_v0](areas/firmware/policy/module_boot_config_v0.md).
- **Wiring:** [M1Runtime](areas/firmware/policy/radio_adapter_boundary_v1a.md) is the single composition point that receives IRadio and init flags; domain is radio-agnostic. Provisioning (role/radio pointers) via serial shell; see [provisioning_interface_v0](areas/firmware/policy/provisioning_interface_v0.md). OOTB autonomous start (#423) and user profile baseline (#443) landed.
- **Limits:** ESP32 + E220 or E22 as UART modem (no chip-level SPI driver). Legacy BLE service remains disabled unless explicitly planned. No mesh/JOIN; no backend.

---

## Radio / Protocol

- **Packet types (on-air):** v0.2 canonical family: Node_Pos_Full (0x06), Node_Status (0x07), Alive (0x02). 2-byte frame header; Common prefix (payloadVersion + nodeId48 + seq16) on all. RX is v0.2-only; v0.1 types no longer accepted. Canon: [packet_truth_table_v02](areas/nodetable/policy/packet_truth_table_v02.md), [packet_migration_v01_v02](areas/nodetable/policy/packet_migration_v01_v02.md), [beacon_payload_encoding_v0](areas/nodetable/contract/beacon_payload_encoding_v0.md).
- **TX queue:** Node_Pos_Full, Node_Status, Alive; priority and degrade under load per [field_cadence_v0](areas/nodetable/policy/field_cadence_v0.md) §4 and [packet_context_tx_rules_v0](areas/radio/policy/packet_context_tx_rules_v0.md).
- **Radio profiles:** Default (2.4 kbps) and Fast (4.8 kbps) presets; E22-400T30D V2 UART clamps air rate ≥ 2.4 kbps. Boot apply + readback verify. [registry_radio_profiles_v0](areas/radio/policy/registry_radio_profiles_v0.md), [radio_modules_naviga](../../hardware/radio_modules_naviga.md).
- **Out of scope:** CAD/LBT real sensing; channel sense OFF/UNSUPPORTED. Mitigation: jitter-only per policy.

---

## Domain / NodeTable

- **Implemented:** NodeTable as single source of truth for node-level state (self + remotes). v0.2: Node_Pos_Full (position+quality single packet), Node_Status (operational+informative snapshot), Alive (no-fix liveness); seq16 single per-node. RX semantics: accepted/duplicate/ooo/wrap. Legacy ref fields (last_core_seq16, last_applied_tail_ref*) are runtime-local decoder only; not in BLE or persistence. Canon: [NodeTable hub](areas/nodetable/index.md), [packet_truth_table_v02](areas/nodetable/policy/packet_truth_table_v02.md), [field_cadence_v0](areas/nodetable/policy/field_cadence_v0.md), [rx_semantics_v0](areas/nodetable/policy/rx_semantics_v0.md), [nodetable_master_field_table_v0](areas/nodetable/policy/nodetable_master_field_table_v0.md).
- **Contracts:** [packet_truth_table_v02](areas/nodetable/policy/packet_truth_table_v02.md) (v0.2); [beacon_payload_encoding_v0](areas/nodetable/contract/beacon_payload_encoding_v0.md), [alive](areas/nodetable/contract/alive_packet_encoding_v0.md), [tail1](areas/nodetable/contract/tail1_packet_encoding_v0.md), [tail2](areas/nodetable/contract/tail2_packet_encoding_v0.md), [info](areas/nodetable/contract/info_packet_encoding_v0.md).
- **Activity/quality:** [activity_state_v0](areas/nodetable/policy/activity_state_v0.md), [position_quality_v0](areas/nodetable/policy/position_quality_v0.md). Role-derived cadence (minInterval, maxSilence, minDisplacement) wired; GNSS scenario emulator for deterministic bench.

---

## Mobile

- **Current status:** Minimal; not in embedded-first critical path. BLE bridge (Phase D) is a pointer only in boot pipeline. Planning/audits: [s02_230_mobile_nodetable_integration_audit](areas/mobile/audit/s02_230_mobile_nodetable_integration_audit.md).

---

## Docs / Process

- **Canon:** Normative specs under [docs/product/areas/](areas/). Process policies under [docs/product/policy/](policy/). Layout: [docs_layout_policy_v0](policy/docs_layout_policy_v0.md).
- **WIP / index:** Drafts and inventory in [docs/product/wip/](wip/); spec map [spec_map_v0](wip/spec_map_v0.md).
- **Evidence / archive:** Bench and iteration evidence in \_working/ (repo root); iteration snapshots canonical location `archive/iterations/` (repo root); deprecated canon versions in [docs/product/archive/](archive/README.md). [docs_layout_policy_v0](policy/docs_layout_policy_v0.md) §10.

---

## What changed this iteration

| Iteration | Date range | Highlights | Links |
|-----------|------------|------------|--------|
| **OOTB** | (baseline) | Beacon + NodeTable baseline; E220 UART; serial provisioning; no BLE. | Reference only. |
| **S02** | 2026-02 – 2026-03 | V1-A closure. TX queue fairness / degrade order (Core > Tail-1 > Operational/Informative). Tail split: Operational (0x04) vs Informative (0x05). Boot pipeline Phase A/B/C; role/radio from NVS. E22 adapter + RadioPreset; GNSS scenario emulator. Docs: Stale link fixes, legacy/OOTB banners, _working index, canon archive. | [#277](https://github.com/AlexanderTsarkov/naviga-app/issues/277) (gate), [#224](https://github.com/AlexanderTsarkov/naviga-app/issues/224) (epic); [#322](https://github.com/AlexanderTsarkov/naviga-app/pull/322), [#341](https://github.com/AlexanderTsarkov/naviga-app/pull/341), [#344](https://github.com/AlexanderTsarkov/naviga-app/pull/344), [#345](https://github.com/AlexanderTsarkov/naviga-app/pull/345)–[#349](https://github.com/AlexanderTsarkov/naviga-app/pull/349). |
| **S03 promotion (c)+(d)** | 2026-03-10 | spec_map updated for 14 S03 promoted docs (canon paths, Promote=Promoted). current_state: source-of-truth / reference-implementation disclaimer added. Canon navigation: Dashboard and §4 traffic_model → canon; no normative WIP refs for promoted S03 material. Step (e) final integrity sweep remains open. | [#412](https://github.com/AlexanderTsarkov/naviga-app/issues/412), [#413](https://github.com/AlexanderTsarkov/naviga-app/pull/413) (step b). |
| **S03 execution** | 2026-03 | P0: persisted seq16 (#417), NodeTable snapshot+restore (#418), field map (#419), TX formation canon lock (#420), RX semantics (#421), packetization (#422). Boot/OOTB: Phase A/B/C + autonomous start (#423), radio profile baseline (#424), observability (#425), user profile baseline (#443). Protocol: v0.2 packet family + v0.1 RX cutover (#435, #438). current_state reconciled (#426). | [#416](https://github.com/AlexanderTsarkov/naviga-app/issues/416) (umbrella), #417–#425, #435, #438, #443. |

---

## Next focus

- **S04 (planning):** BLE WIP → canon review and promotion decision first; then implementation slicing; implementation work in later issues. Umbrella [#460](https://github.com/AlexanderTsarkov/naviga-app/issues/460); input artifact [ble_contract_s04_v0.md](wip/areas/mobile/ble_contract_s04_v0.md) (WIP, #361).
- **Post–S03:** Docs cleanup and legacy migration [#278](https://github.com/AlexanderTsarkov/naviga-app/issues/278) (PR-1–PR-3 landed); planning track [#296](https://github.com/AlexanderTsarkov/naviga-app/issues/296) as needed.
- Channel discovery / selection and AutoPower: post–V1-A ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175), [#180](https://github.com/AlexanderTsarkov/naviga-app/issues/180)).

---

## Update procedure

- After each sprint: update **Last updated**, **Iteration tag**, and append one row to the **What changed this iteration** table.
- Keep sections to 5–10 lines; add links to canon or merged PRs for any new claim.
