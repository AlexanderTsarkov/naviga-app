# OOTB v0 — Scope & boundaries

**Purpose:** Границы OOTB v0, разбивка по спринтам и явный список того, что не входит в Sprint 1. Issue [#17](https://github.com/AlexanderTsarkov/naviga-app/issues/17).

**Related:** [OOTB plan](OOTB_v0_analysis_and_plan.md), [Workmap](../../_archive/ootb_v1/project/ootb_workmap.md), [Gap analysis](ootb_gap_analysis_v0.md).

---

## 1. OOTB Sprint breakdown

### Sprint 1 (Foundation)

**Goal / Definition of Done:**

- **Hardware:** 2 dongles (minimal setup for field test).
- **Radio:** Beacon with contention handling — CAD if supported by module; otherwise jitter + basic backoff (см. [OOTB Radio v0 § 4 Beacon cadence](../protocols/ootb_radio_v0.md#4-beacon-cadence-seed)).
- **NodeTable v0** on device: self entry, grey policy (NORMAL/GREY by expected_interval_s + grace_s), eviction limit = 100. См. [NodeTable v0](../firmware/ootb_node_table_v0.md).
- **BLE read-only telemetry:** DeviceInfo + Health (см. [OOTB BLE v0 § 1.1–1.2](../protocols/ootb_ble_v0.md#1-telemetry-v0-seed-deviceinfo--health)).
- **BLE NodeTableSnapshot** paging (page_size = 10); см. [OOTB BLE v0 § 1.3](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic).
- **App minimal:** list view only — short_id, normal/grey, last_seen_age_s, coords (no map in Sprint 1).
- **Reset path:** Ability to return to OOTB radio defaults (“Back to Public” / hardware reset path); см. [OOTB Radio v0 § 7 Reset](../protocols/ootb_radio_v0.md#7-reset).

**Sprint 1 default cadence («Human» profile):**

| Параметр | Значение | Примечание |
|----------|----------|------------|
| **min_distance_m** | 25 | Минимальное смещение (м) для TX по движению. |
| **min_time_s** | 20 | Минимальный интервал между отправками по движению (приведён от ~5 км/ч и шага 25 м, округлён). Соответствует min_update_interval_s в [Radio v0 § 4](../protocols/ootb_radio_v0.md#41-parameters). |
| **max_silence_s** | 80 | Keep-alive: при отсутствии движения — TX не реже чем раз в 80 с (4 × min_time_s). |

**UX note (cadence):** Целевое ожидание для пользователя — **«видеть обновление примерно каждые 25 м»** (движение). Порог внимания: **«нет обновления ~100 м / ~80 с»** — считать поводом для проверки (нода могла остановиться, потеря связи, или keep-alive ещё не сработал).

### Sprint 2

**TBD** — планируется после завершения Sprint 1.

**Ideas (short list):** map UI, optional write/config via BLE, battery reporting, ownership/pairing exploration, improved congestion/backoff. Конкретный DoD и приоритеты — после ретро Sprint 1.

---

## 2. Out of Sprint 1

Явно **не входит** в Sprint 1:

- **Map UI** — карта в приложении (Sprint 2+).
- **Write / Config** — изменение channel, power, network mode через BLE (read-only в Sprint 1).
- **Ownership / pairing** — claiming device, dongle+collar pairing, fleet admin vs user (см. [Gap analysis § Device ownership](ootb_gap_analysis_v0.md#device-ownership--access-control--pairing)).
- **Battery** — измерение и отчёт battery_mv (seed: USB only, N/A); см. [Gap analysis § Battery](ootb_gap_analysis_v0.md#battery-measurement--power-budget).
- **Advanced congestion control** — airtime utilization, adaptive backoff (вне seed; см. [OOTB Radio v0 § 4.2](../protocols/ootb_radio_v0.md#42-seed-algorithm-псевдоописание)).
