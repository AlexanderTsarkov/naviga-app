# #424 Radio profile baseline — contract and gap table

**Issue:** #424 · **Umbrella:** #416  
**Canon:** provisioning_baseline_v0, boot_pipeline_v0, radio_profiles_model_s03, tx_power_contract_s03, e220_radio_profile_mapping_s03

---

## 1) Contract restatement

**Phase A owns:** Applying the **FACTORY default RadioProfile** (product-level, id 0) to the radio hardware at boot. No NVS read of profile content. Source of truth: product model (channel_slot=1, rate_tier=STANDARD→2.4k, tx_power_baseline_step=0→MIN 21 dBm). Hardware is configured so OOTB comms work.

**Phase B owns:** Loading **current_profile_id** and **previous_profile_id** from NVS; if missing or invalid → set both to **0** (FACTORY) and **persist**. Resolving logical current profile state for cadence/UI. Phase B does **not** re-apply radio hardware config; hardware was already configured in Phase A.

**Must never be conflated:** (1) Logical profile pointers (current/previous id) vs hardware-applied baseline. (2) Baseline (stored in profile, applied at boot) vs runtime-observed values (must not overwrite baseline).

**TX power contract:** Baseline = stored in RadioProfile (tx_power_baseline_step). Runtime = current effective setting. Runtime **must not** overwrite or persist to baseline.

**FACTORY profile (id 0):** Virtual; not stored in NVS; content is const in FW. current_profile_id = 0 always resolves to FACTORY default.

---

## 2) Gap table

| Area | Current | Canon | Gap | Owner |
|------|---------|-------|-----|--------|
| Phase A source | `get_radio_preset(RadioPresetId::Default)` → (2, 1, **30 dBm**) | Phase A applies **FACTORY default** from product model; step 0 = **21 dBm** | Preset is HAL-only, 30 dBm; not derived from product FACTORY default. | #424 |
| Phase A application | E220 apply_critical does not set TX power (advisory in preset) | OOTB MUST use MIN (21 dBm) once mapping implemented | Preset value wrong; E220 UART may not expose TX in config frame — align source to FACTORY and use 21 dBm in preset. | #424 |
| Phase B pointers | load_pointers; if !has_current_radio or invalid id → effective 0,0; save_pointers | Missing/invalid → current=0, previous=0; persist. FACTORY id 0 = virtual. | Logic correct; add explicit normalization comment and ensure previous=0 when normalizing. | #424 (comment/clarity) |
| Logical vs hardware | effective_channel from effective_radio_id (0→1) in Phase B | Phase B does not re-apply radio; logical state only. | Already correct; add short comment that Phase B is pointers only. | #424 |
| Baseline vs runtime | No code overwrites RadioProfileRecord with runtime TX | Runtime must not overwrite baseline. | None found; document invariant. | #424 (doc/test) |
| rprof_ver | Not written | Schema version should be present when using profile keys. | Optional: write rprof_ver on save_pointers. | #424 (minimal) |

---

## 3) Implementation plan (minimal #424 delta)

1. **Phase A:** In `radio_factory`, obtain FACTORY default from product model (`get_factory_default_radio_profile`), build `RadioPreset` from it (channel_slot→channel, rate_tier→air_rate, tx_power_baseline_step 0→21 dBm), call `begin(preset)`. So Phase A explicitly uses FACTORY profile; OOTB preset uses 21 dBm.
2. **HAL preset:** `get_radio_preset(RadioPresetId::Default)` return 21 dBm (not 30) so Default matches FACTORY MIN.
3. **Phase B:** Add explicit comment: current_radio_profile_id 0 = FACTORY (virtual); missing/invalid pointers → normalize to 0/0 and persist. Ensure new_previous_radio is 0 when we normalized (first boot).
4. **rprof_ver:** Optionally write schema version when saving pointers (canon); low risk.
5. **Tests:** Default preset 21 dBm (test_radio_preset); Phase A uses get_factory_default_radio_profile in radio_factory. get_factory_default_radio_profile lives in naviga_storage.cpp (NVS); not linked in native tests; contract and values documented. Baseline vs runtime: no code path overwrites RadioProfileRecord with runtime TX (verified by code review); tx_power_contract_s03 invariant.
---

## 4) Implementation summary (done)

| Change | File | Description |
|--------|------|--------------|
| Phase A source | `radio_factory.cpp` | `preset_from_factory_default()`: get FACTORY from product model (`get_factory_default_radio_profile`), build `RadioPreset` (channel_slot, rate_tier, step→dBm E220 mapping), call `begin(preset)`. Phase A no longer uses HAL-only preset. |
| Default preset | `radio_preset.h` | `get_radio_preset(Default)` tx_power_dbm 30 → 21 (MIN per canon). |
| Phase B comments | `app_services.cpp` | Explicit: current_radio_profile_id 0 = FACTORY (virtual); missing/invalid → normalize 0/0 and persist. Use `kRadioProfileIdFactoryDefault` for default. |
| rprof_ver | `naviga_storage.cpp` | Write `rprof_ver` (schema version) when saving pointers. |
| Test | `test_radio_preset.cpp` | `test_get_radio_preset_default_tx_power_21_dbm`: Default preset = 21 dBm. |

**Validation:** test_radio_preset (incl. Default 21 dBm), test_role_profile_registry, test_boot_phase_contract passed. devkit_e220_oled build passed.

**Remaining for #425 only:** Observability (baseline/runtime/override exposed for telemetry); no scope bleed.

---

## 5) Exit criteria

- [x] #424 contract restated from canon  
- [x] Gap table produced  
- [x] Minimal #424 delta implemented  
- [x] Tests added/updated and passing (Default 21 dBm; Phase B comments)  
- [x] Devkit build passing  
- [x] Docs: 424_contract_and_gaps.md; no canon rewrite  
- [x] Final close recommendation: #424 scope complete; Phase A uses FACTORY product model; Phase B pointers explicit; baseline/runtime separation upheld. #425 for observability.
