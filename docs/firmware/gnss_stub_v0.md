# GNSS Stub v0 (OOTB seed)

**Purpose:** Поведение **GNSSStub** — реализации GNSSProvider для разработки и тестов без GNSS-железа. Seed: детерминированная или псевдо-случайная модель движения и опциональная симуляция NO_FIX. В production используется реальный GNSS provider (например u-blox M8N). См. [Firmware arch § GNSS specifics](ootb_firmware_arch_v0.md#1-scheduler--tick-model-seed).

**Related:** [HAL § 3 GNSSProvider / GNSSSnapshot](hal_contracts_v0.md#31-gnssprovider-abstraction--enum-and-struct), [gnss_v0.md § 5 Stub mode](gnss_v0.md#5-stub-mode).

---

## 1. Base coordinate and update rate

- **Base coordinate:** фиксированная база — **Ростов Великий, Россия**. Параметры **base_lat** / **base_lon** (в градусах или lat_e7/lon_e7) задают центр случайного блуждания; по умолчанию — координаты Ростова Великого.
- **Update frequency:** **1 Hz**. Stub выдаёт новый GNSSSnapshot раз в секунду (согласовано с опросом GNSSTask в [ootb_firmware_arch_v0.md](ootb_firmware_arch_v0.md#1-scheduler--tick-model-seed)).

---

## 2. Movement model

- **Модель:** **random walk** вокруг базы (base_lat, base_lon).
- **Шаг:** на каждом тике (1 Hz) позиция смещается на величину шага в случайном направлении.
  - **Step magnitude:** масштаб шага привязан к **min_distance** (например из cadence: min_movement_m). Формула: шаг = **step_scale × random(0.7, 1.3) × min_distance** (в метрах; затем перевод в приращение lat/lon). Параметр **step_scale** (config knob) позволяет усилить или ослабить движение (по умолчанию 1.0).
  - Итог: шаг порядка **0.7–1.3 × min_distance** (при step_scale=1), что даёт возможность тестировать срабатывание beacon по «min time passed but no min distance» при малом движении.
- **Stationary probability:** с вероятностью **stationary_probability** (например **30%**) на данном тике **шаг не выполняется** (позиция не меняется). Это даёт сценарии «min_update_interval_s истёк, но movement < min_movement_m» — проверка keep-alive и подавления TX по движению.

---

## 3. FIX behavior

- **По умолчанию:** stub отдаёт **FIX_3D** (fix_state = FIX_3D, **pos_valid = true**), координаты из модели движения, last_fix_ms актуален.
- **Опциональная симуляция NO_FIX (только для тестов):**
  - **no_fix_enable** — включить периодические «потери фикса». **По умолчанию OFF.**
  - **no_fix_interval_s** — интервал между началами серий NO_FIX (например **300–600 s**). Каждые no_fix_interval_s секунд начинается «всплеск» NO_FIX.
  - **no_fix_duration_s** — длительность одного всплеска NO_FIX (например **10–20 s**). В этот период getSnapshot() возвращает fix_state = NO_FIX, pos_valid = false; координаты могут быть нулевыми или последними валидными (TBD по реализации).
  - Используется для проверки no-fix behavior (логи fix lost, beacon с pos_valid=false, восстановление после no_fix_duration_s).

---

## 4. Config knobs (seed)

| Параметр | Описание | Пример / default |
|----------|----------|-------------------|
| **base_lat** / **base_lon** | Базовая точка (центр random walk), Ростов Великий или заданные значения. | Координаты Ростова Великого. |
| **step_scale** | Множитель величины шага (относительно min_distance). | 1.0 |
| **stationary_probability** | Вероятность не двигаться на тике (0–1). | 0.3 (30%) |
| **no_fix_enable** | Включить периодическую симуляцию NO_FIX. | false (OFF) |
| **no_fix_interval_s** | Интервал между началами серий NO_FIX, с. | 300–600 |
| **no_fix_duration_s** | Длительность одной серии NO_FIX, с. | 10–20 |

Конкретные значения по умолчанию и способ задания (compile-time, конфиг-файл, BLE) — TBD при реализации.

---

## 5. Note

**Stub предназначен для разработки и тестирования.** В production используется реальный GNSS provider (например u-blox M8N по UART); замена stub → real provider не меняет контракт GNSSSnapshot и логику cadence/NodeTable (см. [ootb_firmware_arch_v0.md](ootb_firmware_arch_v0.md#1-scheduler--tick-model-seed)).
