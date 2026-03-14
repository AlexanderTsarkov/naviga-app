## Summary

Aligns **User Profile baseline** with default/current canon (#443). Phase B resolves user (role) profile pointers independently from radio pointers, normalizes missing/invalid pointers to default id 0 with persistence, and ensures active user profile values are the single source for runtime cadence/liveness and for Status `role_id`. User profile semantics remain separate from radio profile semantics (#424).

## Phase B changes

- **Role vs radio resolved independently:** `effective_role_id` is derived only from NVS role keys (`has_current_role` and `current_role_id <= 2`). Radio pointer resolution is unchanged (V1-A: only 0). A valid persisted role is now used even when the radio pointer is missing.
- **Normalization and persistence:** Missing or invalid role pointer → `effective_role_id = 0`; normalized state is persisted via `save_pointers`. When the role was normalized or the stored profile record is invalid, the profile record is refreshed from `get_ootb_role_profile(effective_role_id)` and saved.
- **Previous pointers:** `new_previous_role` / `new_previous_radio` still computed from role/radio normalization and persisted.

## Runtime source-of-truth audit

An audit was performed for the values that must come from the **active user profile**:

| Value | Source (post-#443) |
|-------|--------------------|
| `min_interval_s` | Resolved from profile_record in Phase B → runtime |
| `min_displacement_m` | profile_record → SelfUpdatePolicy |
| `max_silence_10s` | profile_record → runtime + self_telemetry_ |
| `role_id` | **Was hardcoded to 0** → now from `effective_role_id` via SelfTelemetry |

The only gap found was **`role_id`** in the Status frame being hardcoded to 0 instead of coming from the active user profile.

## Behavioral fix: Status `role_id` from active profile

- **SelfTelemetry:** Added `role_id`; in Phase B it is set from `effective_role_id` (clamped 0–2).
- **BeaconLogic:** Status (0x07) formation now uses `telemetry.role_id` instead of a literal 0. Active user profile values are thus the source for Status `role_id` on air.

## factory_reset_pointers

- After clearing pointer keys, the default pointers **(0,0,0,0)** are now persisted so the next boot sees a consistent default state without relying on “key absent” behavior.

## Validation

- **test_beacon_logic** (59 tests): passed
- **test_role_profile_registry** (5 tests): passed
- **pio run -e devkit_e220_oled_gnss**: passed
- **Proof test:** `test_txq_status_encodes_role_id_from_telemetry` — Status frame encodes `role_id` from SelfTelemetry (active profile).

## Out of scope (unchanged)

- #424 radio profile work
- #425 observability
- #426 docs-only current_state
- BLE/UI profile editing; protocol work from #417–#422 / #435 / #438

Closes #443
