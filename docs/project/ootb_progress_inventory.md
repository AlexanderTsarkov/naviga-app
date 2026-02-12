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

## F) Mobile v1 (Android-first, Flutter)

**Epic:** [#80](https://github.com/AlexanderTsarkov/naviga-app/issues/80)  
**Spec:** [docs/mobile/mobile_v1_screens.md](../mobile/mobile_v1_screens.md)

**Maintenance rule:** Every merged PR that delivers Mobile v1 work (Epic #80 / issues #81–#90) must update the tracking block below (status, PR link, key paths, verify).

### F.1) Mobile v1 tracking

| # | Issue | Status | PR | Key paths | Verify | Notes |
|---|-------|--------|-----|-----------|--------|-------|
| 81 | [#81](https://github.com/AlexanderTsarkov/naviga-app/issues/81) Flutter app bootstrap + module structure | **Done** | [#92](https://github.com/AlexanderTsarkov/naviga-app/pull/92) | `app/lib/app/`, `app/lib/features/*` (placeholders), `app/lib/shared/` | `flutter analyze`; `flutter test`; `flutter run` on Android (minSdk 28) | CI lint runs in `app/`; format check: `dart format .` in `app/` |
| 82 | [#82](https://github.com/AlexanderTsarkov/naviga-app/issues/82) Android permissions + BLE scan | **Done** | [#94](https://github.com/AlexanderTsarkov/naviga-app/pull/94) | `app/lib/features/connect/` (connect_controller, connect_screen) | `flutter analyze`; `flutter test`; run on device: readiness (BT/Location/perm), scan lists Naviga, tap → "Connection not supported yet" | Manual verification: grant CTA → system prompt; Location OFF → open settings; scan lists Naviga dongle; connect not in scope (#83). |
| 83 | [#83](https://github.com/AlexanderTsarkov/naviga-app/issues/83) BLE connection state machine | **Done** | [#95](https://github.com/AlexanderTsarkov/naviga-app/pull/95) | `app/lib/features/connect/` (connect_controller, connect_screen) | `flutter analyze`; `flutter test`; run on device: connect A→disconnect→B, A→B, power off→idle | Manual: connect/disconnect + switch devices OK; power-off→idle after ~2s. Known: device list scroll area too small (fix later). |
| 84 | [#84](https://github.com/AlexanderTsarkov/naviga-app/issues/84) BLE read DeviceInfo (Health deferred) | **Done** | [#96](https://github.com/AlexanderTsarkov/naviga-app/pull/96) | `app/lib/features/connect/` | `flutter analyze`; `flutter test`; manual S8: connect → DeviceInfo telemetry or "Telemetry not available…", disconnect clears | — |
| 85 | [#85](https://github.com/AlexanderTsarkov/naviga-app/issues/85) NodeTableSnapshot paging + DTO decode | **Done** | [#105](https://github.com/AlexanderTsarkov/naviga-app/pull/105) | `app/lib/features/connect/connect_controller.dart`, `app/test/node_table_parser_test.dart` | `flutter analyze`; `flutter test`; S8: set `kDebugFetchNodeTableOnConnect=true`, `flutter run`, connect → log `NodeTable: snapshot=<id> totalNodes=2 pages=1 records=2 collisions=0`, reconnect → new snapshot, set flag back to `false` | No UI changes |
| 86 | [#86](https://github.com/AlexanderTsarkov/naviga-app/issues/86) Repository + NodeTable state | **Done** | [#107](https://github.com/AlexanderTsarkov/naviga-app/pull/107) | `app/lib/features/nodes/` (repository, state, controller) | `flutter analyze`; `flutter test`; S8: kDebugFetchNodeTableOnConnect=true, connect → log `Nodes: refresh start` / `Nodes: refresh done records=2 self=true snapshot=8548`, reconnect → snapshot=8716 | No UI changes |
| 87 | [#87](https://github.com/AlexanderTsarkov/naviga-app/issues/87) Local cache last NodeTable snapshot | **Done** | [#109](https://github.com/AlexanderTsarkov/naviga-app/pull/109) | `app/lib/features/nodes/node_table_cache.dart`, nodes_controller, nodes_state; SharedPreferences `naviga.node_table_cache.<deviceId>`; TTL 10 min | `flutter analyze`; `flutter test`; S8: `kDebugFetchNodeTableOnConnect=true`, connect → NodesCache save/restore logs; relaunch → restore before reconnect; then flag `false` | No UI changes. S8 logs: `NodesCache: expired deviceId=9C:13:9E:AB:BA:A1 ageMs=1210607 ttlMs=600000`; `NodesCache: save deviceId=9C:13:9E:AB:BA:A1 snapshot=31935 records=2`; `NodesCache: save deviceId=3C:DC:75:6F:23:BD snapshot=32033 records=2`; `NodesCache: restore deviceId=3C:DC:75:6F:23:BD snapshot=32033 records=2 ageMs=150316`. |
| 88 | [#88](https://github.com/AlexanderTsarkov/naviga-app/issues/88) Core UI screens (Connect/My Node/Nodes/Details) | Planned | — | `app/lib/features/`, `app/lib/shared/` | analyze; test; run (screens + data) | — |
| 89 | [#89](https://github.com/AlexanderTsarkov/naviga-app/issues/89) Map v1 (flutter_map + markers) | Planned | — | `app/lib/features/map/` | analyze; test; run (markers) | Online tiles only (no offline) |
| 90 | [#90](https://github.com/AlexanderTsarkov/naviga-app/issues/90) Settings v1 (about + diagnostics) | Planned | — | `app/lib/features/settings/` | analyze; test; run | No BLE config in v1 |

**Definition of Done (planning):**
- Epic + all child issues created and on Project board
- Old mobile issues (#28–#34) closed as replaced
- Spec doc approved as baseline (data scope = BLE v0 read-only)

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
| [#22](https://github.com/AlexanderTsarkov/naviga-app/issues/22) | 2.4 HAL BLE transport | Code | Done | PR [#69](https://github.com/AlexanderTsarkov/naviga-app/pull/69) — BLE transport core + ESP32 IBleTransport adapter; test_native unit tests. |
| [#23](https://github.com/AlexanderTsarkov/naviga-app/issues/23) | 2.5 GEO_BEACON codec | Code | Done | PR [#63](https://github.com/AlexanderTsarkov/naviga-app/pull/63). GEO_BEACON codec: fixed 24-byte core, little-endian; decode ignores trailing bytes; pos_valid==0 skips range-check (fields parsed as-is); unit tests (test_native). |
| [#24](https://github.com/AlexanderTsarkov/naviga-app/issues/24) | 2.6 NodeTable domain | Code | Done | PR [#71](https://github.com/AlexanderTsarkov/naviga-app/pull/71) — `firmware/src/domain/node_table.{h,cpp}`, `firmware/test/test_node_table_domain/test_node_table_domain.cpp` |
| [#25](https://github.com/AlexanderTsarkov/naviga-app/issues/25) | 2.7 Beacon logic | Code | Done | PR [#72](https://github.com/AlexanderTsarkov/naviga-app/pull/72) — `firmware/src/domain/beacon_logic.{h,cpp}`, `firmware/test/test_beacon_logic/test_beacon_logic.cpp` |
| [#26](https://github.com/AlexanderTsarkov/naviga-app/issues/26) | 2.8 BLE NodeTable bridge | Code | Done | PR [#73](https://github.com/AlexanderTsarkov/naviga-app/pull/73) — `firmware/protocol/ble_node_table_bridge.{h,cpp}`, `firmware/test/test_ble_node_table_bridge/test_ble_node_table_bridge.cpp` |
| [#27](https://github.com/AlexanderTsarkov/naviga-app/issues/27) | 2.9 Firmware integration test | Test | Done | PR [#75](https://github.com/AlexanderTsarkov/naviga-app/pull/75) — `firmware/test/test_ootb_e2e_native/test_ootb_e2e_native.cpp`, `docs/firmware/stand_tests/issue_27_stand_report.md` |
| [#28](https://github.com/AlexanderTsarkov/naviga-app/issues/28) | 3.1 App module structure | App | Open | N/A |
| [#29](https://github.com/AlexanderTsarkov/naviga-app/issues/29) | 3.2 BLE client + contract | App | Open | N/A |
| [#30](https://github.com/AlexanderTsarkov/naviga-app/issues/30) | 3.3 Domain model (Node/NodeTable) | App | Open | N/A |
| [#31](https://github.com/AlexanderTsarkov/naviga-app/issues/31) | 3.4 Node list screen | App | Open | N/A |
| [#32](https://github.com/AlexanderTsarkov/naviga-app/issues/32) | 3.5 Map + “me” | App | Open | N/A |
| [#33](https://github.com/AlexanderTsarkov/naviga-app/issues/33) | 3.6 Local log | App | Open | N/A |
| [#34](https://github.com/AlexanderTsarkov/naviga-app/issues/34) | 3.7 App integration/E2E test | App | Open | N/A |
| [#35](https://github.com/AlexanderTsarkov/naviga-app/issues/35) | 4.1 Field test 2–5 nodes | Test | Open | N/A |
| [#36](https://github.com/AlexanderTsarkov/naviga-app/issues/36) | 4.2 Results + doc updates | Docs | Open | N/A |
| [#74](https://github.com/AlexanderTsarkov/naviga-app/issues/74) | M1 Runtime wiring (GEO_BEACON ↔ NodeTable ↔ BLE) | Firmware | Done | PR [#77](https://github.com/AlexanderTsarkov/naviga-app/pull/77) — `firmware/src/app/m1_runtime.{h,cpp}`, `firmware/src/app/app_services.cpp`, `firmware/src/protocol/ble_node_table_bridge.cpp`, `firmware/src/protocol/geo_beacon_codec.cpp` |
| [#76](https://github.com/AlexanderTsarkov/naviga-app/issues/76) | Radio channel access + collision mitigation | Firmware | Done | PR [#78](https://github.com/AlexanderTsarkov/naviga-app/pull/78) — `firmware/src/domain/beacon_send_policy.{h,cpp}`, `firmware/src/app/m1_runtime.{h,cpp}`, `firmware/lib/NavigaCore/include/naviga/hal/interfaces.h`, `docs/firmware/stand_tests/issue_76_radio_channel_access.md` |