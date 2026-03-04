# Radio — TX power contract: baseline, runtime, override (S03)

**Status:** WIP (spec).  
**Issue:** [#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384) · **Epic:** [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This doc defines the **TX power contract** that separates **baseline** (stored in RadioProfile, read/write via future BLE/UI), **runtime** (current effective setting), and **reserved** override semantics for future adaptive/automatic escalation—**without** changing baseline. No algorithms are implemented; only a future-safe contract. Aligns with [radio_profiles_model_s03.md](radio_profiles_model_s03.md) ([#382](https://github.com/AlexanderTsarkov/naviga-app/issues/382)), [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)), [boot_pipeline_v0](../../../areas/firmware/policy/boot_pipeline_v0.md), and [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md).

---

## 1) Purpose and scope

- **Purpose:** Define terms and invariants so that (a) profile baseline TX power is the single source of truth for “user choice,” (b) runtime may diverge in future (e.g. adaptive escalation) without mutating baseline, and (c) override semantics are reserved for future use. Support observability (baseline + runtime + override state) and embedded fault handling.
- **Scope:** Contract and policy only; no code, no BLE protocol/UI design, no automatic TX power algorithm implementation (only reserved semantics).
- **Non-goals:** No JOIN/Mesh/CAD/LBT; no implementation of adaptive escalation in S03.

---

## 2) Terms

| Term | Meaning |
|------|--------|
| **baseline_tx_power_step** | TX power step **stored** in the active RadioProfile (e.g. `RadioProfileRecord.tx_power_baseline_step`). Read/write via future BLE/UI; UI may hide but **must** remain writable for provisioning and diagnostics. Source of truth for “user/profile choice.” |
| **runtime_tx_power_step** | The **actual current** backend (module) setting. May equal baseline or, in future, differ (e.g. after adaptive escalation). Not persisted as profile content; may be unknown if backend does not support readback. |
| **effective_tx_power_step** | The step **used for TX** at any moment. In S03: equals runtime when known, else baseline. In future: may be subject to override (see below). |
| **override_active** | **(RESERVED)** Future flag: when true, effective TX power is temporarily above baseline (e.g. adaptive escalation). **Not implemented** in S03; semantics reserved so that baseline is never overwritten by runtime/adaptive behavior. |
| **override_reason** | **(RESERVED)** Future enum or code (e.g. “session missing,” “addressed request”). **Not implemented** in S03. |

**Note:** The RadioProfile model uses the field name `tx_power_baseline_step`; it is the stored baseline. This contract uses **baseline_tx_power_step** as the conceptual term; they refer to the same value.

---

## 3) Invariants

1. **Boot:** Apply **baseline** from the active profile to the radio. OOTB FACTORY default uses **step 0 = MIN** (21 dBm) per [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)) and [boot_pipeline_v0](../../../areas/firmware/policy/boot_pipeline_v0.md) Phase A.
2. **Baseline mutability:** Baseline is changed **only** by profile write, profile switch, or factory reset. **Never** by runtime behavior, adaptive logic, or module readback.
3. **Runtime vs baseline:** Runtime may change in future (e.g. adaptive escalation). Runtime **must not** overwrite or persist to the profile baseline. When override is cleared or on reboot, effective reverts to baseline unless future policy says otherwise.
4. **Reboot behavior:** On reboot, **default to baseline**; any runtime override is cleared. Effective TX power after boot is the baseline from the active profile (or FACTORY default on first boot). Future policy may define explicit “persist override until…”; for S03, reboot clears any reserved override state.

---

## 4) Observability and telemetry requirements

- The system **must** expose:
  - **Baseline:** Current profile’s baseline TX power step (from `RadioProfileRecord` or equivalent).
  - **Runtime:** Current effective/runtime TX power step, or **unknown** if the backend does not support readback or readback failed.
  - **override_active:** **(Reserved)** Exposed for future use; in S03 always false / not implemented.
- **Embedded fault handling:** If backend read or apply of TX power fails (e.g. E220 write/readback failure), the device must **continue best-effort** (no brick) and set a **diag flag** (observable fault state) per [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md) §4. Telemetry may report runtime = unknown and/or a fault code; baseline remains unchanged.

---

## 5) Backend notes

- **E220 UART:** Step mapping and OOTB MIN requirement are defined in [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)). Power-level count differs by module: T30D = 4 levels, T33D = 5 levels; clamp rules apply. Same baseline/runtime/override contract holds.
- **SPI (later):** Driver-defined step-to-dBm mapping; **same contract** (baseline stored in profile, runtime may differ, no overwrite of baseline; override semantics reserved).

---

## 6) Future note: adaptive escalation (out of scope for S03)

**Planned use-case (non-goal for S03):** In a future iteration, the system may implement **adaptive TX power escalation**—e.g. when a session or an important node is missing, stepwise increase of effective TX power (within hardware limits) plus addressed request, then step back down when link is restored. This contract reserves semantics so that:

- **Baseline** stays the stored user/profile choice and is **never** overwritten by adaptive behavior.
- **Runtime** / **effective** may temporarily exceed baseline while **override_active** (and optionally **override_reason**) are set.
- On reboot or when override is cleared, effective reverts to baseline.

No algorithm is specified or implemented in S03; only the contract is defined to keep baseline authoritative and future-safe.

---

## 7) Related

- **RadioProfile model:** [radio_profiles_model_s03.md](radio_profiles_model_s03.md) ([#382](https://github.com/AlexanderTsarkov/naviga-app/issues/382)) — `tx_power_baseline_step` in profile; baseline vs runtime separation.
- **E220 mapping:** [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)) — step-to-dBm mapping, OOTB MIN.
- **Boot:** [boot_pipeline_v0](../../../areas/firmware/policy/boot_pipeline_v0.md) (Phase A apply FACTORY default; Phase B pointers).
- **Failure/observable fault:** [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md) §4 — RepairFailed, best-effort, diag flag.
