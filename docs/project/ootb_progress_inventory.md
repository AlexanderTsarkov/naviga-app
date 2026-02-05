# OOTB Progress Inventory (Post-Issue #11)

**Date:** 2026-02-04  
**Phase:** Phase 1 (M0 docs + firmware bring-up in progress)  
**Source of truth:** GitHub issues + merged PRs

## A) Status snapshot
- Issue [#11](https://github.com/AlexanderTsarkov/naviga-app/issues/11) is **OPEN**, but steps **11.1–11.4** are delivered via merged PRs.
- Issue [#18](https://github.com/AlexanderTsarkov/naviga-app/issues/18) is **DONE** (PR #57).
- Firmware bring-up (skeleton, identity, BLE read-only, GNSS stub) is in place; radio TX/RX is not yet started.
- Docs-only artifacts for earlier phase items (#3–#10) are closed.

## B) Issue #11 — Delivered (checklist items + PR links)
- **11.1 Firmware skeleton + tick loop + service container**  
  PR [#51](https://github.com/AlexanderTsarkov/naviga-app/pull/51) → merge `61831d5`
- **11.2 Node identity + short_id + self NodeTable entry + paging API**  
  PR [#52](https://github.com/AlexanderTsarkov/naviga-app/pull/52) → merge `86e1317`
- **11.3 BLE read-only: DeviceInfo + NodeTable pages**  
  PR [#53](https://github.com/AlexanderTsarkov/naviga-app/pull/53) → merge `05d87fe`
- **11.4 GNSS stub + geo utils + self update policy**  
  PR [#55](https://github.com/AlexanderTsarkov/naviga-app/pull/55) → merge `6365171`

## C) Cross-impact (overlap with other planned tickets)
- **#12 NodeTable spec** — partially addressed by implementation scaffolding (self entry + paging), but spec doc remains open.
- **#15 OOTB BLE v0 spec** — partial overlap: read-only DeviceInfo/NodeTable pages implemented; spec doc still open.
- **#43 GNSS v0 contract** — partial overlap: GNSS stub and policy added; contract doc still open.
- **#20 Logging v0** — serial logs exist, but ring-buffer/export scope not addressed.

## D) Remaining scope in #11 and next recommended issue
- **Issue #11**: still open; close after confirming doc status and checklist closure in GitHub.
- **Recommended next issue:** [#21 HAL radio driver](https://github.com/AlexanderTsarkov/naviga-app/issues/21)

## E) Next 3 suggested steps (from OOTB plan)
1) **[#21] HAL radio driver** — start real TX/RX path (per decision in issue).  
2) **[#23] GEO_BEACON codec** — radio payload encode/decode for end-to-end.  
3) **[#20] Logging v0** — ring-buffer + export (if needed before beacon logic).

## OOTB Plan Issues (2–36)
| Issue | Title | Planned Scope | Status | Evidence |
|---|---|---|---|---|
| [#2](https://github.com/AlexanderTsarkov/naviga-app/issues/2) | Epic: OOTB End-to-End v0 | Epic tracking | Open | N/A |
| [#3](https://github.com/AlexanderTsarkov/naviga-app/issues/3) | 0.1 GitHub Project setup | Project board | Done | Project board (issue refs) |
| [#4](https://github.com/AlexanderTsarkov/naviga-app/issues/4) | 0.2 Epic created | Epic #2 | Done | Issue #2 |
| [#5](https://github.com/AlexanderTsarkov/naviga-app/issues/5) | 0.3 Minimal CI | CI workflows | Done | `.github/workflows/` |
| [#6](https://github.com/AlexanderTsarkov/naviga-app/issues/6) | 0.4 Map SDK decision | Doc | Done | `docs/mobile/map_sdk_decision.md` |
| [#7](https://github.com/AlexanderTsarkov/naviga-app/issues/7) | 0.5.1 POC evidence | Doc | Done | `docs/firmware/poc_e220_evidence.md` |
| [#8](https://github.com/AlexanderTsarkov/naviga-app/issues/8) | 0.5.2 POC gaps/risks | Doc | Done | `docs/product/poc_gaps_risks.md` |
| [#9](https://github.com/AlexanderTsarkov/naviga-app/issues/9) | 0.5.3 Archive cleanup | Repo hygiene | Done | `_archive/` (per issue) |
| [#10](https://github.com/AlexanderTsarkov/naviga-app/issues/10) | 1.1 Architecture Index | Doc | Done | `docs/architecture/index.md` |
| [#11](https://github.com/AlexanderTsarkov/naviga-app/issues/11) | 1.2 Firmware architecture | Doc + steps 11.1–11.4 | Partial | PRs #51/#52/#53/#55 |
| [#12](https://github.com/AlexanderTsarkov/naviga-app/issues/12) | 1.3 NodeTable spec | Doc | Done | [docs/firmware/ootb_node_table_v0.md](../firmware/ootb_node_table_v0.md) |
| [#13](https://github.com/AlexanderTsarkov/naviga-app/issues/13) | 1.4 HAL contracts | Doc | Done | [docs/firmware/hal_contracts_v0.md](../firmware/hal_contracts_v0.md) |
| [#14](https://github.com/AlexanderTsarkov/naviga-app/issues/14) | 1.5 OOTB Radio v0 spec | Doc | Done | [docs/protocols/ootb_radio_v0.md](../protocols/ootb_radio_v0.md) |
| [#15](https://github.com/AlexanderTsarkov/naviga-app/issues/15) | 1.6 OOTB BLE v0 spec | Doc | Done | [docs/protocols/ootb_ble_v0.md](../protocols/ootb_ble_v0.md) |
| [#16](https://github.com/AlexanderTsarkov/naviga-app/issues/16) | 1.7 Test Plan v0 | Doc | Done | [docs/product/ootb_test_plan_v0.md](../product/ootb_test_plan_v0.md) |
| [#17](https://github.com/AlexanderTsarkov/naviga-app/issues/17) | 1.8 ADR: OOTB scope | Doc | Done | [docs/product/ootb_scope_v0.md](../product/ootb_scope_v0.md) (issue refs docs/adr; file is in docs/product) |
| [#18](https://github.com/AlexanderTsarkov/naviga-app/issues/18) | 2.0 HAL interfaces + mocks | Code | Done | PR [#57](https://github.com/AlexanderTsarkov/naviga-app/pull/57) |
| [#19](https://github.com/AlexanderTsarkov/naviga-app/issues/19) | 2.1 Firmware module tree | Code structure | Done | PR [#60](https://github.com/AlexanderTsarkov/naviga-app/pull/60), PR [#61](https://github.com/AlexanderTsarkov/naviga-app/pull/61) — Core purity: platform abstractions + platform adapters; removed Arduino/Serial/delay from app/services; CI guardrails added. |
| [#20](https://github.com/AlexanderTsarkov/naviga-app/issues/20) | 2.2 Logging v0 | Code | Done | PR [#65](https://github.com/AlexanderTsarkov/naviga-app/pull/65) — Logging v0: RAM ring buffer + UART export; TX/RX, NodeTable update, BLE connect/read events; test_native. |
| [#21](https://github.com/AlexanderTsarkov/naviga-app/issues/21) | 2.3 HAL radio driver | Code | Open | N/A |
| [#22](https://github.com/AlexanderTsarkov/naviga-app/issues/22) | 2.4 HAL BLE transport | Code | Done | `firmware/src/platform/ble_transport_core.{h,cpp}`, `firmware/src/platform/ble_esp32_transport.{h,cpp}`, `firmware/test/test_ble_transport_core/test_ble_transport_core.cpp` |
| [#23](https://github.com/AlexanderTsarkov/naviga-app/issues/23) | 2.5 GEO_BEACON codec | Code | Done | PR [#63](https://github.com/AlexanderTsarkov/naviga-app/pull/63). GEO_BEACON codec: fixed 24-byte core, little-endian; decode ignores trailing bytes; pos_valid==0 skips range-check (fields parsed as-is); unit tests (test_native). |
| [#24](https://github.com/AlexanderTsarkov/naviga-app/issues/24) | 2.6 NodeTable domain | Code | Open | N/A |
| [#25](https://github.com/AlexanderTsarkov/naviga-app/issues/25) | 2.7 Beacon logic | Code | Open | N/A |
| [#26](https://github.com/AlexanderTsarkov/naviga-app/issues/26) | 2.8 BLE NodeTable bridge | Code | Open | N/A |
| [#27](https://github.com/AlexanderTsarkov/naviga-app/issues/27) | 2.9 Firmware integration test | Test | Open | N/A |
| [#28](https://github.com/AlexanderTsarkov/naviga-app/issues/28) | 3.1 App module structure | App | Open | N/A |
| [#29](https://github.com/AlexanderTsarkov/naviga-app/issues/29) | 3.2 BLE client + contract | App | Open | N/A |
| [#30](https://github.com/AlexanderTsarkov/naviga-app/issues/30) | 3.3 Domain model (Node/NodeTable) | App | Open | N/A |
| [#31](https://github.com/AlexanderTsarkov/naviga-app/issues/31) | 3.4 Node list screen | App | Open | N/A |
| [#32](https://github.com/AlexanderTsarkov/naviga-app/issues/32) | 3.5 Map + “me” | App | Open | N/A |
| [#33](https://github.com/AlexanderTsarkov/naviga-app/issues/33) | 3.6 Local log | App | Open | N/A |
| [#34](https://github.com/AlexanderTsarkov/naviga-app/issues/34) | 3.7 App integration/E2E test | App | Open | N/A |
| [#35](https://github.com/AlexanderTsarkov/naviga-app/issues/35) | 4.1 Field test 2–5 nodes | Test | Open | N/A |
| [#36](https://github.com/AlexanderTsarkov/naviga-app/issues/36) | 4.2 Results + doc updates | Docs | Open | N/A |
