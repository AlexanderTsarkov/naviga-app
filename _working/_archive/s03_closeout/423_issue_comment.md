**PR:** #441 — feat(#423): align boot phases A/B/C and enable OOTB autonomous start

**Summary of changes:**
- **Setup:** Removed blocking `while (!Serial)` in `main.cpp` so the device boots without USB (OOTB autonomous start).
- **Phase A:** Combined `boot_config_result` (Ok / Repaired / RepairFailed) from GNSS + radio; one boot log line; exposed via provisioning `status` (#393). RepairFailed does not brick; init continues to Phase B/C.
- **Phase B/C:** Already matched canon; no code change.
- **Tests:** New native test `test_boot_phase_contract` for enum ordering and `worst_of` semantics (4 tests).

**Validation:**
- `test_boot_phase_contract` (env `test_native_nodetable`): passed
- `pio run -e devkit_e220_oled`: build passed

**Bench:** Minimal checklist (flash → power on without USB → confirm Alive/Beacon) is documented in `_working/423_boot_phases_contract_and_gaps.md`; can be run when hardware is available. PR does not block on bench.

**Boundary:** Any remaining radio-profile baseline alignment (e.g. FACTORY tx power step 0 = 21 dBm) belongs to **#424**, not #423.
