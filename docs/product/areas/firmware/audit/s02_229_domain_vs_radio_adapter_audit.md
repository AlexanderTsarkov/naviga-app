# S02.5 — Audit: domain vs radio adapter boundaries (E220Radio as IRadio)

**Status:** Canon (audit record).  
**Date:** 2026-02-22  
**Issue:** [#229](https://github.com/AlexanderTsarkov/naviga-app/issues/229) · **Epic:** [#224](https://github.com/AlexanderTsarkov/naviga-app/issues/224) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Promotion:** [#261](https://github.com/AlexanderTsarkov/naviga-app/issues/261)

**Goal/Scope:** Record the S02.5 audit findings: IRadio contract vs E220Radio implementation; domain radio-agnostic; M1Runtime as wiring point. No code or HAL changes in the audit itself. Follow-up policy (V1-A boundaries and init flags design) is in [radio_adapter_boundary_v1a.md](../policy/radio_adapter_boundary_v1a.md).

---

## 1) Audit scope (restatement)

- **E220Radio** is the single **IRadio** adapter for V1-A; it wraps the E220 UART modem (library `LoRa_E220`), not a chip-level SPI driver.
- **Domain** must not depend on concrete radio types or E220-specific enums; only on **IRadio** (and optional **IChannelSense**) from HAL.
- **M1Runtime** is the wiring point: it receives `IRadio*`, `radio_ready`, and `rssi_available` at init and uses only `IRadio::send`, `recv`, `last_rssi_dbm()` in the tick path.
- **Boot/provisioning:** Phase A (module boot/verify-repair), Phase B (role + radio profile pointers from NVS), Phase C — aligned with boot_pipeline and radio_profiles_policy_v0.

---

## 2) Matches (clean boundaries)

| Area | Evidence (file paths / symbols) |
|------|----------------------------------|
| **Domain layer** | No E220 or IRadio in domain. `firmware/src/domain/` — only `log_events.h` uses names `RADIO_TX_OK`, `RADIO_TX_ERR`, `RADIO_RX_OK`, `RADIO_RX_ERR` (domain vocabulary for logging). No `#include` of `e220_radio.h` or `naviga/hal/interfaces.h` in any file under `domain/`. |
| **M1Runtime** | `firmware/src/app/m1_runtime.h` / `.cpp`: holds `IRadio* radio_`; `init(..., IRadio* radio, bool radio_ready, bool rssi_available, ...)`. Uses only `radio_->send()`, `radio_->recv()`, `radio_->last_rssi_dbm()`. No E220 type or include. |
| **RadioSmokeService** | `firmware/src/services/radio_smoke_service.h` / `.cpp`: `init(IRadio* radio, ...)`; uses only `IRadio*`. No E220. |
| **Wiring** | `firmware/src/app/app_services.cpp`: single place that instantiates `E220Radio` and calls `begin()`, `last_boot_config_result()`, `last_boot_config_message()`, `rssi_available()`; then passes `IRadio*` + bools to `runtime_.init()`. Correct: wiring knows concrete type. |
| **IRadio implementation** | `firmware/src/platform/e220_radio.cpp`: implements `send`, `recv`, `last_rssi_dbm()`; extends with `is_ready()`, `rssi_available()`, `last_boot_config_*()` used only from app_services. |
| **Config persistence** | `firmware/src/platform/naviga_storage.*`: role/radio profile pointers (current/previous); no E220 types. Aligned with radio_profiles_policy_v0 and boot pipeline Phase B. |
| **DeviceInfo / BLE** | `device_info.radio_module_model = "E220"` set in `app_services.cpp`; protocol field is generic (`protocol/ble_node_table_bridge.h`: `radio_module_model`). Value is wiring concern. |
| **IChannelSense** | E220 passes `nullptr` in `app_services.cpp` (`runtime_.init(..., nullptr)`). HAL has `IChannelSense`; E220 documented UNSUPPORTED; no domain dependency on sense. |

---

## 3) Gaps / violations (as of audit date)

| # | Location | Issue | Minimal fix approach |
|---|----------|--------|------------------------|
| **G1** | **ProvisioningShell** / **ProvisioningAdapter** | Public API was E220-named: `set_e220_boot_info(...)`, `e220_result_str()`, members `e220_result_`, `e220_message_`. Comment in `provisioning_shell.h` said "Platform-agnostic" but the API leaked E220. | Rename to radio-generic (e.g. `set_radio_boot_info`). *Addressed in [#258](https://github.com/AlexanderTsarkov/naviga-app/issues/258).* |
| **G2** | **IRadio vs docs** | `docs/firmware/hal_contracts_v0.md` describes: `send()` → **RadioTxResult**, **getRadioStatusSnapshot()**, **getRadioCapabilityInfo()**. Current IRadio: `bool send()`, `recv()`, `last_rssi_dbm()` only. | Document as intentional V1-A simplification. *Addressed in [radio_adapter_boundary_v1a.md](../policy/radio_adapter_boundary_v1a.md) and [#259](https://github.com/AlexanderTsarkov/naviga-app/issues/259).* |
| **G3** | **radio_ready / rssi_available** | Not part of IRadio; passed into `M1Runtime::init()` from wiring. E220Radio has `is_ready()` and `rssi_available()`; IRadio does not. | Document design: adapter-specific capabilities supplied at init by wiring. *Addressed in [radio_adapter_boundary_v1a.md](../policy/radio_adapter_boundary_v1a.md) and [#260](https://github.com/AlexanderTsarkov/naviga-app/issues/260).* |

---

## 4) Risks (future footguns)

- **Adding new radio adapters:** Generic naming in provisioning (post-#258) reduces coupling; any new adapter should use the same generic boot-info API.
- **HAL contract drift:** If later we add RadioTxResult or getRadioStatusSnapshot to HAL, E220Radio (and any other adapter) must map to the same contract; current bool send() is sufficient for V1-A but doesn’t match the broader hal_contracts doc.
- **Boot result in BLE/status:** If "radio boot" is exposed in a generic BLE characteristic, use generic result values (Ok/Repaired/RepairFailed), not E220-named in the API.

---

## 5) Alignment with canon

- **radio_profiles_policy_v0:** Default profile, persistence of current/previous pointers — implemented via `naviga_storage` and Phase B in app_services; no gap.
- **e220_radio_implementation_status:** E220 as IRadio adapter, no chip-level driver, channel sense UNSUPPORTED — matches.
- **hal_contracts_v0:** IRadio conceptual contract is richer than current interface; see G2. Policy doc [radio_adapter_boundary_v1a.md](../policy/radio_adapter_boundary_v1a.md) records V1-A simplification.

---

## 6) Summary

- **Domain:** Clean; no E220 or IRadio types in `firmware/src/domain/`.
- **Wiring:** Correct; only `app_services.cpp` and `e220_radio.*` use E220 types.
- **Gaps:** G1 addressed by #258; G2/G3 addressed by radio_adapter_boundary_v1a + #259/#260.
- **Risks:** HAL doc vs implementation drift; keep provisioning/status API generic.

No architecture rewrite; minimal adaptations (rename + docs) sufficient for V1-A boundary clarity.
