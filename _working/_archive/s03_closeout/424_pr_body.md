## Summary

Aligns radio profile baseline semantics with canon (#424). Phase A uses the FACTORY default RadioProfile (id 0) as the source for radio boot; Phase B normalizes and persists current/previous pointers; baseline and runtime TX power remain separated.

**Scope (this PR only):** #424. No User Profile baseline work; no #425/#426.

---

## Phase A

- **FACTORY default (id 0)** is now the explicit source for radio boot baseline. `radio_factory` calls `get_factory_default_radio_profile()` and builds the HAL preset from that product-level record (channel_slot, rate_tier, tx_power_baseline_step → E220 mapping).
- **Default preset** now uses **21 dBm** (MIN) instead of 30 dBm, per provisioning_baseline_v0 and e220_radio_profile_mapping_s03. OOTB requires MIN at boot; module default 30 dBm is not acceptable per canon.
- **Note:** `get_factory_default_radio_profile()` lives in `naviga_storage.cpp` (ESP32/NVS side) and is **not** linked in native tests. Native tests cover the Default preset value (21 dBm) and Phase B pointer semantics; the full factory-profile source path is exercised in the devkit build.

---

## Phase B

- **current_radio_profile_id** and **previous_radio_profile_id** normalize to **0/0** when missing or invalid; normalized values are persisted. Profile id **0 = FACTORY** (virtual, not stored in NVS).
- Phase B does **not** re-apply radio hardware; hardware is configured in Phase A.
- **rprof_ver** (schema version) is now written when saving pointers.

---

## Baseline vs runtime TX power

- Baseline (stored in profile, applied at boot) and runtime (effective setting) remain separated; runtime does not overwrite baseline. No change to that contract in this PR.

---

## Validation

- **test_radio_preset** — passed (includes Default 21 dBm).
- **test_role_profile_registry** — passed.
- **test_boot_phase_contract** — passed.
- **pio run -e devkit_e220_oled** — passed.

Semantics are aligned with canon; devkit build is green. No claim of full native end-to-end coverage of `get_factory_default_radio_profile()` (it is ESP32-only).

---

## Out of scope

- **User Profile** baseline alignment is intentionally out of scope and will be handled separately after #424.
- #425 / #426 — not in this PR.

---

## Refs

- Closes #424.
- Canon: provisioning_baseline_v0, boot_pipeline_v0, radio_profiles_model_s03, tx_power_contract_s03, e220_radio_profile_mapping_s03.
