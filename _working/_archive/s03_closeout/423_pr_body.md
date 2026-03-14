## Summary

Implements **#423**: Boot phases A/B/C conformance and OOTB autonomous start without phone/BLE.

**Scope (this PR only):**
- Phase A: combined `boot_config_result` (Ok / Repaired / RepairFailed), one boot log line, exposed via status (#393). RepairFailed does not brick; init continues to Phase B/C.
- Setup: removed blocking `while (!Serial)` so the device boots and runs without USB.
- Phase B/C: already matched canon; no code change in this PR.

**Out of scope (do not expand here):**
- **#424** — Radio profile baseline alignment (e.g. FACTORY tx power step 0 = 21 dBm). Any such work belongs in #424.
- #425 / #426 — Not in scope.

---

## Code changes

| Area | Change |
|------|--------|
| **Setup** | `firmware/src/main.cpp`: Remove blocking `while (!Serial)`. Device proceeds with boot without waiting for USB (OOTB autonomous start). |
| **Phase A** | `firmware/src/app/app_services.cpp`: After GNSS and radio bring-up, compute combined Phase A `boot_config_result` = worst of (GNSS, radio). Log one line `Phase A boot_config_result: Ok|Repaired|RepairFailed`. Expose via `set_radio_boot_info(combined_result, combined_msg)` so `status` command shows Phase A result. |
| **Tests** | `firmware/test/test_boot_phase_contract/`: New native test for `RadioBootConfigResult` enum ordering and `worst_of` semantics. |
| **Working doc** | `_working/423_boot_phases_contract_and_gaps.md`: Contract restatement, gap table, implementation summary, bench checklist. |

---

## Validation

- **`test_boot_phase_contract`** (native): 4 tests passed (`test_native_nodetable` env).
- **`pio run -e devkit_e220_oled`**: Build succeeded.

---

## Bench

Minimal bench checklist (flash → power on without USB → confirm Alive/Beacon within window) is documented in `_working/423_boot_phases_contract_and_gaps.md`. It may be run separately when hardware is available; this PR does not block on bench execution.

---

## Refs

- Closes #423 (after review / optional bench).
- Canon: boot_pipeline_v0, module_boot_config_v0, provisioning_baseline_v0, ootb_autonomous_start_v0.
- Legacy: #393 (boot_config_result observable, no brick on RepairFailed).
