# Radio — RadioProfile product-level model and NVS schema (S03)

**Status:** WIP (schema/spec).  
**Issue:** [#382](https://github.com/AlexanderTsarkov/naviga-app/issues/382) · **Epic:** [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This doc defines the **product-level RadioProfile data model** and **NVS storage schema** for S03 so that FW is ready for S04 BLE/UI (list/read/select/create profiles). It does **not** define BLE protocol or UI; mapping to driver-level params (E220 UART now, SPI later) is adapter responsibility. Semantics align with [radio_profiles_policy_v0](../../../areas/radio/policy/radio_profiles_policy_v0.md); boot placement with [boot_pipeline_v0](../../../areas/firmware/policy/boot_pipeline_v0.md) and [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md).

---

## 1) Purpose and scope

- **Purpose:** Define product-level profile fields, pointer semantics (default/current/previous), baseline vs runtime separation, and NVS schema (namespace, keys, versioning) so that Phase A can apply FACTORY default at boot without NVS, Phase B can persist pointers/records for rollback and future UI, and S04 can list/read/select/create profiles against this schema.
- **Scope:** Schema and model only; no BLE protocol, no UI, no SPI driver implementation. Mapping to E220 UART (and later SPI) is documented as notes; adapter layer owns the mapping.
- **Non-goals:** No automatic TX power control algorithms; no JOIN/Mesh/CAD/LBT. Runtime telemetry vs baseline contract is in [#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384); UART mapping spec is [#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383).

---

## 2) Product-level RadioProfile fields

These fields are **product-level abstractions**. They are **not** SF/BW/CR or register values; the **adapter** (E220 UART, or future SPI) maps them to driver/module parameters.

| Field | Type | Meaning |
|-------|------|--------|
| **profile_id** | uint32 (opaque id) | Stable identifier. 0 = FACTORY default; 1..n = user profiles. |
| **kind** | enum | **FACTORY** = immutable, embedded in FW/product; **USER** = created/edited by user, stored in NVS. |
| **label** | string (optional) | Human-readable name (e.g. "Home", "Long range"). Empty or omitted for FACTORY. |
| **channel_slot** | uint8 | Logical channel slot (product-defined). Mapping to actual frequency is adapter-specific (e.g. E220 CHAN register). |
| **rate_tier** | enum/step (uint8) | Air rate / modulation tier (product-defined step). Mapping to air_data_rate (E220) or SF/BW/CR (SPI) is adapter-specific. |
| **tx_power_baseline_step** | enum/step (uint8) | TX power as product step (e.g. 0..4 for E220 steps 21/24/27/30/33 dBm). Mapping to module is adapter-specific; not all modules expose this via config (E220 UART: module default only in S03 unless product mapping added). |

**Baseline vs runtime:** The values above define the **profile baseline** (stored, user-editable for USER profiles). **Runtime** state (e.g. actual TX power read back from the module, or RSSI/SNR on RX) does **not** overwrite the stored baseline. Runtime is used for telemetry and display only; see [#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384).

**Backend hint (optional):** A profile may carry an optional **backend_hint** (e.g. E220_UART vs SPI) for mapping; if omitted, the active adapter is used. Prefer keeping mapping in the adapter layer and avoid storing backend_hint in NVS unless needed for multi-adapter products.

---

## 3) Pointer semantics

| Pointer | Meaning |
|---------|--------|
| **default_profile_id** | Always **0** (FACTORY). Product constant; need not be stored in NVS. Used when no valid current exists and after factory reset. |
| **current_profile_id** | The profile **active** at boot and at runtime. Persisted in NVS so that next boot restores selection. Phase A applies the **content** of this profile (or FACTORY if invalid/missing) to the radio; Phase B persists the pointer. |
| **previous_profile_id** | The profile that was active before the current one. Used for quick rollback (e.g. "revert to previous"). Cleared on factory reset. Optional; may be 0 or invalid. |

**First-boot behavior:** Phase A does **not** depend on NVS for radio operability. It applies the **FACTORY default profile** (id 0) to the radio so that OOTB comms work. Phase B then loads pointers from NVS (if any); if none or invalid, it sets current_profile_id = 0 and previous_profile_id = 0 and persists them. So first boot: radio already configured in Phase A with factory default; Phase B just persists the pointers for consistency and future UI.

**Factory reset:** Remove or deactivate all USER profiles; set current_profile_id = 0, previous_profile_id = 0; persist. FACTORY profile (0) is never removed.

---

## 4) NVS storage schema (schema-level)

- **Namespace:** `naviga` (same as existing role/radio pointers; see [naviga_storage](../../../../firmware/src/platform/naviga_storage.cpp)).
- **Schema version:** A key **rprof_ver** (uint8 or uint16) MUST be present when any radio profile keys are used. Enables future migration. Recommended value for this schema: **1**.

### 4.1 Pointers (always present when using radio profiles)

| Key | Type | Meaning |
|-----|------|--------|
| **rprof_ver** | uint8/16 | Schema version. Write when first creating or migrating profile storage. |
| **rprof_cur** | uint32 | current_profile_id. |
| **rprof_prev** | uint32 | previous_profile_id. |

Existing implementation uses **prof_cur** and **prof_prev** in the same namespace; migration or alias can map rprof_cur → prof_cur for compatibility. This doc defines the **canonical** names for new code; existing keys may remain until a migration step.

### 4.2 FACTORY profile (id 0)

- **Not stored in NVS.** FACTORY profile is **virtual**: its content is const in firmware (or product registry). It is **addressable by id 0** so that current_profile_id = 0 always resolves to the factory default. Phase A and boot logic resolve id 0 to the embedded default (channel_slot, rate_tier, tx_power_baseline_step).

### 4.3 USER profiles (id 1..n)

- **Stored in NVS** when the product supports user profiles. Each USER profile record contains: profile_id (implied by key), channel_slot, rate_tier, tx_power_baseline_step, label (optional).
- **Key pattern (example):** `rprof_u_<id>_channel`, `rprof_u_<id>_rate`, `rprof_u_<id>_tx`, `rprof_u_<id>_label`, or a single blob key per profile. Exact key layout is implementation-defined; the schema requires that USER profiles can be listed, read, written, and removed by id.
- **Allocation:** A counter or bitmap (e.g. rprof_next_id) can be used to assign new ids. Deleted profiles leave holes unless implementation compacts; policy does not require compaction.

**S03 minimal:** For S03 OOTB-only, implementation may persist **only** pointers (rprof_cur, rprof_prev) and schema_ver; USER profile storage can be stubbed or minimal (zero user profiles) until S04 BLE/UI. Phase A still applies FACTORY (id 0) without reading any profile record from NVS.

---

## 5) Baseline vs runtime separation

- **Baseline:** The stored profile content (channel_slot, rate_tier, tx_power_baseline_step) that is applied to the module at boot or when the user selects a profile. It is **not** mutated by runtime behavior (e.g. RSSI on RX, or module readback of actual TX power).
- **Runtime:** Values observed at runtime (e.g. last TX power read back from module, last_rx_rssi, snr). Used for telemetry and NodeTable/display only. **Must not** overwrite the persisted profile baseline. See [#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384) for the contract.

---

## 6) Mapping notes (adapter responsibility)

Product-level fields are mapped to driver/module parameters by the **adapter** (E220 UART, or future SPI). This section is normative only as "what the product model is"; mapping details are in [#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383) (UART) and future SPI spec.

### 6.1 E220 UART (now)

- **channel_slot** → E220 CHAN register (e.g. slot 1 → 411 MHz). One-to-one or small lookup table.
- **rate_tier** → E220 air_data_rate (SPED bits [2:0]). E.g. tier 0 = 2.4 kbps (air_rate 2), tier 1 = 4.8 kbps (3). Normalization (min 2) applies in adapter.
- **tx_power_baseline_step** → E220 does not expose TX power in UART config frame in current product mapping; document as **module default only** until product defines step mapping (e.g. 0..4 → 21/24/27/30/33). When mapping exists, adapter applies at boot; otherwise runtime telemetry may report "unknown" or module default.
- **RSSI:** Enabling RSSI append is **module-critical** (see [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md)); it affects **runtime** metrics (last_rx_rssi). Not a profile field.
- **SNR:** E220 does not provide SNR; adapter/runtime use sentinel (e.g. NA) for SNR. See link metrics policy.

### 6.2 SPI (later)

- Same product model: **channel_slot**, **rate_tier**, **tx_power_baseline_step** map to SF/BW/CR/preamble and txPower register or equivalent. Adapter owns the mapping.
- RSSI/SNR typically available at runtime; baseline vs runtime contract unchanged.

---

## 7) First-boot and Phase A/B alignment

- **Phase A ([boot_pipeline_v0](boot_pipeline_v0.md) §3):** Hardware provisioning applies the **FACTORY default profile** (id 0) to the radio so that OOTB comms work. No NVS read of profile content is required for Phase A; the factory default is const in FW.
- **Phase B ([boot_pipeline_v0](boot_pipeline_v0.md) §4):** Load pointers (rprof_cur, rprof_prev) from NVS. If missing or invalid, set current = 0, previous = 0 and persist. Resolve current_profile_id to profile content (FACTORY or USER from NVS); if resolution fails, fall back to FACTORY (0) and persist. Phase B does **not** re-apply radio config; Phase A already did that with factory default. Phase B ensures **logical** state (pointers and optional current profile record for cadence/UI) is consistent for rollback and future provisioning.

---

## 8) S04 readiness (list/read/select/create)

The schema and model support:

- **list:** Enumerate profiles: FACTORY (id 0) + all USER profiles present in NVS (ids from key scan or stored list).
- **read:** For a given profile_id, return profile content (channel_slot, rate_tier, tx_power_baseline_step, label). Id 0 → const factory default; id ≥ 1 → read from NVS.
- **select:** Set current_profile_id to given id; set previous_profile_id to previous current; persist pointers.
- **create:** Allocate new USER profile id; write record to NVS (channel_slot, rate_tier, tx_power_baseline_step, label). Optionally select it (select new id).

No BLE protocol or UI is defined here; only that the storage and model support these operations.

---

## 9) Related

- **Radio profiles (semantics):** [radio_profiles_policy_v0](../../../areas/radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)) — default vs user, first boot, factory reset.
- **Boot pipeline:** [boot_pipeline_v0](../../../areas/firmware/policy/boot_pipeline_v0.md) — Phase A applies FACTORY default; Phase B persists pointers/records.
- **Module boot config:** [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md) — module-critical vs profile-applied; FACTORY default applied in Phase A.
- **Provisioning interface:** [provisioning_interface_v0](../../../areas/firmware/policy/provisioning_interface_v0.md) — role/radio get/set/reset; writes pointers.
- **UART mapping spec:** [#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383). **TX power contract (baseline vs runtime):** [#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384).
- **Current NVS impl:** `firmware/src/platform/naviga_storage.{h,cpp}` — namespace `naviga`, keys prof_cur, prof_prev. Product-level struct **RadioProfileRecord** (profile_id, kind, channel_slot, rate_tier, tx_power_baseline_step, label) and **get_factory_default_radio_profile()** added for schema alignment (#382); rprof_ver and USER profile load/save can be added in follow-up. Constants: **kRadioProfileSchemaVersion**, **kRadioProfileIdFactoryDefault**.
