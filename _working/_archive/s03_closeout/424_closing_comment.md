**Closed via PR #442 (merged).**

- Phase A now uses the **FACTORY radio profile** (id 0, virtual) as the boot baseline source; default preset is **21 dBm** (MIN per canon).
- Phase B normalizes and persists current/previous pointers to 0/0 when missing or invalid; Phase B does not re-apply radio hardware.
- Validation (test_radio_preset, test_role_profile_registry, test_boot_phase_contract, devkit build) passed.
- **User Profile** baseline alignment is intentionally separate follow-up work; not in #424 scope.
