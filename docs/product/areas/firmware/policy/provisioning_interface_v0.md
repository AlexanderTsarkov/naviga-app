# Firmware — Provisioning interface v0 (serial-only)

**Status:** Canon (policy).  
**Work Area:** Product Specs WIP · **Parent:** [#221](https://github.com/AlexanderTsarkov/naviga-app/issues/221) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines the **v0 provisioning interface**: **serial console commands** to set role and radio profile pointers, reset to defaults, and query status. It enables **one firmware image** for V1-A: the device is configured per-unit via provisioning; boot pipeline ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) reads persisted pointers in Phase B before Start comms (Phase C). Role semantics: [role_profiles_policy_v0](../../../wip/areas/domain/policy/role_profiles_policy_v0.md) ([#219](https://github.com/AlexanderTsarkov/naviga-app/issues/219)). Radio profile semantics: [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)). This doc does not change those policies; it defines the **interface** that writes the pointers they rely on.

---

## 1) Purpose

- **Purpose:** Specify the **normative** serial command set for provisioning (role get/set/reset, radio get/set/reset, factory reset, status), **persistence rules** (what is written, when), and **relationship to boot pipeline** so that one FW image can be used for all V1-A devices and configuration is applied via provisioning before comms start.
- **Scope (v0):** Serial console only; no BLE bridge, no UI. Commands and expected behavior are normative; exact prompt/syntax (e.g. line endings) may be implementation-defined within the contract below.

---

## 2) Non-goals

- No [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) channel discovery; no BLE bridge; no mesh/JOIN; no backend.
- No implementation of the commands in this issue — docs-first; code later.
- No changes to [role_profiles_policy_v0](../../../wip/areas/domain/policy/role_profiles_policy_v0.md), [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md), or [boot_pipeline_v0](boot_pipeline_v0.md) — only references.

---

## 3) Concepts

### 3.1 Pointers (persisted)

| Pointer | Meaning | Policy source |
|---------|--------|----------------|
| **CurrentRoleId** | Persisted id/handle of the **current** role profile (Person/Dog/Infra or user-defined). Used on boot to resolve operational params (minIntervalSec, minDisplacementM, maxSilence10s). | [#219](https://github.com/AlexanderTsarkov/naviga-app/issues/219) |
| **PreviousRoleId** | Optional persisted id of the previously active role; used for rollback UX. Cleared on role reset and factory reset. | [#219](https://github.com/AlexanderTsarkov/naviga-app/issues/219) |
| **CurrentRadioProfileId** (CurrentProfileId) | Persisted id/handle of the **current** radio profile (channel, modulation preset, txPower). Used on boot for Phase C comms. | [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211) |
| **PreviousRadioProfileId** (PreviousProfileId) | Optional persisted id of the previously active radio profile; cleared on radio reset and factory reset. | [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211) |

### 3.2 Defaults and factory reset

- **Default role** and **default radio profile** are **immutable** (embedded in firmware / product). They are defined in [role_profiles_policy_v0](../../../wip/areas/domain/policy/role_profiles_policy_v0.md) and [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md).
- **Factory reset** (normative): Remove or deactivate all **user** roles and **user** radio profiles; set **CurrentRoleId** and **CurrentRadioProfileId** to the default role and default radio profile; clear **PreviousRoleId** and **PreviousRadioProfileId**. After factory reset, next boot uses defaults only.

---

## 4) Commands (normative)

### 4.1 Role

| Command | Expected behavior |
|---------|-------------------|
| **role get** | Return the **current role** (CurrentRoleId resolved to a display form, e.g. person \| dog \| infra or a stable id). MUST reflect persisted CurrentRoleId. |
| **role set \<person\|dog\|infra\>** | Set current role to the given OOTB role. **MUST** persist **CurrentRoleId** immediately (pointer to that role). MAY set PreviousRoleId to the previous current. Invalid argument → error; no change to persisted state. |
| **role reset** | Set CurrentRoleId to the **default role** and persist; clear PreviousRoleId. Semantics per [role_profiles_policy_v0](../../../wip/areas/domain/policy/role_profiles_policy_v0.md) factory reset for role only. |

### 4.2 Radio

| Command | Expected behavior |
|---------|-------------------|
| **radio get** | Return the **current radio profile** (CurrentProfileId resolved to display form, e.g. profileId or channel/preset/txPower). MUST reflect persisted CurrentRadioProfileId. |
| **radio set \<profileId\>** | Set current radio profile to the given profile (by id). **MUST** persist **CurrentRadioProfileId** immediately. MAY set PreviousRadioProfileId to the previous current. If profileId is not yet implemented, **radio set \<channel\> \<preset\> \<txPower\>** (or equivalent) is allowed as alternative; implementation MUST persist a valid CurrentRadioProfileId. Invalid args → error; no change. |
| **radio reset** | Set CurrentRadioProfileId to the **default radio profile** and persist; clear PreviousRadioProfileId. Semantics per [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md) factory reset for radio only. |

### 4.3 Factory reset and status

| Command | Expected behavior |
|---------|-------------------|
| **factory reset** | Perform full factory reset: clear user roles and user radio profiles (or mark inactive); set CurrentRoleId and CurrentRadioProfileId to defaults; clear PreviousRoleId and PreviousRadioProfileId. Persist immediately. Semantics per [#219](https://github.com/AlexanderTsarkov/naviga-app/issues/219) and [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211). |
| **status** | Print **at least**: current role (resolved), current radio profile (resolved), GNSS fix state (fix \| no-fix). **Optional but recommended:** lastRxAt and lastTxAt (or equivalent counters) for diagnostics; ref [PR #205](https://github.com/AlexanderTsarkov/naviga-app/pull/205) RX semantics. Format is implementation-defined; the listed fields are the **minimum contract**. |

---

## 5) Persistence rules

- **Immediate persistence:** Commands that **set** or **reset** role or radio (role set, role reset, radio set, radio reset, factory reset) MUST persist the affected pointers **before** returning success. No "pending write" or batch; provisioning result is durable so that a reboot immediately after uses the new values.
- **Idempotency:** **role set X** when current is already X (and **radio set Y** when current is already Y) MAY succeed without error and leave state unchanged; implementation may treat as no-op.
- **Error handling:** Invalid arguments (e.g. unknown role name, invalid profileId) MUST NOT change persisted pointers. Implementation MUST return an error indication and leave state unchanged.

---

## 6) Boot pipeline interaction

- **Phase B** of [boot_pipeline_v0](boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) **reads** persisted CurrentRoleId and CurrentRadioProfileId (and applies first-boot rule if missing). **Phase C** (Start comms) uses **only** those provisioned values.
- **Provisioning** (this interface) **writes** those pointers. Provisioning may be used **before** first boot (e.g. at factory) or **after** boot (serial console at runtime). Whenever provisioning runs, the next **boot** (or the next time Phase B is re-evaluated, if applicable) will use the updated pointers. For V1-A, **comms must not start until** Phase B has run with valid pointers; provisioning that runs at runtime updates pointers for the **next** boot unless implementation defines an explicit "apply now" path (out of scope for this doc).

---

## 7) Examples (informative)

- **Set role to Person:** `role set person` → CurrentRoleId persisted to Person; next boot uses Person operational params.
- **Set role to Dog:** `role set dog` → CurrentRoleId persisted to Dog (minIntervalSec=9, minDisplacementM=15, maxSilence10s=3 per [role_profiles_policy_v0](../../../wip/areas/domain/policy/role_profiles_policy_v0.md) OOTB).
- **Select default radio profile:** `radio reset` or `radio set default` (if supported) → CurrentRadioProfileId points to default; next boot uses default channel/preset/txPower.
- **Factory reset:** `factory reset` → all user roles/profiles cleared or deactivated; CurrentRoleId and CurrentRadioProfileId set to defaults; device behaves as OOTB after next boot.

---

## 8) Related

- **Boot pipeline:** [boot_pipeline_v0](boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) — Phase B reads pointers; Phase C starts comms.
- **Role profiles:** [role_profiles_policy_v0](../../../wip/areas/domain/policy/role_profiles_policy_v0.md) ([#219](https://github.com/AlexanderTsarkov/naviga-app/issues/219)) — defaults, persistence, factory reset semantics.
- **Radio profiles:** [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)) — defaults, persistence, factory reset semantics.
