## Goal

Implement NVS adapter and persisted seq16 behavior so that after reboot the first TX uses `restored_seq16 + 1`. This issue is the **execution wrapper**; use [#296](https://github.com/AlexanderTsarkov/naviga-app/issues/296) and [#355](https://github.com/AlexanderTsarkov/naviga-app/issues/355) as trackers — do not lose that history. No duplicate implementation; implement or complete what #296/#355 specify.

## Definition of done

- NVS/Preferences adapter exists (no ESP32-specific logic spread across domain).
- On boot: `restored_seq16` read; if > 0, first TX uses `restored_seq16 + 1`.
- Write policy for seq16 (per TX or batched) documented and implemented per canon/TODO.
- Unit test(s) for restore and first-TX behavior.

## Touched paths

- New or existing: NVS adapter (e.g. `firmware/src/platform/` or `firmware/lib/NavigaCore`); seq16 persistence call sites; domain/beacon or TX path that consumes restored_seq16.
- Canon: persistence/seq16 semantics (docs/product/areas/).

## Tests

- Unit test: after "restore" of a non-zero seq16, next generated seq16 is restored + 1.
- Optional: test that write policy (each TX vs flush) is consistent with implementation.

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Trackers: #296, #355.
