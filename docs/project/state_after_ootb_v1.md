# State after OOTB v1

**Completion date:** 2026-02-12.  
**Meaning of “OOTB v1 done”:** All planned OOTB v0 / Mobile v1 issues (#2–#35, #74, #76, #80–#90) are delivered and closed. Firmware sends/receives GEO_BEACON, maintains NodeTable, exposes data over BLE; mobile app connects, shows nodes and map. Field test 2–5 nodes done.

Evidence and trail:

- [OOTB progress inventory](ootb_progress_inventory.md) — per-issue PR links and Mobile v1 table.
- [OOTB workmap](ootb_workmap.md) — plan, status table, phase rules.

## Key constraints / invariants (do not violate)

- **Embedded-first stack:** firmware → radio → domain → BLE bridge → mobile.
- **Hardware:** ESP32 + E220 UART LoRa modules used as **MODEMs** (not chip-level drivers). E220Radio = IRadio adapter.
- **CAD/LBT:** Out of scope. Channel-sense abstraction exists; E220 returns UNSUPPORTED. Default = jitter-only, sense OFF.
- **Domain:** NodeTable and BeaconLogic are **radio-agnostic** (no radio types in domain).
- **Wiring:** **M1Runtime** is the single composition point (GEO_BEACON ↔ NodeTable ↔ BLE).
- **Legacy BLE service:** Remains **disabled** unless explicitly reintroduced by plan.

## Explicitly out of scope (OOTB v1)

- JOIN, Session Master, anchor roles.
- Mesh (covered_mask, best_mask, relays).
- Real CAD/LBT; SPI radio driver; offline maps in v0.
- BLE write/config in mobile v1.

## Next known follow-ups

- **Issue #135:** Docs archive/cleanup (working docs → `_archive/ootb_v1/`). **PR3** of [#119](https://github.com/AlexanderTsarkov/naviga-app/issues/119) will perform the archive; this PR (PR2) does not move or delete files.
- JOIN/Mesh: future; see product specs when needed.
