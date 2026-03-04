# Firmware — Boot pipeline v0 (policy)

**Status:** Canon (policy).  
**Work Area:** Product Specs WIP · **Parent:** [#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines the **v0 boot pipeline**: ordered phases from power-on to first radio comms (Alive/Beacon), with clear invariants per phase. It does not define UI, mesh/JOIN, or channel discovery; radio profile semantics are in [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211) — this doc only references them. **S03 provisioning baseline:** [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353); hardware provisioning + boot placement: [#381](https://github.com/AlexanderTsarkov/naviga-app/issues/381). **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351).

---

## 1) Purpose

- **Purpose:** Specify the **order** of boot phases (HW bring-up → provision mandatory settings → start comms) and the **invariants** that hold at the end of each phase, so firmware has a single normative source for "from power-on to first comms."
- **Scope (v0):** Phases A (HW bring-up), B (provision role + radio profile), C (start Alive/Beacon cadence). Phase D (BLE bridge) is a pointer only.

---

## 2) Non-goals

- No mesh/JOIN; no channel discovery ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)); no backend.
- No UI description; no implementation detail (e.g. exact code paths).
- Radio profile **model** and first-boot/factory-reset rules are in [radio_profiles_policy_v0](../../radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)); this doc only requires that provisioning follows that policy.

---

## 3) Phase A: Hardware provisioning (module boot configs)

- **Phase A = Hardware provisioning:** FW brings **GNSS** and **radio module** (and any other modules required for comms) to the **required Naviga operating mode**, including **verify-and-heal** per [module_boot_config_v0](module_boot_config_v0.md) ([#215](https://github.com/AlexanderTsarkov/naviga-app/issues/215)).
- **Strategy:** For each module, FW MUST **verify** critical config on every boot and **repair** (re-init or re-apply config) on mismatch. One-time init alone is not sufficient unless explicitly documented per parameter in [module_boot_config_v0](module_boot_config_v0.md). See that doc for the verify/heal contract and failure behavior (RepairFailed must not brick the device; §5).
- **OOTB invariant:** By end of Phase A, the **radio** MUST be configured for OOTB comms using the **FACTORY default RadioProfile**: channel, air-rate (modulation preset), and **tx power baseline** as defined by the product. This ensures the device can transmit and receive on first boot without any persisted profile or UI. User-selectable profiles and persistence are applied in Phase B; Phase A does not depend on NVS for radio operability.
- **Invariant by end of Phase A:** All modules needed for comms are in the correct state for Naviga (or an observable fault state is set and best-effort continues; see [module_boot_config_v0](module_boot_config_v0.md) §4). Critical configs are verified (and repaired if needed) every boot.

---

## 4) Phase B: Role and profile selection + persistence (for rollback and future UI)

- **Phase B = selection + persistence:** Phase B **selects** the active role and radio profile (default if none persisted) and **persists** default/current/previous pointers and records so that rollback and future UI/BLE can use them. **Persistence is not a prerequisite for OOTB working:** on first boot, Phase A has already applied the FACTORY default RadioProfile to the radio; Phase B ensures the in-memory and persisted state (pointers/records) are consistent for cadence, rollback, and later provisioning interface use.
- **Role provisioning:** If no persisted role exists → apply **default role** and persist. Role is required for comms (e.g. cadence params per [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §6).
- **RadioProfile provisioning:** Apply default/current/previous per [radio_profiles_policy_v0](../../radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)): first-boot rule (no persisted CurrentProfileId → apply default and persist), CurrentProfileId/PreviousProfileId semantics. The **radio hardware** is already configured in Phase A with the FACTORY default; Phase B persists the logical "current" pointer/record for consistency and future UI.
- **Invariant:** **Role** and **radio profile** (current) are **both** resolved for comms. OOTB MUST guarantee defaults exist so that "first comms" does not depend on UI or backend. Persistence supports rollback and future provisioning; it does not block first-boot OOTB.

---

## 5) Phase C: Start comms

- Start **Alive / Beacon cadence** using the **provisioned role** and **provisioned radio profile** (channel, modulation preset, txPower from current profile). Encoding and semantics: [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md), [alive_packet_encoding_v0](../../nodetable/contract/alive_packet_encoding_v0.md); RX semantics: [rx_semantics_v0](../../nodetable/policy/rx_semantics_v0.md) (ref [PR #205](https://github.com/AlexanderTsarkov/naviga-app/pull/205)).
- **First-fix bootstrap** ([field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §2.1, ref [PR #213](https://github.com/AlexanderTsarkov/naviga-app/pull/213)): While no GNSS fix, only Alive(no-fix) per maxSilence; when first valid fix and no baseline → send first BeaconCore at next opportunity without min-displacement gate; then normal cadence.
- **Invariant:** Comms use **only** the provisioned role and provisioned current profile; no ad hoc defaults at "start comms" time.

---

## 6) Phase D: BLE bridge (later)

- Expose BLE bridge for app pairing/control — **out of scope for v0 pipeline**. Phase D is a pointer only: "when BLE stack is up, bridge can be exposed"; no detail in this doc.

---

## 7) Invariants checklist (post-boot guarantees)

By the time first radio comms (Alive or Beacon) are sent:

| Invariant | Phase |
|-----------|--------|
| All modules needed for comms are in correct Naviga state (verify-and-repair where required). | A |
| Role is provisioned (default if missing, persisted). | B |
| CurrentProfileId is provisioned (default if missing per [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211), persisted). | B |
| Comms use only provisioned role + current profile; first-fix bootstrap respected. | C |

---

## 8) Related

- **Module boot config (verify/heal, failure behavior):** [module_boot_config_v0](module_boot_config_v0.md) ([#215](https://github.com/AlexanderTsarkov/naviga-app/issues/215)) — Phase A hardware provisioning follows this doc; RepairFailed behavior §4.
- **Radio profiles:** [radio_profiles_policy_v0](../../radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)); FACTORY default profile applied in Phase A for OOTB.
- **Provisioning interface:** [provisioning_interface_v0](provisioning_interface_v0.md) ([#221](https://github.com/AlexanderTsarkov/naviga-app/issues/221)) — writes pointers/records that Phase B reads.
- **Field cadence / first-fix:** [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §2.1 ([PR #213](https://github.com/AlexanderTsarkov/naviga-app/pull/213)).
- **Beacon / Alive encoding:** [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md), [alive_packet_encoding_v0](../../nodetable/contract/alive_packet_encoding_v0.md) ([PR #205](https://github.com/AlexanderTsarkov/naviga-app/pull/205)).
- **S03:** [#381](https://github.com/AlexanderTsarkov/naviga-app/issues/381) (hardware provisioning + boot placement), [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353) (epic), [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) (umbrella).
