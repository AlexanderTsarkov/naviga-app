# Firmware — S03 Provisioning Baseline v0

**Status:** Canon (policy).  
**Issue:** [#385](https://github.com/AlexanderTsarkov/naviga-app/issues/385) · **Epic:** [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This document is the **S03 Provisioning Baseline** deliverable for epic [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353). It answers in one place: **what** gets provisioned (hardware + radio profile + role), **where** it is stored (ESP NVS vs module config), **when** it is applied/read during boot (Phase A vs Phase B), **fallback** behavior on first boot and on failures, and **how** it affects OOTB start and NodeTable/telemetry (consumer pointers). Every claim is backed by cross-links to existing policy/spec docs.

---

## 1) Scope (S03) and non-goals

**Scope (S03):**

- **Hardware provisioning:** GNSS and E220 radio module bring-up, verify-and-heal, FACTORY default RadioProfile applied at boot so that OOTB comms work without NVS or UI.
- **Product provisioning:** Role and radio profile pointers (and optional records) persisted in ESP NVS for rollback and future UI; persistence is **not** a prerequisite for first-boot OOTB.
- **Storage and contracts:** NVS schema for role/radio pointers and records; baseline vs runtime TX power contract; mapping of product-level profile to E220 (channel, rate, tx power).

**Non-goals:**

- No code changes; no BLE protocol/UI; no JOIN/Mesh/CAD/LBT; no legacy BLE changes. No automatic TX power algorithm implementation—only reference to the [TX power contract](https://github.com/AlexanderTsarkov/naviga-app/issues/384) ([tx_power_contract_s03.md](../../../wip/areas/radio/policy/tx_power_contract_s03.md)).

---

## 2) Definitions: Phase A vs Phase B

| Phase | Name | Responsibility |
|-------|------|----------------|
| **Phase A** | **Hardware provisioning** | Bring GNSS and radio module to required Naviga operating mode: verify config on every boot, repair on mismatch. Apply **FACTORY default RadioProfile** (channel_slot=1, rate_tier=STANDARD→2.4k, tx_power step0=MIN 21 dBm) so that OOTB comms work. No NVS read of profile content required for Phase A. See [boot_pipeline_v0](boot_pipeline_v0.md) §3 and [module_boot_config_v0](module_boot_config_v0.md). |
| **Phase B** | **Product provisioning + persistence** | Load (or default) **role** and **radio profile** pointers from NVS; persist default/current/previous so that rollback and future UI/BLE can use them. Radio hardware is already configured in Phase A; Phase B ensures logical state (pointers/records) is consistent. Persistence is **not** required for OOTB on first boot. See [boot_pipeline_v0](boot_pipeline_v0.md) §4. |

**Phase C** (Start comms) uses the provisioned role and current profile; it does not change provisioning. See [boot_pipeline_v0](boot_pipeline_v0.md) §5.

---

## 3) Phase A: Hardware provisioning

### 3.1 GNSS bring-up (verify/heal summary)

- **Strategy:** Minimal verify-on-boot: open GNSS UART at product baud, send required UBX config (e.g. NAV-PVT). **Verify** = at least one byte received within timeout (link alive). **Repair** = re-send config and retry. No readback of baud/protocol from receiver.
- **Outcomes:** Ok (link responsive), Repaired (retry succeeded), RepairFailed (link not responsive after retries). RepairFailed MUST NOT brick; set observable fault state; continue best-effort. See [module_boot_config_v0](module_boot_config_v0.md) §3 and §4.

### 3.2 E220 bring-up (verify/heal summary)

- **Strategy:** On every boot, **read** module config (getConfiguration). If critical parameters do not match expected (from FACTORY default profile where applicable), **apply** correct values (setConfiguration) and **re-read** to verify.
- **Module-critical:** UART baud/parity, **RSSI append ON** (required for link metrics; receiver strips appended RSSI bytes and stores separately), LBT OFF, sub-packet, air_data_rate, channel. See [module_boot_config_v0](module_boot_config_v0.md) §2.
- **Profile-applied (FACTORY default in Phase A):** Channel (channel_slot=1), air_data_rate (STANDARD→2.4k), **tx power baseline step0 = MIN (21 dBm)**. E220 UART backend has no SNR; RSSI only. See [e220_radio_profile_mapping_s03.md](../../../wip/areas/radio/policy/e220_radio_profile_mapping_s03.md).
- **Outcomes:** Ok / Repaired / RepairFailed. See §3.4 for failure behavior.

### 3.3 OOTB invariant

By end of Phase A, the **radio** MUST be configured for OOTB comms using the **FACTORY default RadioProfile**:

- **channel_slot** = 1 (slot 0 reserved for dev/test).
- **rate_tier** = STANDARD → 2.4 kbps (E220 air_data_rate 010).
- **tx_power_baseline_step** = 0 (MIN) → 21 dBm. Module default (30 dBm) is **not** acceptable for OOTB; product requires MIN at boot once mapping is implemented. See [boot_pipeline_v0](boot_pipeline_v0.md) §3 and [e220_radio_profile_mapping_s03.md](../../../wip/areas/radio/policy/e220_radio_profile_mapping_s03.md) §4.

### 3.4 Failure behavior (Ok / Repaired / RepairFailed)

- **Ok:** No repair needed; module config already matched expected state.
- **Repaired:** Mismatch detected and successfully corrected; readback verify passed.
- **RepairFailed:** Apply failed or readback verify failed. **MUST NOT brick the device.** FW MUST continue best-effort (e.g. Phase B and Phase C may run with degraded state). FW MUST set an **observable fault state** (e.g. boot_config_result = RepairFailed) for **progressive signaling**. See [module_boot_config_v0](module_boot_config_v0.md) §4.

---

## 4) Phase B: Product provisioning + persistence

### 4.1 Role pointers and role record

- **Pointers in NVS:** CurrentRoleId, PreviousRoleId. Phase B **reads** them; if missing or invalid → apply **default role** and **persist**. Default role is immutable (embedded in FW). Role record (cadence params) may be stored in NVS or resolved from default. See [provisioning_interface_v0](provisioning_interface_v0.md) and [boot_pipeline_v0](boot_pipeline_v0.md) §4.
- **Persistence is for rollback and future UI;** first-boot OOTB does not depend on persisted role—default is applied and then persisted so that state is consistent.

### 4.2 Radio profile pointers and schema

- **Pointers in NVS:** current_profile_id (rprof_cur / prof_cur), previous_profile_id (rprof_prev / prof_prev). Schema version: rprof_ver. **FACTORY profile (id 0)** is **virtual** (not stored in NVS; const in FW). Phase B **reads** pointers; if missing or invalid → set current = 0, previous = 0 and **persist**. See [radio_profiles_model_s03.md](../../../wip/areas/radio/policy/radio_profiles_model_s03.md) and [boot_pipeline_v0](boot_pipeline_v0.md) §4.
- **Persistence is not required for first-boot OOTB:** Phase A applied FACTORY default; Phase B only ensures pointers/records are consistent for cadence, rollback, and later UI.

---

## 5) Storage summary

### 5.1 ESP NVS namespace `naviga` (key summary)

| Key (existing / S03) | Type | Meaning |
|----------------------|------|--------|
| **role_cur** | uint32 | Current role id. |
| **role_prev** | uint32 | Previous role id (rollback). |
| **prof_cur** / **rprof_cur** | uint32 | Current radio profile id (canonical: rprof_cur). |
| **prof_prev** / **rprof_prev** | uint32 | Previous radio profile id. |
| **rprof_ver** | uint8/16 | Radio profile schema version. |
| **role_interval_s**, **role_silence_10**, **role_dist_m** | — | Role record (cadence params). |
| **rprof_u_&lt;id&gt;_*** | — | USER profile records (id ≥ 1); S03 minimal may omit. |

See [radio_profiles_model_s03.md](../../../wip/areas/radio/policy/radio_profiles_model_s03.md) §4 for full schema.

### 5.2 E220 module vs ESP NVS

| Stored where | What |
|--------------|------|
| **E220 module (config)** | Channel (CH), air_data_rate (SPED), TX power bits, UART baud/parity, RSSI enable, LBT, sub-packet, etc. Applied in Phase A from FACTORY default; verify-and-repair every boot. See [module_boot_config_v0](module_boot_config_v0.md) §2. |
| **ESP NVS** | Role and radio profile **pointers** (and role record, optional USER profile records). Pointers are read in Phase B; provisioning interface writes them. See [provisioning_interface_v0](provisioning_interface_v0.md). |

---

## 6) Fallback rules

| Scenario | Behavior |
|----------|----------|
| **First boot (no NVS)** | Phase A: Apply FACTORY default RadioProfile to radio. Phase B: No valid pointers → set current_role_id and current_profile_id to defaults (0); **persist**. OOTB comms work without any prior NVS. |
| **Missing or invalid NVS pointers** | Phase B: Treat as "no valid current"; apply default role and default radio profile (id 0); persist pointers. Radio was already configured in Phase A with FACTORY default. |
| **RepairFailed (GNSS or E220)** | Do **not** brick. Set **observable fault state**. Continue best-effort. See [module_boot_config_v0](module_boot_config_v0.md) §4. |

---

## 7) Effects / consumers

### 7.1 OOTB autonomous start

- After Phase A and Phase B, the device has: (1) radio configured with FACTORY default, (2) role and radio profile pointers resolved and persisted. Phase C starts Alive/Beacon cadence using **provisioned** role and **provisioned** current profile. **Trigger/sequence/fallback and NodeTable effects:** [ootb_autonomous_start_v0](ootb_autonomous_start_v0.md) ([#354](https://github.com/AlexanderTsarkov/naviga-app/issues/354)).

### 7.2 NodeTable / telemetry (consumer pointers)

- Provisioned values feed **self** defaults and **exported** fields in NodeTable/telemetry. **Link:** [NodeTable master table README](../../../wip/areas/nodetable/master_table/README.md), [fields_v0_1.csv](../../../wip/areas/nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../../wip/areas/nodetable/master_table/packets_v0_1.csv).

### 7.3 Baseline vs runtime TX power

- **Baseline** = stored in RadioProfile (tx_power_baseline_step); applied at boot. **Runtime** = current effective setting. Runtime **must not** overwrite baseline. See [tx_power_contract_s03.md](../../../wip/areas/radio/policy/tx_power_contract_s03.md) ([#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384)).

---

## 8) Related docs and issues

| Doc / issue | Description |
|-------------|-------------|
| [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353) | Epic: S03 Provisioning baseline. |
| [#385](https://github.com/AlexanderTsarkov/naviga-app/issues/385) | This deliverable. |
| [boot_pipeline_v0](boot_pipeline_v0.md) | Phase A/B/C ordering and invariants. |
| [module_boot_config_v0](module_boot_config_v0.md) | GNSS and E220 verify/heal; failure behavior §4. |
| [provisioning_interface_v0](provisioning_interface_v0.md) | Serial provisioning commands; Phase B reads pointers. |
| [ootb_autonomous_start_v0](ootb_autonomous_start_v0.md) ([#354](https://github.com/AlexanderTsarkov/naviga-app/issues/354)) | OOTB autonomous start: trigger, sequence, fallback, when we start sending what. |
| [fw_version_id_semantics_v0](fw_version_id_semantics_v0.md), [hw_profile_id_semantics_v0](hw_profile_id_semantics_v0.md) | Build/hardware identity semantics; link to [fw_version_id_registry_v0](../registry/fw_version_id_registry_v0.md), [hw_profile_id_registry_v0](../registry/hw_profile_id_registry_v0.md). |
