# OOTB v0 Scope and Effort Estimate

**Purpose:** Translate completed work (day zero through current OOTB vertical slice) into measurable repo metrics and a realistic person-hour estimate for a normal professional team. Factual only; no code or behavior changes.

**Repo:** https://github.com/AlexanderTsarkov/naviga-app  
**Date:** 2026-02-06

---

## 1. Repository metrics (merged PRs)

Data source: GitHub API (`gh pr view`) for merged PRs; git for unique/new file counts.

### 1.1 Per-PR summary (selected)

| PR   | Title (short)                    | Additions | Deletions | Files | Commits |
|------|----------------------------------|-----------|-----------|-------|---------|
| 39   | Minimal CI + PlatformIO          | 168       | 0         | 7     | 2       |
| 40   | Bootstrap / restructure          | 2,939     | 1,349     | 164   | 28      |
| 41   | ADR Map SDK                      | 61        | 0         | 1     | 1       |
| 42   | POC E220 evidence                | 133       | 0         | 1     | 2       |
| 47   | OOTB #7 docs + English paths     | 1,842     | 480       | 34    | 5       |
| 48   | Architecture index              | 80        | 0         | 3     | 6       |
| 50   | HW profiles v0                   | 233       | 0         | 9     | 7       |
| 51   | Firmware skeleton (11.1)         | 121       | 10        | 10    | 2       |
| 52   | Node identity + NodeTable (11.2) | 285       | 0         | 5     | 5       |
| 53   | BLE DeviceInfo + pages (11.3)    | 232       | 10        | 5     | 6       |
| 54   | AppServices refactor             | 108       | 71        | 3     | 7       |
| 55   | GNSS stub + self position (11.4)| 293       | 0         | 8     | 1       |
| 56   | OOTB progress inventory          | 74        | 0         | 1     | 1       |
| 57   | HAL interfaces + mocks (#18)     | 465       | 32        | 16    | 6       |
| 58   | Inventory update                 | 9         | 8         | 2     | 1       |
| 59   | E220 radio HAL + OLED smoke      | 556       | 2         | 15    | 2       |
| 60   | Platform abstraction (#19 step 1)| 693       | 2         | 19    | 3       |
| 61   | Platform adapters (#19 step 2)   | 275       | 73        | 14    | 1       |
| 62   | CI guardrails (#19 step 3)       | 108       | 4         | 6     | 4       |
| 63   | GEO_BEACON codec (#23)           | 404       | 4         | 9     | 5       |
| 64   | Docs #23 done                    | 405       | 5         | 9     | 5       |
| 65   | Logging v0 (#20)                 | 565       | 19        | 14    | 2       |
| 67   | UART log drain limit             | 66        | 0         | 1     | 1       |
| 68   | Docs #12–#17 done                | 6         | 6         | 1     | 1       |
| 69   | IBleTransport + ESP32 (#22)      | 324       | 8         | 7     | 2       |
| 70   | Inventory #22                    | 1         | 1         | 1     | 1       |
| 71   | NodeTable domain (#24)           | 765       | 1         | 4     | 3       |
| 72   | Beacon logic (#25)               | 245       | 1         | 4     | 2       |
| 73   | BLE NodeTable bridge (#26)       | 341       | 3         | 7     | 2       |
| 75   | Native E2E test (#27)            | 172       | 1         | 4     | 3       |
| 77   | M1 runtime wiring (#74)         | 268       | 31        | 9     | 2       |
| 78   | Channel sense + send policy (#76)| 370       | 9         | 11    | 2       |

### 1.2 Totals across the repo (all merged PRs above)

| Metric                     | Value  | Note |
|----------------------------|--------|------|
| **Total insertions**       | ~12,600| Sum of PR additions (overlaps possible if same line touched in multiple PRs). |
| **Total deletions**        | ~2,100 | Sum of PR deletions. |
| **Unique files touched**   | 419    | `git log main --name-only` union. |
| **New files created**      | 288    | `git log main --diff-filter=A` union. |
| **Merged PRs (OOTB-relevant)** | 32 | From list; excludes non-merged. |

---

## 2. Breakdown by area (path-based)

Approximate mapping of current codebase and PR impact by path (no automated per-PR path split; based on repo layout and PR descriptions).

| Area | Paths | Approx. content |
|------|--------|------------------|
| **firmware/src/app** | `app/`, `app_services`, `m1_runtime`, `node_table` (app), `app.cpp` | Top-level init/tick, M1 wiring, legacy app NodeTable. |
| **firmware/src/domain** | `domain/node_table`, `beacon_logic`, `beacon_send_policy`, `logger`, `log_events` | Domain model, beacon cadence, send policy, event log. |
| **firmware/src/platform (HAL)** | `platform/e220_radio`, `ble_esp32_transport`, `ble_transport_core`, `arduino_*`, `device_id`, `timebase`, `log_export_uart`, `esp_device_id_provider` | E220 adapter, BLE GATT, platform adapters, timebase. |
| **firmware/protocol + src/protocol** | `protocol/geo_beacon_codec`, `ble_node_table_bridge`; `src/protocol/*` wrappers | GEO_BEACON codec, BLE bridge; wrappers for embedded build. |
| **firmware/lib/NavigaCore** | `hal/interfaces.h`, `hal/mocks/*` (radio, ble, channel_sense, gnss, log), `platform/*` | IRadio, IBleTransport, IChannelSense, mocks. |
| **firmware/test** | `test/test_*` (9 suites) | Unit + integration tests (test_native). |
| **firmware/hw_profiles** | `get_hw_profile`, `devkit_e220_oled*` | HW profile selection and pinout. |
| **firmware/services** | `ble_service`, `radio_smoke_service`, `gnss_stub_service`, `self_update_policy`, `oled_status` | Legacy BLE, smoke, GNSS stub, OLED. |
| **docs** | `docs/` (product, architecture, protocols, firmware, adr, project, dev, hardware) | Specs, ADRs, inventory, bench plans. |
| **mobile (app/)** | `app/` Flutter scaffold | Minimal; clean slate (lib/main.dart, platform stubs). OOTB UI not implemented. |

---

## 3. Codebase snapshot

| Category | Approx. LOC | Notes |
|----------|-------------|--------|
| **Firmware (src + protocol + lib)** | ~5,400 | C/C++ only; excludes `.pio`, external libs. |
| **Tests (firmware/test)** | ~1,070 | test_*.cpp. |
| **Docs (markdown)** | ~4,900 | 45 `.md` files under `docs/`. |

| Test suites (test_native) | Count |
|---------------------------|--------|
| test_geo_beacon_codec     | 1     |
| test_beacon_send_policy   | 1     |
| test_logger               | 1     |
| test_ble_transport_core   | 1     |
| test_ble_node_table_bridge| 1     |
| test_ootb_e2e_native      | 1     |
| test_beacon_logic         | 1     |
| test_node_table_domain    | 1     |
| test_hal_mocks            | 1     |
| **Total**                 | **9** |

---

## 4. Deliverables map

| Deliverable | PR(s) | Key paths |
|-------------|--------|-----------|
| Product/domain structure | 40, 47, 48, 50–55, 60–61 | `docs/architecture/`, `docs/product/`, `firmware/src/` layout, platform abstraction. |
| NodeTable (app-level + domain) | 52, 71 | `firmware/src/app/node_table.*`, `firmware/src/domain/node_table.*`, `firmware/test/test_node_table_domain/`. |
| BeaconLogic | 72 | `firmware/src/domain/beacon_logic.*`, `firmware/test/test_beacon_logic/`. |
| GEO_BEACON codec | 63 | `firmware/protocol/geo_beacon_codec.*`, `firmware/test/test_geo_beacon_codec/`. |
| BLE NodeTable protocol & bridge | 53, 69, 73 | `firmware/protocol/ble_node_table_bridge.*`, `firmware/src/platform/ble_esp32_transport.*`, `firmware/lib/NavigaCore` (IBleTransport), `firmware/test/test_ble_node_table_bridge/`, `test_ble_transport_core/`. |
| Radio abstraction (IRadio + E220) | 57, 59 | `firmware/lib/NavigaCore/include/naviga/hal/interfaces.h` (IRadio), `firmware/src/platform/e220_radio.*`. |
| M1Runtime wiring | 77 | `firmware/src/app/m1_runtime.*`, `firmware/src/app/app_services.cpp`, `firmware/src/protocol/` wrappers. |
| Beacon send policy + IChannelSense | 78 | `firmware/src/domain/beacon_send_policy.*`, `firmware/lib/NavigaCore` (IChannelSense, MockChannelSense), `firmware/test/test_beacon_send_policy/`. |
| Unit + integration tests | 57, 63, 69, 71–73, 75, 78 | All `firmware/test/test_*`; `pio test -e test_native`. |
| Bench & inventory docs | 56, 58, 64, 68, 70, 75, 78, etc. | `docs/project/ootb_progress_inventory.md`, `docs/firmware/stand_tests/issue_27_stand_report.md`, `issue_76_radio_channel_access.md`. |
| Logging v0 | 65, 67 | `firmware/src/domain/logger.*`, `log_events.h`, `firmware/src/platform/log_export_uart.*`. |
| HAL interfaces + mocks | 57 | `firmware/lib/NavigaCore/include/naviga/hal/interfaces.h`, `hal/mocks/*`. |

---

## 5. Effort estimation (person-hours)

Ranges are **LOW / MID / HIGH** for a normal professional team (one or two developers, familiar with embedded/C++ and the stack). Assumptions: scope = from initial product docs through current OOTB firmware vertical slice (no mobile UI implementation); estimates include design, implementation, testing, docs, and iteration.

| Category | LOW | MID | HIGH | Assumptions |
|----------|-----|-----|------|--------------|
| **(a) Product / requirements / acceptance** | 16 | 28 | 44 | Reading/refining initial ~5 product docs, OOTB plan, acceptance criteria, issue breakdown, prioritization. |
| **(b) Firmware development** | 80 | 120 | 180 | Skeleton, HAL, domain (NodeTable, BeaconLogic, policy), protocol (codec, bridge), platform (E220, BLE, adapters), M1 wiring. |
| **(c) Testing (unit + integration)** | 24 | 40 | 60 | Writing and maintaining 9 test suites, test_native env, E2E test, bench smoke runs. |
| **(d) Documentation** | 20 | 35 | 55 | Specs (Radio, BLE, NodeTable, HAL), ADRs, architecture index, POC evidence, inventory, stand test docs. |
| **(e) Debugging, CI, iteration & coordination** | 24 | 40 | 64 | CI setup, platform leak guardrails, fix cycles, merge/conflict resolution, code review. |
| **Total (range)** | **164** | **263** | **403** | Sum of mid columns; use LOW–HIGH as band. |

- **Explicit assumptions:** Single or small team; no formal PM; scope stops at “working OOTB firmware vertical slice” (beacon ↔ NodeTable ↔ BLE, tests, docs). POC/bootstrap and restructure (e.g. PR #40) counted as part of (a) and (e). Mobile app is minimal scaffold only.
- **No fake precision:** Ranges only; MID is a plausible central estimate.

---

## 6. What is NOT included

- **Mobile app UI:** OOTB list/map/log (issues #28–#34). App folder is present with minimal Flutter scaffold only.
- **SPI radio driver:** No LLCC68/SX126x or other direct SPI radio driver; E220 UART via library only.
- **CAD/LBT real implementation on hardware:** IChannelSense and send policy exist; E220 UART path is UNSUPPORTED for channel sense (documents).
- **Field tests (2–5 nodes):** Issue #35 and real outdoor/field validation not done.
- **Backend / web:** Not in scope for this estimate.
- **JOIN / Mesh / advanced MAC:** Out of OOTB v0 scope.

---

## 7. References

- **Inventory:** [docs/project/ootb_progress_inventory.md](ootb_progress_inventory.md)  
- **Workmap:** [docs/project/ootb_workmap.md](ootb_workmap.md)  
- **E220 implementation status:** [docs/firmware/e220_radio_implementation_status.md](../firmware/e220_radio_implementation_status.md)
