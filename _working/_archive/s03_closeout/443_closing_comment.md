**Closed via PR #444 (merged).**

User Profile baseline is aligned for this scope:
- Phase B resolves, normalizes, and persists role pointers correctly (independent from radio); missing/invalid → default id 0, persisted.
- Status `role_id` now comes from the active user profile (SelfTelemetry) instead of a hardcoded default.
- Validation passed: test_beacon_logic, test_role_profile_registry, devkit build; proof test `test_txq_status_encodes_role_id_from_telemetry`.

Observability and current_state follow-ups remain in #425 / #426.
