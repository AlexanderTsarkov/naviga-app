**PR:** #442 — feat(#424): align radio profile baseline with FACTORY/current canon

**Summary of changes:**
- **Phase A:** FACTORY default RadioProfile (id 0, virtual, not in NVS) is now the source for radio boot baseline. `radio_factory` uses `get_factory_default_radio_profile()` and builds the HAL preset from it. Default preset tx_power_dbm changed from 30 to **21 dBm** (MIN per canon).
- **Phase B:** current/previous radio profile pointers normalize to 0/0 when missing or invalid; normalized pointers are persisted. Phase B does not re-apply radio hardware. `rprof_ver` is written when saving pointers.
- Baseline vs runtime TX power remains separated (no conflation).

**Validation:** test_radio_preset, test_role_profile_registry, test_boot_phase_contract passed; pio run -e devkit_e220_oled passed. Note: `get_factory_default_radio_profile()` lives on the ESP32/NVS side and is not linked in native tests; semantics are aligned and devkit build is green.

**User Profile baseline** will be handled separately after #424; not in scope for this PR.
