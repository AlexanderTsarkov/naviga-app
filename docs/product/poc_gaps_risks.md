# POC gaps & risks (artifact)

**Purpose:** Сводка пробелов и рисков по POC, артефакты и ссылки для Issue [#7](https://github.com/AlexanderTsarkov/naviga-app/issues/7) и связанных (в т.ч. #8). Документация — не код.

**Related:** [POC E220 evidence](../firmware/poc_e220_evidence.md), [Gap analysis v0](ootb_gap_analysis_v0.md), [OOTB Radio preset v0](ootb_radio_preset_v0.md).

---

## Артефакты

| Документ | Описание |
|----------|----------|
| [poc_e220_evidence.md](../firmware/poc_e220_evidence.md) | Что проверено в POC (E220-400T30D): hardware, TX/RX, contention, ACK, power. |
| [ootb_gap_analysis_v0.md](ootb_gap_analysis_v0.md) | Что не входит в seed (battery, ownership, …). |
| [ootb_radio_preset_v0.md](ootb_radio_preset_v0.md) | **Конфигурация модуля OOTB** для Sprint 1 (E220/E22 UART): channel 1, air rate, UART baud, power, LBT, RSSI, config readback. |

Для полевой отладки и воспроизводимости конфигурации радиомодуля см. [OOTB Radio preset v0](ootb_radio_preset_v0.md).

**Resolved:** Air data rate default — **2.4k**; UART baud default — **9600**; LBT default — **OFF** (vendor default); contention через jitter/backoff в ПО (зафиксировано в [ootb_radio_preset_v0.md](ootb_radio_preset_v0.md)).

---

## Seed-critical gaps remaining

Пробелы, которые нужно закрыть для seed; отмечено: **[lookup/manual]** — извлечь из документации/ручного ввода, **[implementation]** — зафиксировать или реализовать в коде/спеках.

| # | Gap | Тип | Действие |
|---|-----|-----|----------|
| 1 | **E220/E22 UART config values** | lookup/manual + implementation | Извлечь и зафиксировать **точные значения** для OOTB preset: channel (номер/индекс), air rate, uart_baud, power steps, LBT flag, RSSI append, WOR, packet/sub-packet mode. Источник — manual модуля и при необходимости POC-архив. Заполнить TBD в [ootb_radio_preset_v0.md](ootb_radio_preset_v0.md). |
| 2 | **Channel → frequency mapping** | lookup/manual + implementation | Явно описать: **Channel** (vendor) маппится на частоту по **таблице производителя** (vendor-defined). В приложении показывать **каналы** (channel_id), не частоты в Hz — пользователь выбирает/видит «канал 1», «канал 2» и т.д. Зафиксировать в [radio_profile_presets_v0.md](../protocols/radio_profile_presets_v0.md) или [ootb_radio_preset_v0.md](ootb_radio_preset_v0.md); при наличии — ссылка на таблицу из manual. |
| 3 | **BLE connection policy** | implementation | Политика подключения BLE: **1 клиент** (одно активное подключение), **non-blocking reads** (чтение характеристик не блокирует основной цикл). Задокументировать в [OOTB BLE v0](../protocols/ootb_ble_v0.md) (новая подсекция или seed constraint). |
| 4 | **Session boundary policy** | implementation | Seed: **сессия = boot session** (с момента включения до выключения). Опционально в будущем: cleanup/TTL 24 h (например, сброс кэшей или маркировка устаревших данных). Зафиксировать в scope или BLE/firmware arch. |
| 5 | **Seed indication (screen vs LED)** | implementation + future | Для seed: индикация состояния через **экран** (если устройство с дисплеем) или минимальный UI в приложении. **LED-индикаторы** (мигание, цвет) — **отложены** (future). Явно записать в scope или product note. |
