# OOTB v0 — Gap analysis (пробелы и отложенные темы)

**Purpose:** Что не входит в seed sprint 1, но остаётся в зоне внимания проекта (owned). Issue [#8](https://github.com/AlexanderTsarkov/naviga-app/issues/8) (POC gaps/risks) и связанные.

**Related:** [OOTB plan](OOTB_v0_analysis_and_plan.md), [Test Plan](ootb_test_plan_v0.md), [Workmap](ootb_workmap.md).

---

## Deferred / Not in seed sprint 1

### Battery measurement & power budget

- **Current seed:** питание только от USB; аппаратного измерения батареи в seed нет.
- **Telemetry:** поле **battery_mv** в BLE Health в seed — **N/A или unknown**; зафиксировать ожидаемое поведение (например: 0, sentinel value, или «not available» в spec/UI).
- **Future:** добавить измерение по ADC + калибровка + отчёт в BLE + power profiling (оценка бюджета по режимам). Владелец темы — [#46](https://github.com/AlexanderTsarkov/naviga-app/issues/46) (см. [workmap](../project/ootb_workmap.md) § 4 Future / Deferred).

### Device ownership / access control / pairing

- **Problem statement:**
  - Who can connect over BLE and change settings?
  - How does a user “claim” a device (personal ownership)?
  - How to pair dongle + collar as a set?
  - Fleet scenario (e.g. hunting lodge distributing devices): admin vs user permissions.
- **Seed scope:** out of scope; document as **planned work** (Phase 2+). См. [workmap](../project/ootb_workmap.md) § 4 Future / Deferred и issue по ownership & pairing (TBD).
