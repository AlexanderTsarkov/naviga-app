# Naviga — Architecture Index

## A) Purpose
Этот документ — навигация по архитектуре Naviga и “source of truth” для ключевых решений.
Используйте его как карту: быстро найти главный документ по теме и связанные спецификации.
Если информация не определена, ищите пометку **Future/TBD** в таблице ниже.
Новые документы должны ссылаться на этот индекс и не противоречить ADR.
При конфликте решений приоритет у ADR и его ссылок из этого индекса.
Индекс обновляется вместе с изменениями архитектуры и протоколов.

## B) Layer map (high-level)
- **Firmware (ESP32)**
  - Ответственность: GNSS → beacon cadence → GEO_BEACON TX/RX → NodeTable → BLE snapshot/telemetry.
  - Ключевые модули/слои: **Domain** (NodeTable, cadence), **Protocol** (GEO_BEACON, BLE payloads), **HAL** (IRadio/IGnss/IBleTransport/ILog), **Scheduler/tick** (GNSSTask/RadioRxTask/BeaconTxTask/BleTask). Радио драйвер: Issue [#21](https://github.com/AlexanderTsarkov/naviga-app/issues/21).
- **Mobile app**
  - Ответственность: BLE-клиент, список нод, карта/“я”, базовый лог.
  - Ключевые экраны/флоу: **BLE connect + DeviceInfo**, **Node list** (seed), **Map/Me** (planned), **Log** (seed minimal).
- **Protocols**
  - **Radio:** GEO_BEACON формат, cadence, public/private channel semantics, contention handling.
  - **BLE:** DeviceInfo, Health, NodeTableSnapshot (read-only в seed), совместимость форматов.
- **Future placeholders**
  - **Backend/Web/Tools:** TBD; см. `docs/backend/README.md`, `docs/web/README.md`, `tools/`.

## C) “Source of Truth” table
| Topic | Primary doc | Related docs/ADRs | Status |
|---|---|---|---|
| Product vision & core | [naviga_product_core.md](../product/naviga_product_core.md) | [naviga_vision_decision_edition.md](../product/naviga_vision_decision_edition.md), [naviga_ootb_and_activation_scenarios.md](../product/naviga_ootb_and_activation_scenarios.md) | Seed |
| OOTB scope + workmap | [ootb_scope_v0.md](../product/ootb_scope_v0.md) | [ootb_workmap.md](../../_archive/ootb_v1/project/ootb_workmap.md), [OOTB_v0_analysis_and_plan.md](../product/OOTB_v0_analysis_and_plan.md) | Seed |
| Radio: band strategy | [radio_band_strategy_v0.md](../adr/radio_band_strategy_v0.md) | [radio_profile_presets_v0.md](../protocols/radio_profile_presets_v0.md) | Seed |
| Radio: preset (OOTB public) | [ootb_radio_preset_v0.md](../product/ootb_radio_preset_v0.md) | [radio_profile_presets_v0.md](../protocols/radio_profile_presets_v0.md), [radio_channel_mapping_v0.md](../product/radio_channel_mapping_v0.md) | Seed |
| Radio: channel mapping | [radio_channel_mapping_v0.md](../product/radio_channel_mapping_v0.md) | [ootb_radio_preset_v0.md](../product/ootb_radio_preset_v0.md) | Seed |
| Radio protocol (GEO_BEACON) | [ootb_radio_v0.md](../protocols/ootb_radio_v0.md) | [radio_profile_presets_v0.md](../protocols/radio_profile_presets_v0.md), [ootb_firmware_arch_v0.md](../firmware/ootb_firmware_arch_v0.md) | Seed |
| BLE protocol (GATT, NodeTableSnapshot) | [ootb_ble_v0.md](../protocols/ootb_ble_v0.md) | [ootb_node_table_v0.md](../firmware/ootb_node_table_v0.md), [hal_contracts_v0.md](../firmware/hal_contracts_v0.md) | Seed |
| Firmware architecture (tick/timebase) | [ootb_firmware_arch_v0.md](../firmware/ootb_firmware_arch_v0.md) | [hal_contracts_v0.md](../firmware/hal_contracts_v0.md), [ootb_scope_v0.md](../product/ootb_scope_v0.md) | Seed |
| HAL contracts | [hal_contracts_v0.md](../firmware/hal_contracts_v0.md) | [gnss_v0.md](../firmware/gnss_v0.md), [ootb_firmware_arch_v0.md](../firmware/ootb_firmware_arch_v0.md) | Seed |
| GNSS (stub now, real later) | [gnss_stub_v0.md](../firmware/gnss_stub_v0.md) | [gnss_v0.md](../firmware/gnss_v0.md), [ootb_position_source_v0.md](../adr/ootb_position_source_v0.md) | Seed |
| Node identity + short_id | [ootb_node_table_v0.md](../firmware/ootb_node_table_v0.md) | [ootb_ble_v0.md](../protocols/ootb_ble_v0.md) | Seed |
| NodeTable spec | [ootb_node_table_v0.md](../firmware/ootb_node_table_v0.md) | [ootb_ble_v0.md](../protocols/ootb_ble_v0.md) | Seed |
| Logging spec | [logging_v0.md](../product/logging_v0.md) | [hal_contracts_v0.md](../firmware/hal_contracts_v0.md), [ootb_test_plan_v0.md](../product/ootb_test_plan_v0.md) | Seed |
| Mesh protocol concept | [naviga_mesh_protocol_concept_v1_4.md](../product/naviga_mesh_protocol_concept_v1_4.md) | [naviga_join_session_logic_v1_2.md](../product/naviga_join_session_logic_v1_2.md) | Future |
| Join / session logic | [naviga_join_session_logic_v1_2.md](../product/naviga_join_session_logic_v1_2.md) | [naviga_mesh_protocol_concept_v1_4.md](../product/naviga_mesh_protocol_concept_v1_4.md) | Future |
| Test plan / field tests | [ootb_test_plan_v0.md](../product/ootb_test_plan_v0.md) | [poc_e220_evidence.md](../firmware/poc_e220_evidence.md), [poc_gaps_risks.md](../product/poc_gaps_risks.md) | Seed |
| ADR index / ADR rules | [adr/README.md](../adr/README.md) | [ootb_workmap.md](../../_archive/ootb_v1/project/ootb_workmap.md) | Seed |

## D) Key runtime flows (short diagrams)
**OOTB seed end-to-end (data path):**  
`GNSS snapshot → Beacon cadence decision → GEO_BEACON TX/RX → NodeTable update → BLE NodeTableSnapshot → App list`

**Join/session (future):**  
`Join/Session logic → short_id assignment → role params → Mesh routing`  
See: [naviga_join_session_logic_v1_2.md](../product/naviga_join_session_logic_v1_2.md)

**Mesh (future):**  
`GEO packets → covered_mask/best_mask → relay/forwarding rules`  
See: [naviga_mesh_protocol_concept_v1_4.md](../product/naviga_mesh_protocol_concept_v1_4.md)

## E) Repo layout (docs and code)
- `docs/` — архитектура, продукт, протоколы, firmware, mobile, ADR
- `docs/architecture/` — этот индекс и архитектурные карты
- `docs/product/` — продуктовые решения, scope, тест-план
- `docs/protocols/` — wire-контракты (Radio, BLE, Join, Mesh)
- `docs/firmware/` — архитектура и контракты прошивки (NodeTable, HAL, GNSS)
- `docs/mobile/` — решения по app (SDK, UI/flows)
- `docs/adr/` — решения/ADR
- `app/` — mobile app (Flutter)
- `firmware/` — прошивка
- `backend/`, `web/` — Future/TBD
- `tools/` — скрипты и утилиты

## F) Conventions + governance
- **Filenames:** English-only, `snake_case`, no spaces, no Cyrillic.
- **Timebase:** внутри использовать `uptime_ms`; наружу (BLE/UI/logs) отдавать **age** (`*_age_s`), не wall-clock.
- **Packet philosophy:** радио-пакеты частые и **минимальные** по размеру; расширения — **опциональны**.
- **Public vs Private:** `public_channel_id = 1` зарезервирован под OOTB Public.
- **ADR governance rule:** если документ меняет ранее принятое решение, он **обязан** ссылаться на ADR (новый или обновлённый). ADR — источник истины по решениям; Architecture Index на них ссылается.
- **ADR change rule (PR):** любой PR, который добавляет/обновляет ADR, **обязан** обновить этот Architecture Index в том же PR.
