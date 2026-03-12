# #443 User Profile baseline — final report

**Issue:** #443 · **Umbrella:** #416  
**Canon:** node_role_type_review_s03, provisioning_baseline_s03/v0, boot_pipeline_v0, ootb_autonomous_start_s03, role_profiles_policy_v0

---

## 1) What changed

### Phase B (app_services.cpp)

- **Role vs radio resolved independently:** `effective_role_id` is computed from NVS using only `ptrs.has_current_role` and `ptrs.current_role_id <= 2`. `effective_radio_id` is computed from NVS alone (V1-A: only 0). A valid persisted role is now used even when radio pointer is missing.
- **Profile record when role normalized:** If the role pointer was invalid or out-of-range (role_normalized), or the stored profile record is invalid (!profile_valid), the active profile record is set from `get_ootb_role_profile(effective_role_id)` and persisted. This keeps the stored record aligned with the resolved role.
- **Previous pointers:** `new_previous_role` / `new_previous_radio` still computed from role_normalized / radio_changed and persisted with `save_pointers`.

### SelfTelemetry and Status (role_id)

- **SelfTelemetry.role_id:** Added `role_id` to `SelfTelemetry` (beacon_logic.h). In Phase B, `self_telemetry_.role_id` is set from `effective_role_id` (clamped 0–2). `has_max_silence` and `max_silence_10s` are set in the same block from the resolved profile.
- **BeaconLogic Status formation:** Status frame now uses `telemetry.role_id` instead of hardcoded 0 (beacon_logic.cpp).

### factory_reset_pointers (naviga_storage.cpp)

- After removing pointer keys, **persists default pointers** (0,0,0,0) so the next boot sees consistent state without relying on “no key” behavior.

### Tests

- **test_txq_status_encodes_role_id_from_telemetry:** Ensures that when `SelfTelemetry.role_id = 1`, the Status slot encodes role_id=1 and that after dequeue the decoded payload has `decoded.role_id == 1`. Proves runtime role_id comes from active user profile (telemetry), not a constant.

---

## 2) What tests prove

| Test | Proves |
|------|--------|
| test_txq_status_encodes_role_id_from_telemetry | Status frame role_id is taken from SelfTelemetry (active profile), not hardcoded. |
| test_ootb_role_* (existing) | get_ootb_role_profile(0,1,2) and invalid fallback; baseline for Phase B profile record. |
| test_txq_max_silence_only_enqueues_informative_not_operational | Status enqueue path unchanged; cadence/telemetry still drive formation. |
| Devkit build (devkit_e220_oled_gnss) | Full firmware build passes. |
| test_beacon_logic (59 cases) | All beacon logic tests pass, including new role_id test. |

**Not covered by unit tests (by design):** Missing/invalid NVS pointer fallback and persistence are in the init path that uses real Preferences/NVS; manual or device tests can confirm. Implementation follows canon: when `!has_current_role` or `current_role_id > 2`, `effective_role_id = 0` and we persist and refresh the profile record.

---

## 3) What remains for #425 only

- Observability/logging beyond the existing Phase B log line (e.g. “Phase B: role=X profile=Y source=…”).
- Any additional instrumentation or diagnostics for profile/pointer state.
- Broader “current state” docs (#426) and BLE/UI profile editing are out of scope for #443.

---

## 4) Exit criteria checklist

- [x] #443 contract restated from canon
- [x] Gap table produced
- [x] Minimal #443 delta implemented (pointers independent, profile record on role normalize, role_id in telemetry and Status, factory_reset persists 0,0,0,0)
- [x] Tests added/updated and passing (test_txq_status_encodes_role_id_from_telemetry; existing role_profile_registry and beacon_logic)
- [x] Devkit build passing
- [x] Docs synchronized where needed (contract/source-of-truth in _working/443_contract_and_gaps.md; no canon rewrite)
- [x] Final close recommendation prepared (this report)
- [x] Runtime source-of-truth table for cadence/liveness produced (in contract doc §2)
- [x] No hardcoded cadence defaults bypassing active profile; role_id hardcode removed

**Recommendation:** Close #443 as implemented. User profile baseline is aligned with canon: Phase B resolves default/current/previous role pointers and normalizes to id 0 with persistence; active user profile values (role_id, min_interval_s, min_displacement_m, max_silence_10s) are the single source for runtime cadence/liveness and for Status role_id. No scope bleed into #424/#425/#426.
