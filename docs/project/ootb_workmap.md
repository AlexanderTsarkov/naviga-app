# Naviga / OOTB v0 — карта работ (Workmap)

Один документ: план, прогресс, границы scope. Issues [#2](https://github.com/AlexanderTsarkov/naviga-app/issues/2)–[#36](https://github.com/AlexanderTsarkov/naviga-app/issues/36).
Architecture Index: [docs/architecture/index.md](../architecture/index.md).

---

## 1. OOTB v0: цель и не-цели

**Цель:** Работающий фундамент «из коробки»: донгл шлёт/принимает GEO_BEACON, ведёт NodeTable, отдаёт данные в приложение по BLE; приложение показывает список нод, карту, «я», минимальный лог. Полевой тест 2–5 нод.

**Не-цели (вне scope OOTB v0):**
- JOIN, Session Master, short_id, anchor, роли
- Mesh (covered_mask, best_mask, ретрансляции, QoD)
- Сценарий «Охота» в UX, сессии, backend
- Оффлайн-карты в v0 (решение по Map SDK — с прицелом на оффлайн позже)

**Принятые решения (ADR):**
- [Position source (GNSS на донгле)](../adr/ootb_position_source_v0.md) — primary position = dongle GNSS; stub допускается до появления железа.
- **Radio band & defaults (DECIDED):** [Radio band strategy + OOTB public profile](../adr/radio_band_strategy_v0.md) — OOTB band = 433 MHz; no dual-band; default power = module_min; Channel + Power (+ SpeedPreset) в абстракциях; Public OOTB profile и «Back to public network». Issue [#45](https://github.com/AlexanderTsarkov/naviga-app/issues/45).

---

## 2. Current status (сделано)

| Issue | Артефакт / ссылка |
|-------|-------------------|
| [#3](https://github.com/AlexanderTsarkov/naviga-app/issues/3) | GitHub Project под OOTB v0 |
| [#4](https://github.com/AlexanderTsarkov/naviga-app/issues/4) | Epic «OOTB End-to-End v0» |
| [#5](https://github.com/AlexanderTsarkov/naviga-app/issues/5) | [.github/workflows/](https://github.com/AlexanderTsarkov/naviga-app/tree/main/.github/workflows) — CI firmware, mobile, lint |
| [#6](https://github.com/AlexanderTsarkov/naviga-app/issues/6) | [docs/mobile/map_sdk_decision.md](../mobile/map_sdk_decision.md) — ADR Map SDK (flutter_map) |
| [#7](https://github.com/AlexanderTsarkov/naviga-app/issues/7) | [docs/firmware/poc_e220_evidence.md](../firmware/poc_e220_evidence.md) — POC E220 evidence |
| — | [docs/adr/ootb_position_source_v0.md](../adr/ootb_position_source_v0.md) — ADR position source (GNSS на донгле) |
| — | [docs/firmware/gnss_v0.md](../firmware/gnss_v0.md), [docs/firmware/hal_contracts_v0.md](../firmware/hal_contracts_v0.md) (IGnss) — GNSS v0 contract + HAL |
| — | [docs/adr/radio_band_strategy_v0.md](../adr/radio_band_strategy_v0.md) — ADR radio band + OOTB public profile defaults |

**Опорные доки в репо:** [docs/product/OOTB_v0_analysis_and_plan.md](../product/OOTB_v0_analysis_and_plan.md), [docs/product/naviga_product_core.md](../product/naviga_product_core.md).

---

## 3. Таблица: Issues #2–#36

| # | Issue | Заголовок | Phase | Component | Status | Output artifact | Dependencies / Blockers |
|---|-------|-----------|-------|-----------|--------|-----------------|-------------------------|
| 2 | [#2](https://github.com/AlexanderTsarkov/naviga-app/issues/2) | Epic: OOTB End-to-End v0 | — | docs | Todo | — | — |
| 3 | [#3](https://github.com/AlexanderTsarkov/naviga-app/issues/3) | 0.1 Настроить GitHub Project под OOTB v0 | 0 | infra | Done | Project board | — |
| 4 | [#4](https://github.com/AlexanderTsarkov/naviga-app/issues/4) | 0.2 Завести Epic «OOTB End-to-End v0» | 0 | docs | Done | Epic #2 | — |
| 5 | [#5](https://github.com/AlexanderTsarkov/naviga-app/issues/5) | 0.3 Минимальный CI (гейты) | 0 | infra | Done | .github/workflows/ | — |
| 6 | [#6](https://github.com/AlexanderTsarkov/naviga-app/issues/6) | 0.4 Решение по Map SDK (offline-friendly) | 0 | docs | Done | docs/mobile/map_sdk_decision.md | Блокирует 3.5 (карта) |
| 7 | [#7](https://github.com/AlexanderTsarkov/naviga-app/issues/7) | 0.5.1 Документ POC Evidence | 0.5 | docs | Done | docs/firmware/poc_e220_evidence.md | — |
| 8 | [#8](https://github.com/AlexanderTsarkov/naviga-app/issues/8) | 0.5.2 Список пробелов/рисков POC | 0.5 | docs | Todo | doc в docs/firmware или product | — |
| 9 | [#9](https://github.com/AlexanderTsarkov/naviga-app/issues/9) | 0.5.3 Навести порядок в архиве | 0.5 | infra | Todo | archive/ + README | — |
| 10 | [#10](https://github.com/AlexanderTsarkov/naviga-app/issues/10) | 1.1 Architecture Index | 1 | docs | Todo | docs/architecture/ | — |
| 11 | [#11](https://github.com/AlexanderTsarkov/naviga-app/issues/11) | 1.2 Архитектура firmware: слои и границы | 1 | docs | Todo | docs/firmware/ootb_firmware_arch_v0.md | — |
| 12 | [#12](https://github.com/AlexanderTsarkov/naviga-app/issues/12) | 1.3 Спецификация NodeTable | 1 | docs | Todo | docs/firmware/ootb_node_table_v0.md | — |
| 13 | [#13](https://github.com/AlexanderTsarkov/naviga-app/issues/13) | 1.4 Абстракции железа (HAL) | 1 | docs | Todo | docs/firmware/hal_contracts_v0.md | — |
| 14 | [#14](https://github.com/AlexanderTsarkov/naviga-app/issues/14) | 1.5 OOTB Radio v0 — спецификация | 1 | docs | Todo | docs/protocols/ootb_radio_v0.md | — |
| 15 | [#15](https://github.com/AlexanderTsarkov/naviga-app/issues/15) | 1.6 OOTB BLE v0 — спецификация | 1 | docs | Todo | docs/protocols/ootb_ble_v0.md | — |
| 16 | [#16](https://github.com/AlexanderTsarkov/naviga-app/issues/16) | 1.7 Test Plan v0 | 1 | docs | Todo | docs/product/ootb_test_plan_v0.md | — |
| 17 | [#17](https://github.com/AlexanderTsarkov/naviga-app/issues/17) | 1.8 ADR: границы OOTB v0 | 1 | docs | Todo | docs/adr/ootb_scope_v0.md | — |
| 18 | [#18](https://github.com/AlexanderTsarkov/naviga-app/issues/18) | 2.0 HAL interfaces + mock implementations | 2 | firmware | Todo | firmware/ (interfaces + mocks) | 1.4 (HAL contracts) |
| 19 | [#19](https://github.com/AlexanderTsarkov/naviga-app/issues/19) | 2.1 Дерево модулей firmware по слоям | 2 | firmware | Todo | firmware/ README, domain/, protocol/, hal/ | 1.2 |
| 20 | [#20](https://github.com/AlexanderTsarkov/naviga-app/issues/20) | 2.2 Logging v0 (ring-buffer + export) | 2 | firmware | Todo | firmware/ | 2.0 |
| 21 | [#21](https://github.com/AlexanderTsarkov/naviga-app/issues/21) | 2.3 HAL радио (реальный драйвер) | 2 | firmware | Todo | firmware/hal/ | 2.0, 1.4, POC evidence |
| 22 | [#22](https://github.com/AlexanderTsarkov/naviga-app/issues/22) | 2.4 HAL BLE (реальный транспорт) | 2 | firmware | Todo | firmware/hal/ | 2.0, 1.4 |
| 23 | [#23](https://github.com/AlexanderTsarkov/naviga-app/issues/23) | 2.5 Протокол GEO_BEACON (кодер/декодер) | 2 | firmware | Todo | firmware/protocol/ | 1.5 (Radio v0 spec) |
| 24 | [#24](https://github.com/AlexanderTsarkov/naviga-app/issues/24) | 2.6 NodeTable (domain) | 2 | firmware | Todo | firmware/domain/ | 1.3 (NodeTable spec) |
| 25 | [#25](https://github.com/AlexanderTsarkov/naviga-app/issues/25) | 2.7 Логика beacon (domain) | 2 | firmware | Todo | firmware/domain/ | 2.5, 2.6, 2.3, 2.4 |
| 26 | [#26](https://github.com/AlexanderTsarkov/naviga-app/issues/26) | 2.8 BLE-мост NodeTable (protocol + hal) | 2 | firmware | Todo | firmware/ | 1.6 (BLE v0 spec), 2.4, 2.6 |
| 27 | [#27](https://github.com/AlexanderTsarkov/naviga-app/issues/27) | 2.9 Интеграция и тест firmware на стенде | 2 | firmware | Todo | — | 2.1–2.8 |
| 28 | [#28](https://github.com/AlexanderTsarkov/naviga-app/issues/28) | 3.1 Структура модулей app | 3 | app | Todo | app/ README, слои | 1.1 (Arch Index) |
| 29 | [#29](https://github.com/AlexanderTsarkov/naviga-app/issues/29) | 3.2 BLE-клиент и контракт с firmware | 3 | app | Todo | app/ | 1.6 (BLE v0 spec) |
| 30 | [#30](https://github.com/AlexanderTsarkov/naviga-app/issues/30) | 3.3 Доменная модель (Node, NodeTable) | 3 | app | Todo | app/ | — |
| 31 | [#31](https://github.com/AlexanderTsarkov/naviga-app/issues/31) | 3.4 Экран списка нод | 3 | app | Todo | app/ | 3.2, 3.3 |
| 32 | [#32](https://github.com/AlexanderTsarkov/naviga-app/issues/32) | 3.5 Карта и «я» | 3 | app | Todo | app/ | #6 (Map SDK ADR) |
| 33 | [#33](https://github.com/AlexanderTsarkov/naviga-app/issues/33) | 3.6 Базовый локальный лог | 3 | app | Todo | app/ | 3.2 |
| 34 | [#34](https://github.com/AlexanderTsarkov/naviga-app/issues/34) | 3.7 Интеграция и E2E тест mobile app | 3 | app | Todo | — | 3.1–3.6, 2.9 |
| 35 | [#35](https://github.com/AlexanderTsarkov/naviga-app/issues/35) | 4.1 Полевой тест 2–5 нод | 4 | — | Todo | — | 1.7 (Test Plan), 2.9, 3.7 |
| 36 | [#36](https://github.com/AlexanderTsarkov/naviga-app/issues/36) | 4.2 Фиксация результатов и обновление доков | 4 | docs | Todo | Test Plan, при необходимости spec/ADR | 4.1 |

*Phase: 0 = подготовка, 0.5 = POC evidence/архив, 1 = M0 доки/архитектура, 2 = firmware, 3 = app, 4 = полевой тест и закрытие.*

### 3.1. Issues added after initial plan

| # | Issue | Заголовок | Phase | Component | Type | Output artifact | Related |
|---|-------|-----------|-------|-----------|------|-----------------|--------|
| 43 | [#43](https://github.com/AlexanderTsarkov/naviga-app/issues/43) | GNSS v0: IGnss contract + stub policy + no-fix behavior | 1 (docs) → 2 (impl) | firmware + docs | Doc | [docs/firmware/gnss_v0.md](../firmware/gnss_v0.md), update [hal_contracts_v0.md](../firmware/hal_contracts_v0.md) | [#13](https://github.com/AlexanderTsarkov/naviga-app/issues/13), [#14](https://github.com/AlexanderTsarkov/naviga-app/issues/14), [#16](https://github.com/AlexanderTsarkov/naviga-app/issues/16), [#17](https://github.com/AlexanderTsarkov/naviga-app/issues/17) |
| 44 | [#44](https://github.com/AlexanderTsarkov/naviga-app/issues/44) | Radio presets mapping research: UART↔SPI compatibility (channel/speed/power) | 1 (docs/research) | docs/protocols + firmware | Doc / Research | update [radio_profile_presets_v0.md](../protocols/radio_profile_presets_v0.md) | [#13](https://github.com/AlexanderTsarkov/naviga-app/issues/13), [#14](https://github.com/AlexanderTsarkov/naviga-app/issues/14) |
| 45 | [#45](https://github.com/AlexanderTsarkov/naviga-app/issues/45) | ADR: Radio band strategy + OOTB public profile defaults | 1 | docs/adr | Doc | [docs/adr/radio_band_strategy_v0.md](../adr/radio_band_strategy_v0.md) | [#14](https://github.com/AlexanderTsarkov/naviga-app/issues/14), [#15](https://github.com/AlexanderTsarkov/naviga-app/issues/15) |

**GNSS (position source):** решение зафиксировано в [ADR ootb_position_source_v0](../adr/ootb_position_source_v0.md); контракт и stub — в [gnss_v0.md](../firmware/gnss_v0.md). Пробел POC «источник координат / GNSS» имеет владельца (issue #43).

**Radio band & defaults:** решение **DECIDED** — [ADR radio_band_strategy_v0](../adr/radio_band_strategy_v0.md) (OOTB band=433, no dual-band, default power=module_min). Отдельный issue для трекинга ADR: [#45](https://github.com/AlexanderTsarkov/naviga-app/issues/45). Выбор band/канала больше не в assumptions — перенесён в Decided.

**Mapping UART↔SPI:** маппинг channel_id/speed_preset_id/power между UART (E22/E220) и будущими SPI модулями имеет владельца — отдельный тикет [#44](https://github.com/AlexanderTsarkov/naviga-app/issues/44) (research + обновление radio_profile_presets_v0.md).

---

## 4. Future / Deferred (not in seed sprint 1)

| Тема | Issue | Примечание |
|------|-------|------------|
| **Battery v0: measurement (mV) + reporting + power budget** | [#46](https://github.com/AlexanderTsarkov/naviga-app/issues/46) | Seed: USB only, battery_mv N/A; см. [ootb_gap_analysis_v0.md](../product/ootb_gap_analysis_v0.md). |
| **Device ownership & pairing model (claiming, permissions, fleet mode)** | TBD (Phase 2+, Doc/Design) | Seed: out of scope; см. [ootb_gap_analysis_v0.md § Device ownership](../product/ootb_gap_analysis_v0.md#device-ownership--access-control--pairing). Related: BLE spec [#15](https://github.com/AlexanderTsarkov/naviga-app/issues/15), app settings [#31](https://github.com/AlexanderTsarkov/naviga-app/issues/31)/[#32](https://github.com/AlexanderTsarkov/naviga-app/issues/32). |

### 4.1. Issue to create (optional): Device ownership & pairing

**Title:** Device ownership & pairing model (claiming, permissions, fleet mode)  
**Type:** Doc/Design  
**Phase:** 2+  
**Artifacts:** TBD (ADR + spec)  
**Related / Link:** BLE spec [#15](https://github.com/AlexanderTsarkov/naviga-app/issues/15), app settings [#31](https://github.com/AlexanderTsarkov/naviga-app/issues/31)/[#32](https://github.com/AlexanderTsarkov/naviga-app/issues/32).  
**Description:** См. [ootb_gap_analysis_v0.md § Device ownership](../product/ootb_gap_analysis_v0.md#device-ownership--access-control--pairing) (who can connect/change settings; claiming; dongle+collar pairing; fleet admin vs user).

### 4.2. Issue to create (optional): ChannelId→CH mapping for 433 band

**Title:** Define ChannelId→CH mapping for 433 band (E220/E22 UART compatibility)  
**Type:** Doc/Design  
**Artifacts:** [radio_channel_mapping_v0.md](../product/radio_channel_mapping_v0.md) (created).  
**Related / Link:** [OOTB Radio preset v0](../product/ootb_radio_preset_v0.md), BLE settings / DeviceInfo ([ootb_ble_v0.md § 1.1](../protocols/ootb_ble_v0.md#11-deviceinfo-characteristic)).  
**Description:** Продуктовый ChannelId (1..N) → module CH для 433 MHz; anchor ChannelId=1 → CH=0x17 (433.125 MHz); единый маппинг для E220 и E22; max N по диапазону CH модуля.

---

## 5. Правила, чтобы не расползалось

- **Делать только DoD текущего тикета** — без «заодно» и расширения scope в рамках одного PR.
- **Соседние задачи — отдельно:** если всплыло что-то из другого issue (например, решение по формату ID), завести отдельный issue или заметку, не делать в рамках текущего тикета.
- **Решения фиксировать:** любое архитектурное/продуктовое решение — ссылкой на ADR или док (docs/adr/, docs/mobile/, docs/firmware/, docs/product/).
- **Порядок фаз:** 0 → 0.5 → 1 → 2 → 3 → 4; внутри Phase 2 — HAL+mocks → logging → реальные HAL → кодеки → domain → BLE-мост → стенд.
- **No guessing:** если в тикете нужен выбор (параметры, форматы) — в описании issue должны быть блоки «Decisions needed» и «Defaults», иначе не угадывать.
- **Один файл — одна задача:** модули и файлы с чёткой ответственностью; границы по [OOTB plan](../product/OOTB_v0_analysis_and_plan.md) и Architecture Index (после #10).
