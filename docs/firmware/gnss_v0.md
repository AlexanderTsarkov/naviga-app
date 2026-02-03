# GNSS subsystem v0 (OOTB)

**Purpose:** Определить поведение и контракт подсистемы GNSS на донгле для OOTB v0. Только «что нужно определить / как должно себя вести», без реализации. Источник координат — [ADR: position source](../adr/ootb_position_source_v0.md).

---

## 1. States

| State    | Описание | Valid position? |
|----------|----------|-----------------|
| NO_FIX   | Нет решения (cold start, нет спутников, потеря) | Нет |
| FIX_2D   | 2D fix (lat/lon без высоты или с барометрической) | Да (для beacon — см. политику) |
| FIX_3D   | 3D fix (lat/lon/alt) | Да |

**Valid position для beacon:** считаем валидной позицией для отправки в GEO_BEACON только когда fix достаточен (FIX_2D или FIX_3D по политике v0; критерии точности — см. Open questions).

---

## 2. Data model (что отдаёт IGnss наверх)

- **lat, lon** — широта/долгота (формат и единицы — TBD, см. OOTB plan: coordinate encoding).
- **alt** — высота (опционально для v0; при FIX_2D может отсутствовать или 0).
- **fixType** — NO_FIX / FIX_2D / FIX_3D.
- **accuracy / hdop** — если есть от модуля (TBD: как передавать, единицы).
- **satCount** — количество спутников (опционально для логирования/диагностики).
- **timestamp / uptime** — момент измерения (unix seconds или uptime — TBD, см. OOTB plan: time field).

Domain и beacon используют только эти поля; не зависят от NMEA/serial.

---

## 3. Update rates

- **Как часто IGnss обновляет данные:** TBD (зависит от железа и энергобаланса; типично 1 Hz или реже для beacon-сценария).
- **Как beacon использует позицию:** высокоуровнево — при очередной отправке beacon читает последнюю валидную позицию из IGnss; при NO_FIX — см. No-fix behavior.

---

## 4. No-fix behavior

- **Что отдаём наверх:** fixType = NO_FIX; позиция не считается валидной (pos_valid = false).
- **Что логируем:** fix lost, cold start, повторные попытки (см. Logging requirements).
- **Что отправляем по радио:** beacon может не отправляться или отправляться с явным флагом **pos_valid = false** (и пустыми/нулевыми координатами или без поля позиции — формат в GEO_BEACON v0 TBD). Приёмники не обновляют свою позицию «я» по такому пакету.

---

## 5. Stub mode

Для разработки без GNSS-железа допускается **GNSS-stub** — детерминированный генератор координат.

- **Параметры:** center (lat, lon), radius (м или градусы — TBD), speed (условная «скорость» смещения — TBD).
- **Поведение:** генерация последовательности позиций вокруг center в пределах radius; детерминированность для воспроизводимости тестов.
- **Реализация:** за загранью этого документа; в HAL — реализация IGnss (stub), подменяемая на реальный драйвер.

---

## 6. Logging requirements (минимум)

- **Cold start** — модуль запущен, фикса ещё нет.
- **Fix acquired** — переход в FIX_2D или FIX_3D (при желании — satCount, hdop).
- **Fix lost** — переход в NO_FIX.
- **Stale fix** — фикс есть, но данные не обновлялись дольше порога (TBD).

Частота и объём логов — TBD (с учётом ring-buffer и экспорта по BLE/UART).

---

## 7. Open questions

- **Accuracy:** как получать и передавать accuracy/hdop от конкретного GNSS-модуля; порог «достаточно хорошей» точности для valid position в v0.
- **Coordinate encoding:** формат lat/lon и точность — в [OOTB plan](../product/OOTB_v0_analysis_and_plan.md) (coordinate encoding, time field).
- **Stub parameters:** конкретные значения по умолчанию для radius/speed — при необходимости зафиксировать в конфиге или тестах.
