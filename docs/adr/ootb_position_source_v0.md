# ADR: OOTB v0 — источник координат (position source)

**Status:** Accepted  
**Date:** 2026-02

---

## Decision

**Primary position source in OOTB v0 = GNSS on the dongle.** Coordinates for GEO_BEACON and NodeTable come from the dongle’s GNSS subsystem. The dongle must work as an autonomous radio beacon without a phone.

---

## Rationale

- **Автономность донгла:** Донгл — самостоятельный радио-маяк; работа не зависит от подключения телефона.
- **Не сажать телефон:** Геолокация телефона не используется как источник координат в v0 — экономия батареи телефона и упрощение сценария.
- **Энерго-профиль донгла:** Источник позиции один (GNSS на донгле); проще планировать питание и логику.
- **Телефон — настройка и UX:** Телефон нужен для подключения по BLE, отображения NodeTable/карты и настроек, но не как источник координат для beacon.

---

## Non-goals for v0

- **Phone GNSS as position source:** Геолокация телефона не является источником координат в OOTB v0.

---

## Future option (TBD)

- **Secondary / override from phone GNSS:** Возможность использовать координаты телефона как вторичный или переопределяющий источник — вне scope v0, решение позже.

---

## Consequences

- BLE- и Radio- payload не зависят от телефона как источника координат; beacon всегда берёт позицию из донгла (GNSS или stub).
- В firmware нужен интерфейс **IGnss** (HAL) и реализация (реальный GNSS или **stub** для разработки без железа). См. [hal_contracts_v0.md](../firmware/hal_contracts_v0.md), [gnss_v0.md](../firmware/gnss_v0.md).
- Domain-слой не знает про NMEA/serial — только через IGnss.
