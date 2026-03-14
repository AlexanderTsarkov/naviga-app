**#443 implemented** in PR [#444](https://github.com/AlexanderTsarkov/naviga-app/pull/444).

**Changes:**
- **Phase B:** Role pointers resolved independently from radio; missing/invalid role → default id 0, persisted. Profile record refreshed from OOTB when role normalized or record invalid.
- **Runtime:** Audit confirmed `min_interval_s`, `min_displacement_m`, `max_silence_10s` already from active profile. **Gap fixed:** `role_id` was hardcoded 0 in Status — now from resolved active user profile via SelfTelemetry.
- **factory_reset_pointers:** Persists 0,0,0,0 so next boot sees consistent default pointer state.
- User profile semantics remain separate from radio profile (#424).

**Validation:** `test_beacon_logic` (59), `test_role_profile_registry` (5), `pio run -e devkit_e220_oled_gnss` passed. Proof test: `test_txq_status_encodes_role_id_from_telemetry`.

**Remains for #425 only:** Observability/logging beyond current Phase B log; no other behavior change in this PR.
