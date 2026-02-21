# Firmware — Boot pipeline v0 (policy)

**Status:** Canon (policy).  
**Work Area:** Product Specs WIP · **Parent:** [#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines the **v0 boot pipeline**: ordered phases from power-on to first radio comms (Alive/Beacon), with clear invariants per phase. It does not define UI, mesh/JOIN, or channel discovery; radio profile semantics are in [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211) — this doc only references them.

---

## 1) Purpose

- **Purpose:** Specify the **order** of boot phases (HW bring-up → provision mandatory settings → start comms) and the **invariants** that hold at the end of each phase, so firmware has a single normative source for "from power-on to first comms."
- **Scope (v0):** Phases A (HW bring-up), B (provision role + radio profile), C (start Alive/Beacon cadence). Phase D (BLE bridge) is a pointer only.

---

## 2) Non-goals

- No mesh/JOIN; no channel discovery ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)); no backend.
- No UI description; no implementation detail (e.g. exact code paths).
- Radio profile **model** and first-boot/factory-reset rules are in [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)); this doc only requires that provisioning follows that policy.

---

## 3) Phase A: HW bring-up (module boot configs)

- FW brings hardware modules (radio, GNSS, and any others required for comms) to the **required Naviga operating mode**.
- **Strategy:** Where a module uses **one-time init**, it sets a "configured" state (e.g. persisted flag); **then on every boot** FW MUST **verify** that critical module config matches the expected state and **repair** (re-init or re-apply config) on mismatch. For **critical** module configs (parameters required for comms), verify-and-repair on boot is **required**; see [module_boot_config_v0](module_boot_config_v0.md) ([#215](https://github.com/AlexanderTsarkov/naviga-app/issues/215)) for which parameters are critical per module.
- **Invariant by end of Phase A:** All modules needed for comms are in the correct state for Naviga. Critical configs are verified (and repaired if needed) every boot; one-time init alone is not sufficient unless explicitly documented per parameter elsewhere.

---

## 4) Phase B: Provision mandatory runtime settings

- **Role provisioning:** If no persisted role exists → apply **default role** and persist. Role is required for comms (e.g. DOG_COLLAR vs HUMAN per [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §6).
- **RadioProfile provisioning:** Apply default/current/previous per [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)): first-boot rule (no persisted CurrentProfileId → apply default and persist), CurrentProfileId/PreviousProfileId semantics. Do not duplicate that policy here — only require that FW has **provisioned** a current profile (and optional previous) before Phase C.
- **Invariant:** **Role** and **radio profile** (current) are **both** required for comms. OOTB MUST guarantee defaults exist so that "first comms" does not depend on UI or backend.

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

- **Radio profiles:** [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)).
- **Field cadence / first-fix:** [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §2.1 ([PR #213](https://github.com/AlexanderTsarkov/naviga-app/pull/213)).
- **Beacon / Alive encoding:** [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md), [alive_packet_encoding_v0](../../nodetable/contract/alive_packet_encoding_v0.md) ([PR #205](https://github.com/AlexanderTsarkov/naviga-app/pull/205)).
- **Module boot config (E220, GNSS):** [module_boot_config_v0](module_boot_config_v0.md) ([#215](https://github.com/AlexanderTsarkov/naviga-app/issues/215)).
