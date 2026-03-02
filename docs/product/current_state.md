# Current product state

**Last updated:** 2026-03-01  
**Iteration tag:** S02  
**Scope:** Embedded-first (firmware → radio → domain → BLE bridge → mobile later).

---

## Firmware

- **Boot pipeline:** Phase A (module boot / verify-and-repair), Phase B (provision role + radio profile from NVS), Phase C (start Alive/Beacon cadence). Canon: [boot_pipeline_v0](areas/firmware/policy/boot_pipeline_v0.md).
- **Module config:** E220/E22 UART modem and GNSS (u-blox M8N) verified on every boot; critical params (air rate, channel, RSSI, UART) apply + readback. Canon: [module_boot_config_v0](areas/firmware/policy/module_boot_config_v0.md).
- **Wiring:** [M1Runtime](areas/firmware/policy/radio_adapter_boundary_v1a.md) is the single composition point that receives IRadio and init flags; domain is radio-agnostic. Provisioning (role/radio pointers) via serial shell; see [provisioning_interface_v0](areas/firmware/policy/provisioning_interface_v0.md).
- **Limits:** ESP32 + E220 or E22 as UART modem (no chip-level SPI driver). Legacy BLE service remains disabled unless explicitly planned. No mesh/JOIN; no backend.

---

## Radio / Protocol

- **Packet types (on-air):** Core_Pos (0x01), Alive (0x02), Core_Tail (0x03), Operational (0x04), Informative (0x05). 2-byte frame header; Common prefix (payloadVersion + nodeId48 + seq16) on all. Canon: [beacon_payload_encoding_v0](areas/nodetable/contract/beacon_payload_encoding_v0.md), [ootb_radio_v0](../../protocols/ootb_radio_v0.md).
- **TX queue:** Priority order (P0 Core/Alive > P2 best-effort); Tail-1 (Core_Tail) and Operational/Informative sub-ordered; degrade under load per [field_cadence_v0](areas/nodetable/policy/field_cadence_v0.md) §4. Implemented (e.g. [PR #322](https://github.com/AlexanderTsarkov/naviga-app/pull/322)).
- **Radio profiles:** Default (2.4 kbps) and Fast (4.8 kbps) presets; E22-400T30D V2 UART clamps air rate ≥ 2.4 kbps. Boot apply + readback verify. [registry_radio_profiles_v0](areas/radio/policy/registry_radio_profiles_v0.md), [radio_modules_naviga](../../hardware/radio_modules_naviga.md).
- **Out of scope:** CAD/LBT real sensing; channel sense OFF/UNSUPPORTED. Mitigation: jitter-only per policy.

---

## Domain / NodeTable

- **Implemented:** NodeTable as single source of truth for node-level state (self + remotes). Core only with valid fix; Alive for no-fix liveness; seq16 single per-node; Core_Tail ref_core_seq16 linkage; Operational vs Informative split (independent TX paths). RX semantics: accepted/duplicate/ooo/wrap; Tail-1 apply only if ref matches lastCoreSeq. Canon: [NodeTable hub](areas/nodetable/index.md), [field_cadence_v0](areas/nodetable/policy/field_cadence_v0.md), [rx_semantics_v0](areas/nodetable/policy/rx_semantics_v0.md).
- **Contracts:** [beacon_payload_encoding_v0](areas/nodetable/contract/beacon_payload_encoding_v0.md), [alive](areas/nodetable/contract/alive_packet_encoding_v0.md), [tail1](areas/nodetable/contract/tail1_packet_encoding_v0.md), [tail2](areas/nodetable/contract/tail2_packet_encoding_v0.md), [info](areas/nodetable/contract/info_packet_encoding_v0.md).
- **Activity/quality:** [activity_state_v0](areas/nodetable/policy/activity_state_v0.md), [position_quality_v0](areas/nodetable/policy/position_quality_v0.md). Role-derived cadence (minInterval, maxSilence, minDisplacement) wired; GNSS scenario emulator for deterministic bench.

---

## Mobile

- **Current status:** Minimal; not in embedded-first critical path. BLE bridge (Phase D) is a pointer only in boot pipeline. Planning/audits: [s02_230_mobile_nodetable_integration_audit](areas/mobile/audit/s02_230_mobile_nodetable_integration_audit.md).

---

## Docs / Process

- **Canon:** Normative specs under [docs/product/areas/](areas/). Process policies under [docs/product/policy/](policy/). Layout: [docs_layout_policy_v0](policy/docs_layout_policy_v0.md).
- **WIP / index:** Drafts and inventory in [docs/product/wip/](wip/); spec map [spec_map_v0](wip/spec_map_v0.md).
- **Evidence / archive:** Bench and iteration evidence in [\_working/](../../_working/README.md); iteration snapshots canonical location `archive/iterations/` (repo root); deprecated canon versions in [docs/product/archive/](archive/README.md). [docs_layout_policy_v0](policy/docs_layout_policy_v0.md) §10.

---

## What changed this iteration

| Iteration | Date range | Highlights | Links |
|-----------|------------|------------|--------|
| **OOTB** | (baseline) | Beacon + NodeTable baseline; E220 UART; serial provisioning; no BLE. | Reference only. |
| **S02** | 2026-02 – 2026-03 | V1-A closure. TX queue fairness / degrade order (Core > Tail-1 > Operational/Informative). Tail split: Operational (0x04) vs Informative (0x05). Boot pipeline Phase A/B/C; role/radio from NVS. E22 adapter + RadioPreset; GNSS scenario emulator. Docs: Stale link fixes, legacy/OOTB banners, _working index, canon archive. | [#277](https://github.com/AlexanderTsarkov/naviga-app/issues/277) (gate), [#224](https://github.com/AlexanderTsarkov/naviga-app/issues/224) (epic); [#322](https://github.com/AlexanderTsarkov/naviga-app/pull/322), [#341](https://github.com/AlexanderTsarkov/naviga-app/pull/341), [#344](https://github.com/AlexanderTsarkov/naviga-app/pull/344), [#345](https://github.com/AlexanderTsarkov/naviga-app/pull/345)–[#349](https://github.com/AlexanderTsarkov/naviga-app/pull/349). |

---

## Next focus

- **S03 planning:** [#296](https://github.com/AlexanderTsarkov/naviga-app/issues/296).
- Persisted seq16 / snapshot semantics (post–V1-A).
- Docs cleanup and legacy migration: [#278](https://github.com/AlexanderTsarkov/naviga-app/issues/278) (PR-1–PR-3 landed).
- Channel discovery / selection and AutoPower: post–V1-A ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175), [#180](https://github.com/AlexanderTsarkov/naviga-app/issues/180)).

---

## Update procedure

- After each sprint: update **Last updated**, **Iteration tag**, and append one row to the **What changed this iteration** table.
- Keep sections to 5–10 lines; add links to canon or merged PRs for any new claim.
