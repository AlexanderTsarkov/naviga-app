# Radio channel mapping v0 (433 band, E220/E22 UART)

**Purpose:** Маппинг продуктового **ChannelId** (1..N) на **module CH** (vendor channel index) для 433 MHz band. Один и тот же маппинг для E220-400T30D и E22-400T30D (UART compatibility).

**Related:** [Радио модули Навига (hardware summary)](../hardware/radio_modules_naviga.md), [OOTB Radio preset v0](ootb_radio_preset_v0.md), [Radio profile presets v0](../protocols/radio_profile_presets_v0.md), [OOTB Radio v0 § 5–6](../protocols/ootb_radio_v0.md#5-contention-handling-seed). GitHub: issue «Define ChannelId→CH mapping for 433 band (E220/E22 UART compatibility)» (Doc/Design).

---

## 1. Definitions

- **ChannelId** — продуктовая абстракция: целое 1..N. Пользователь и приложение (в т.ч. BLE DeviceInfo / настройки) оперируют ChannelId; частоты в Hz в UI не показываются.
- **Module CH** — индекс канала производителя (E220/E22): 0..CH_max. Соответствие CH ↔ частота (MHz) задаётся таблицей в manual модуля.

---

## 2. Mapping (433 band)

**Формула:** для 433 MHz band

- **ChannelId = 1** (OOTB Public) → **module CH = 0x17** (23) → **433.125 MHz** (якорь).
- **ChannelId = 2..N** → **module CH = 0x17 + (ChannelId − 1)** (линейный сдвиг: 2→0x18, 3→0x19, …).

| ChannelId | Module CH (hex) | Module CH (dec) | Примечание |
|-----------|-----------------|-----------------|-------------|
| 1 | 0x17 | 23 | Public; 433.125 MHz. |
| 2 | 0x18 | 24 | Private. |
| … | … | … | |
| N | 0x17 + (N−1) | 23 + (N−1) | Private; верхняя граница — см. § 3. |

---

## 3. Constraints and max N

- **Одинаковый маппинг для E220 и E22:** одна и та же формула CH = 0x17 + (ChannelId − 1) для обоих модулей в 433 band (совместимость по эфиру при одинаковом ChannelId).
- **Public зарезервирован:** ChannelId = 1 — только OOTB Public; диапазон **private** — **2..N**.
- **Верхняя граница N:** ограничена диапазоном **module CH** (0..CH_max по manual). Для типичного 433 MHz модуля CH_max = 83 (или по даташиту). Тогда **max ChannelId N = CH_max − 0x17 + 1**. Пример: при CH_max = 83 получаем N_max = 83 − 23 + 1 = **61**. Конкретное CH_max уточнять по manual E220/E22 и зафиксировать здесь (TBD если ещё не подтверждено).

---

## 4. References

- Реализация: при установке канала передавать в модуль **CH**, вычисленный по ChannelId (конвертация в драйвере/HAL). BLE и приложение работают только с **ChannelId**; см. [OOTB BLE v0 DeviceInfo / network_mode, channel_id](../protocols/ootb_ble_v0.md#11-deviceinfo-characteristic), [OOTB Radio preset v0 § 2.1](ootb_radio_preset_v0.md#21-rf-channel-mapping-anchor).
