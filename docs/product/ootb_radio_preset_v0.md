# OOTB Radio preset v0 (Sprint 1 — E220/E22 UART)

**Purpose:** Зафиксировать конфигурацию радиомодуля **«из коробки»** для E220/E22 UART, используемую в Sprint 1. Отдельно различаем: **UART baud** (ESP ↔ модуль) и **Air data rate** (скорость по эфиру).

**Related:** [Радио модули Навига (hardware summary)](../hardware/radio_modules_naviga.md), [POC E220 evidence](../firmware/poc_e220_evidence.md), [Radio profile presets v0](../protocols/radio_profile_presets_v0.md), [OOTB Radio v0](../protocols/ootb_radio_v0.md). Артефакт Issue #7 — см. [poc_gaps_risks.md](poc_gaps_risks.md).

---

## 1. UART vs Air

- **UART baud** — скорость последовательного порта между **ESP32 и модулем** (E220 RX/TX). Инженерный выбор; на работу по эфиру не влияет.
- **Air data rate** — скорость передачи **по воздуху** (LoRa SF/BW или предустановки производителя). Должна **совпадать** на всех нодах в одной сети и с выбором POC, если доказана совместимость.

---

## 2. OOTB Public profile (channel 1) — единый пресет для E220 и E22

Один и тот же конфиг для **E220-400T30D** и **E22-400T30D** (UART), Sprint 1. Чеклист:

| Параметр | Значение | Примечание |
|----------|----------|------------|
| **channel_id** (product) | 1 | OOTB Public network. Маппинг на частоту/модуль — см. § 2.1. |
| **air_data_rate** | **2.4 kbps** | Минимальная скорость для максимальной дальности; airtime beacon ~40 B в пределах 300 ms. См. примечание ниже. |
| **UART** | **9600 8N1** | 9600 baud, 8 data bits, no parity, 1 stop bit. Seed: стабильность; трафик малый. |
| **LBT** | **OFF** | Seed default. Детерминированность таймингов; contention — jitter + backoff/retry в ПО. См. [OOTB Radio v0 § 5.2](../protocols/ootb_radio_v0.md#52-software-level-randomization). |
| **RSSI append** | **ON** | Модуль добавляет RSSI к пакету на UART. **Правило разбора:** payload + **trailing RSSI byte(s)** по manual (формат и число байт — по документации модуля). |
| **TX power** | **Минимум** (доступный уровень) | Режим модуля «21 dBm»; по отзывам сообщества эффективная мощность ~19–20 dBm. ADR: [radio band strategy](../adr/radio_band_strategy_v0.md) — module_min. |
| **sub-packet length** | **128 bytes** | Явно задано для **совместимости между модулями**: E220 default 200 B, E22 default 240 B — приводим к 128 B. |
| **WOR** | **OFF** | Wake-on-Radio в seed не используется. |
| **encryption** | **OFF** | Seed: публичная сеть. |
| **mode (transparent / fixed)** | **TBD** | По реализации и manual. |

Конкретные команды/регистры (M0/M1, конфиг по UART) — по документации модуля и выбранной библиотеке.

### 2.1. RF channel mapping (anchor)

- **channel_id** — **продуктовая абстракция** (1 = OOTB Public, 2..N = private). Пользователь и приложение оперируют channel_id; частоты в Hz не показываются.
- **Модуль (vendor):** у E220/E22 параметр **CH** — индекс канала производителя; соответствие CH ↔ частота задаётся **таблицей в manual**. Источник выжимки по каналу, sub-packet, RSSI append и др.: [radio_modules_naviga.md](../hardware/radio_modules_naviga.md).
- **Якорь для Public:** продуктовый **channel_id = 1** (OOTB Public) маппится на **module CH = 0x17** → **433.125 MHz**. Полный маппинг ChannelId → CH (433 band, E220/E22): [radio_channel_mapping_v0.md](radio_channel_mapping_v0.md).

**Примечание (airtime):** Payload ~40 B при 2.4k даёт ~133 ms «сырого» времени в эфире + накладные расходы (preamble, header); ожидаемо укладывается в лимит **300 ms** (см. [OOTB Radio v0 § 4.3 Airtime](../protocols/ootb_radio_v0.md#43-airtime-constraint)). **Future:** при росте перегрузки канала рассмотреть 4.8k / 9.6k, если дальность остаётся приемлемой.

---

## 3. Config readback

- **При загрузке:** после инициализации модуля выполнять **чтение текущей конфигурации** (read config по UART) и **логировать эффективные настройки** (channel, air rate, power, LBT, RSSI и т.д.).
- **Цель:** полевой отладка — по логам видно, с каким конфигом реально работает устройство (исключить рассинхрон с ожидаемым профилем).

---

## 4. Future notes

- **Advanced power toggle:** возможность переключения уровня мощности (не только module_min) — вне seed; при необходимости отдельный пресет или конфиг.
- **Region SKUs:** разные SKU модулей по регионам (частоты, мощность по нормам); маппинг band_id / channel_id на конкретный модуль — TBD.
- **Security / ownership:** шифрование и привязка к владельцу — вне seed (публичная сеть); см. [Gap analysis § Device ownership](ootb_gap_analysis_v0.md#device-ownership--access-control--pairing).
