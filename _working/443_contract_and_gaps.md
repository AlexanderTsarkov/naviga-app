# #443 User Profile baseline — contract and gaps

**Issue:** #443 · **Umbrella:** #416  
**Canon:** node_role_type_review_s03, provisioning_baseline_s03/v0, boot_pipeline_v0, ootb_autonomous_start_s03, role_profiles_policy_v0

---

## 1) #443 contract (restated from canon)

- **Phase B owns:** Resolve default/current/previous **user profile (role) pointers** from NVS (`role_cur`, `role_prev`). If missing or invalid → apply **default role** (profile id 0, Person) and **persist** pointers. Resolve **active role profile values** (min_interval_s, min_displacement_m, max_silence_10s) from the current profile record or from OOTB defaults for the resolved role id; when falling back to default role, refresh the stored role record to that role’s defaults and persist. User profile semantics are **separate** from radio profile semantics (#424).
- **Phase C consumes:** Only the **provisioned** active user profile: role_id and the three normative params (min_interval_s, min_displacement_m, max_silence_10s) for cadence/liveness. No ad hoc or hidden hardcoded cadence defaults at “start comms” or in the tick path.
- **Must not be conflated:** Logical profile state (pointers + active values) vs UI/BLE profile management; user (role) profile vs radio profile.

---

## 2) Runtime cadence/liveness source-of-truth

**Post-#443 implementation:**

| Value | Source | File:line evidence |
|-------|--------|--------------------|
| min_interval_s | Resolved from profile_record in Phase B; passed to runtime as min_interval_ms | app_services.cpp:264–271, 328; M1Runtime::init → BeaconLogic::set_min_interval_ms |
| min_displacement_m | profile_record.min_displacement_m → self_policy.set_min_distance_m | app_services.cpp:267, 297; SelfUpdatePolicy |
| max_silence_10s | profile_record.max_silence_10s → effective_max_silence_10s_ → runtime + self_telemetry_ | app_services.cpp:265, 270, 301, 352–353 |
| role_id | effective_role_id → self_telemetry_.role_id → BeaconLogic Status formation | app_services.cpp:299–302; beacon_logic.cpp:227 telemetry.role_id |

**No hidden hardcoded cadence defaults** bypassing active profile; BeaconLogic/SelfUpdatePolicy defaults are overwritten in init from profile_record.

---

## 3) Gap table

| # | Item | Current state | Target | Class |
|---|------|---------------|--------|-------|
| G1 | Default/current/previous role pointers | NVS keys role_cur, role_prev; load_pointers/save_pointers | Explicit; fallback to 0 and persist | In-scope: ensure resolution independent of radio |
| G2 | Fallback to profile id 0 on missing/invalid | use_persisted requires both role and radio; invalid role_id > 2 → default | Missing/invalid **role** only → effective_role_id = 0, persist | In-scope |
| G3 | Persistence of normalized pointers | save_pointers(effective_*) after Phase B | Same; ensure we persist when role normalized | In-scope |
| G4 | Active role resolution at boot | effective_role_id from ptrs when use_persisted; else 0 | Resolve role independently: has_current_role && role in [0,2] → use ptrs.current_role_id; else 0. Persist. | In-scope |
| G5 | Profile record when role normalized | If !profile_valid we get_ootb and save. If role normalized we don’t refresh record. | When effective_role_id was normalized (fallback), set profile_record = get_ootb_role_profile(effective_role_id) and save | In-scope |
| G6 | runtime cadence from active profile | min_interval_s, min_displacement_m, max_silence_10s from profile_record | No change; already correct | Already correct |
| G7 | role_id in runtime/telemetry/Status | BeaconLogic st.role_id = 0 | SelfTelemetry.role_id from effective_role_id; BeaconLogic use telemetry.role_id in Status | In-scope |
| G8 | Hidden hardcoded cadence defaults | BeaconLogic default members 5000/30000 ms; SelfUpdatePolicy 72000/25 — overwritten in init | No bypass; init overwrites from profile | Already correct |
| G9 | User vs radio profile separation | Same Phase B block; shared use_persisted | Resolve role and radio independently; no conflation | In-scope |
| G10 | factory_reset_pointers | Removes pointer keys; writes default role record | Optionally persist 0,0,0,0 so state consistent without next boot | Later / optional |

---

## 4) Implementation summary (minimal #443 delta)

1. **Phase B (app_services.cpp):** Resolve effective_role_id from ptrs alone: if loaded && ptrs.has_current_role && ptrs.current_role_id <= 2 → effective_role_id = ptrs.current_role_id; else effective_role_id = 0. Resolve effective_radio_id from ptrs alone (unchanged logic for V1-A: only 0). When effective_role_id was normalized (we fell back), set profile_record = get_ootb_role_profile(effective_role_id) and save_current_role_profile_record(profile_record). When !profile_valid (existing), same as today. Persist pointers with save_pointers(effective_role_id, effective_radio_id, new_previous_role, new_previous_radio).
2. **SelfTelemetry + role_id:** Add role_id to SelfTelemetry (beacon_logic.h). In AppServices::init set self_telemetry_.role_id = effective_role_id (as uint8_t). In BeaconLogic Status formation use telemetry.role_id instead of 0.
3. **Tests:** Missing/invalid role pointer → effective 0 and persist; active role resolution; cadence from active profile; role_id in Status from telemetry.
4. **Docs:** Sync only if implementation truth diverges from existing canon (no rewrite of canon).

---

## 5) Out of scope (#443)

- #424 radio profile work (no reopen).
- #425 observability except minimal logs/tests for #443.
- #426 docs-only current_state.
- BLE/UI profile editing.
- New user-defined roles beyond S03 (Person/Dog/Infra).
- Protocol work #417–#422 / #435 / #438.
