# Domain — Node role/type review (S03)

**Status:** WIP (spec).  
**Issue:** [#357](https://github.com/AlexanderTsarkov/naviga-app/issues/357) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This doc reviews and formalizes the **node role/type** concept for S03: what “role/type” means at product level, which parameters firmware uses (min_interval_s, min_displacement_m, max_silence_10s), how role is stored and applied at boot (Phase B), and how it is exposed in NodeTable and on-air packets. It aligns with S03 OOTB/default constraints and existing provisioning/boot policies. **User-defined role changes and UI/protocol for roles are NOT in S03** — only OOTB/default.

---

## 1) S03 scope and constraints

- **In scope (S03):** OOTB/default role set and behavior parameters; how role is provisioned, persisted, and applied at boot; how role-derived params (maxSilence10s, cadence) are consumed by NodeTable and on-air. **Effective values only** — no “speed” or other derived models as stored fields; firmware uses the three normative params.
- **Out of scope (S03):** User-defined role creation/editing; BLE/UI protocol for role change; JOIN/Mesh/CAD/LBT; redefining packet semantics (only reference NodeTable master table and existing contracts). Sub-roles (e.g. hunter variants) and repeater/car as first-class roles are **future (S04+)** unless already in canon; this doc references [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) as normative for the role set.

---

## 2) Definitions

### 2.1 Role/type at product level

- **Role/type** denotes the **operational mode** of the node for cadence and liveness: **when** to send (Alive vs Core_Pos) and the **maxSilence** guarantee. It is **distinct from** radio profile (channel, rate, txPower); see [radio_profiles_policy_v0](../../../../areas/radio/policy/radio_profiles_policy_v0.md) and [provisioning_baseline_s03](../firmware/policy/provisioning_baseline_s03.md).
- **Built-in roles (S03, normative):** **Person**, **Dog**, **Infra** per [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) §3.1. The device default role (used on first boot and after factory reset) is one of these — product-defined (e.g. Person). **Repeater** and **car** (or “machine”) are **not** in the v0 normative set; they may be added as built-ins or sub-roles in a future revision (S04+); do not invent values here.
- **Sub-roles:** Optional labels (e.g. hunter vs general Person) are **future-only**; S03 does not define or store them. Only the normative role id and the three behavior parameters are used by firmware in S03.

### 2.2 What firmware uses in S03: the three parameters

Firmware uses exactly **three normative parameters** from the active role profile. These gate sending Core_Pos vs Alive and define the liveness bound:

| Parameter | Meaning | Units | Use in S03 |
|-----------|---------|--------|------------|
| **min_interval_s** (minIntervalSec) | Minimum interval between beacon sends (rate limit). | seconds | Gates **when** the next TX is allowed; used for first-fix bootstrap and normal cadence ([field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md) §2.1). |
| **min_displacement_m** (minDisplacementM) | Minimum displacement (meters) before sending next position-bearing Core. | meters | Movement gating **after** first Core sent; not applied to the first Core ([role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) §3; first-fix bootstrap). |
| **max_silence_10s** (maxSilence10s) | Upper bound of acceptable silence (seconds) before the node **MUST** send at least one alive-bearing packet (Core_Pos or I_Am_Alive). | 10s steps (uint8) | Encoded on-air in Node_Informative; receivers use it as “silence doesn’t imply offline until &gt; maxSilence10s × 10 s”. Invariant: maxSilence ≥ 3×minInterval (same units). |

**Explicit:** “Speed” or other derived models are **not** stored as role fields in S03; only these three effective values are used. Design artifacts (e.g. avgSpeedKmh, multiplier) may be used to derive them but are informative only ([role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) §6).

---

## 3) Role parameter set (S03)

- **List:** min_interval_s, min_displacement_m, max_silence_10s (see §2.2).
- **Gating behavior:**  
  - **Core_Pos:** Sent when (1) min_interval_s has elapsed since last eligible send, and (2) min_displacement_m has been exceeded since last Core (or first Core after fix per first-fix bootstrap).  
  - **Alive:** Sent when no fix or when within maxSilence to satisfy liveness; cadence per [field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md) §2.1.
- **Invariant (normative):** maxSilence ≥ 3 × minInterval (in consistent time units). maxSilence10s encoded as uint8 in 10s steps; clamp ≤ 255 (2550 s). See [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) §3.

---

## 4) Defaults and profiles

- **FACTORY default role:** One of Person / Dog / Infra; **product-defined** which one is the single default (e.g. Person). Default role is **immutable**, embedded in firmware; not user-editable. See [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) §3.1, §4.
- **Built-in roles (OOTB):** Person, Dog, Infra with normative values per [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) §3.1:

| Role   | minIntervalSec | minDisplacementM | maxSilence10s |
|--------|----------------|------------------|---------------|
| Person | 18             | 25               | 9             |
| Dog    | 9              | 15               | 3             |
| Infra  | 360            | 100              | 255           |

(Infra: 255 = 2550 s; intended 3600 s may be clamped to encoding max.)
- **CurrentRoleId / PreviousRoleId:** CurrentRoleId points to the **active** role; used on boot to resolve the three params. PreviousRoleId is for **rollback** (e.g. “revert to previous”); cleared on factory reset. First boot: if no valid persisted CurrentRoleId → apply **default role**, persist CurrentRoleId (and optionally clear PreviousRoleId). See [boot_pipeline_v0](../../../../areas/firmware/policy/boot_pipeline_v0.md) §4, [provisioning_baseline_s03](../firmware/policy/provisioning_baseline_s03.md) §4.1, §6.

---

## 5) Storage and boot application

- **Where pointers live:** NVS namespace **`naviga`**; keys **role_cur**, **role_prev** (CurrentRoleId, PreviousRoleId). See [provisioning_baseline_s03](../firmware/policy/provisioning_baseline_s03.md) §5.1, [provisioning_interface_v0](../../../../areas/firmware/policy/provisioning_interface_v0.md).
- **Where the effective role record lives:** Cadence params may be stored in NVS (e.g. **role_interval_s**, **role_silence_10**, **role_dist_m**) or **resolved from default** / built-in role by id. Phase B **reads** role_cur/role_prev; if missing or invalid → apply default role and **persist** pointers (and optionally persist the default role record). See [provisioning_baseline_s03](../firmware/policy/provisioning_baseline_s03.md) §4.1, §5.1.
- **Phase B apply point:** Phase B selects active role (default if none persisted), persists pointers and ensures role record is available for Phase C. Phase C (Start comms) uses **only** the provisioned role for cadence. See [boot_pipeline_v0](../../../../areas/firmware/policy/boot_pipeline_v0.md) §4–§5, [ootb_autonomous_start_s03](../firmware/policy/ootb_autonomous_start_s03.md) §3.

---

## 6) NodeTable / on-air consumption

- **NodeTable fields reflecting role/type and maxSilence:**  
  - **max_silence_10s:** In NodeEntry; source = role profile; exported in **Node_Informative** (msg_type 0x05). Liveness hint; receivers interpret “silence doesn’t imply offline until last_seen_age &gt; maxSilence10s × 10 s” (subject to grace_s / hysteresis per [presence_and_age_semantics_v0_1](../../nodetable/policy/presence_and_age_semantics_v0_1.md)).  
  - **role_id:** Planned in master table (Node_Informative, WIP); role identifier on-air.  
  - **is_stale (derived):** expected_interval_s = max_silence_10s × 10; used with grace_s for staleness.  
  Source of truth: [NodeTable master table README](../../nodetable/master_table/README.md), [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../nodetable/master_table/packets_v0_1.csv). See [info_packet_encoding_v0](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) §3.2 (maxSilence10s at payload offset 9).
- **Where role/type is carried on-air:** maxSilence10s in **Node_Informative** (0x05, BEACON_INFO); role_id in Node_Informative (packet_sets_v0_1, WIP). Packet and field definitions: [packets_v0_1.csv](../../nodetable/master_table/packets_v0_1.csv), [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv); encoding: [info_packet_encoding_v0](../../../../areas/nodetable/contract/info_packet_encoding_v0.md).
- **Receiver interpretation of maxSilence:** “Silence doesn’t imply offline until last_seen_age_s &gt; (maxSilence10s × 10) + grace_s” (grace_s TBD; see presence_and_age_semantics). Node MUST send at least one alive-bearing packet within maxSilence window.

---

## 7) Open questions / future

- **Sub-roles and user-defined role profiles:** S04+; not in S03. When added, same persistence model (CurrentRoleId / PreviousRoleId, role record) can be extended.
- **Repeater / car (machine) as built-in roles:** Not in v0 normative set; product may add later with defined params; do not invent values in this doc.
- **role_id wire encoding:** Master table marks role_id as WIP (packet_sets_v0_1); exact encoding and id registry TBD; CONFLICT note (hw/fw u16 vs u8) in fields_v0_1.csv.

---

## 8) Related links

| Doc / issue | Description |
|-------------|-------------|
| [#357](https://github.com/AlexanderTsarkov/naviga-app/issues/357) | This task (Node role/type review). |
| [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) | S03 umbrella planning. |
| [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) ([#219](https://github.com/AlexanderTsarkov/naviga-app/issues/219)) | Normative role model: Person/Dog/Infra, three params, invariants, persistence, first boot, factory reset. |
| [field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md) | Core/Alive cadence; first-fix bootstrap; DOG_COLLAR vs HUMAN. |
| [provisioning_baseline_s03](../firmware/policy/provisioning_baseline_s03.md) | Phase A/B/C; role pointers and role record in NVS; fallbacks. |
| [boot_pipeline_v0](../../../../areas/firmware/policy/boot_pipeline_v0.md) | Phase B role provisioning; Phase C uses provisioned role. |
| [provisioning_interface_v0](../../../../areas/firmware/policy/provisioning_interface_v0.md) | Serial role get/set/reset; persistence rules. |
| [ootb_autonomous_start_s03](../firmware/policy/ootb_autonomous_start_s03.md) ([#354](https://github.com/AlexanderTsarkov/naviga-app/issues/354)) | OOTB trigger, sequence, fallback; default role at first boot. |
| [info_packet_encoding_v0](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) | Node_Informative (0x05); maxSilence10s encoding. |
| [NodeTable master table](../../nodetable/master_table/README.md) | [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../nodetable/master_table/packets_v0_1.csv) — consumer truth. |
| [presence_and_age_semantics_v0_1](../../nodetable/policy/presence_and_age_semantics_v0_1.md) | last_seen_age_s, is_stale, expected_interval_s = max_silence_10s×10; grace_s TBD. |
