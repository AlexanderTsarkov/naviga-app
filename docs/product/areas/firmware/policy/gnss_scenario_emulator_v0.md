# GNSS Scenario Emulator v0 (policy)

**Status:** Policy (design + V0 implementation).  
**Issue:** [#288](https://github.com/AlexanderTsarkov/naviga-app/issues/288)  
**Gate:** #277

---

## 1) Goal

Provide **deterministic GNSS behavior** for bench and CI-style acceptance tests: scripted states (NO_FIX, FIX_NO_MOVE, FIX_MOVE, FLAP), controllable via provisioning shell, repeatable. Enables proving cadence rules (#281: minInterval, maxSilence, minDistance, CORE vs ALIVE) under controlled scenarios without moving hardware.

## 2) Non-goals

- No hardware GNSS simulator.
- No mesh/JOIN/CAD/LBT.
- No on-air protocol or codec changes.
- No BLE/mobile changes.
- V0 only: minimal shell-driven injection; no full scenario runner or UART replay in this version.

## 3) Design (V0)

- **Override layer:** When enabled, the app uses an injected `GnssSnapshot` (pos_valid, lat_e7, lon_e7, last_fix_ms) instead of calling the real GNSS provider. Default: **disabled** (real GNSS).
- **Control:** Provisioning shell commands over serial.
- **States:** NO_FIX (pos_valid=false), FIX (pos_valid=true, fixed lat/lon), FIX_MOVE (increment position with `move`). FLAP (optional V0: manual toggle or future extension).

## 4) Commands

| Command | Effect |
|--------|--------|
| `gnss off` | Disable override; use real GNSS. |
| `gnss nofix` | Override: NO_FIX (pos_valid=false). |
| `gnss fix <lat_e7> <lon_e7>` | Override: FIX at given position (e.g. `571844000 383663000`). |
| `gnss move <dlat_e7> <dlon_e7>` | Increment override position (e.g. `1000 0` ≈ ~11 m north). |

Coordinates in **e7** (degrees × 10^7). Response confirms mode, e.g. `OK; gnss override NO_FIX`.

## 5) Log output

When override is active, the app logs periodically (e.g. every 5 s):  
`GNSS override: NO_FIX` or `GNSS override: FIX lat_e7=... lon_e7=...`.

## 6) Example (repeatable script)

```
gnss nofix
debug on
# wait > maxSilence (e.g. 35 s for role 0) → expect ALIVE at maxSilence
# then:
gnss fix 571844000 383663000
# wait min_interval (e.g. 18 s) with no move → no CORE (NO_SEND)
gnss move 3000000 0
# position commit → next TX can be CORE
```

## 7) DoD

- Run script produces repeatable logs.
- Scenario 1: NO_FIX for > maxSilence ⇒ ALIVE at maxSilence.
- Scenario 2: FIX_NO_MOVE (no movement) ⇒ NO_SEND until maxSilence ⇒ then CORE (or ALIVE if no fix at that moment per policy).
- Scenario 3: FIX_MOVE ⇒ commits happen and CORE cadence follows role gating.

## 8) References

- field_cadence_v0, role_profiles_policy_v0
- #281 (role cadence wiring)
- provisioning_interface_v0 (shell)
