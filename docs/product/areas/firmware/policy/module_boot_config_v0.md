# Firmware — Module boot configuration v0 (policy)

**Status:** Canon (policy).  
**Work Area:** Product Specs WIP · **Parent:** [#215](https://github.com/AlexanderTsarkov/naviga-app/issues/215) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines **device-level** module boot configuration: parameters to set/verify on startup and the **boot strategy** (verify-and-repair vs one-time init) per module. **This is not user radio profile selection** — that is [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)). User profile (channel, modulation preset, txPower) is applied **after** module config; this doc covers the modem/GNSS/NFC/power **device state** required for predictable operation. Ordering of boot phases (when this runs) is in [boot_pipeline_v0](boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) Phase A.

---

## 1) Purpose and scope

- **Purpose:** Specify which parameters FW must set or verify on boot for E220/E22 modem, GNSS, and (as placeholders) NFC/power, and whether each uses verify-and-repair on every boot vs one-time init.
- **Scope (v0):** E220/E22 device-level config; GNSS (u-blox M8N or abstract); NFC/power as explicit TBD list only.
- **Non-goals:** No user radio profile selection ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)); no channel discovery ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)); no CAD/LBT implementation (sense unsupported; jitter-only per policy).

---

## 2) E220 / E22 modem boot config (device-level)

**Critical parameters (verify-and-repair every boot):** The following **MUST** be verified on every boot and repaired on mismatch: **Enable RSSI Byte**, **LBT Enable**, **UART baud/parity**, **sub-packet**, **RSSI ambient/enable**. All other parameters may use one-time init or init-on-change (see table).

| Parameter | Required value / policy | When applied | Rationale | Notes |
|-----------|-------------------------|--------------|-----------|--------|
| **Enable RSSI Byte** | ON | **Verify-and-repair every boot** | Link metrics / receiver-side RSSI; required for NodeTable and activity. | Critical. |
| **LBT Enable** | OFF | **Verify-and-repair every boot** | Policy: sense unsupported; mitigation is jitter-only, no CAD/LBT. | Critical. |
| **UART baud / parity** | Product-defined; must match host | **Verify-and-repair every boot** | Consistent host–modem link; wrong baud breaks comms. | Critical. |
| **Sub-packet setting** | Product-defined; consistent for TX/RX | **Verify-and-repair every boot** | Payload and framing must match encoding contract. | Critical. Align with [beacon_payload_encoding](../../nodetable/contract/beacon_payload_encoding_v0.md). |
| **RSSI ambient noise / enable** | Align with "RSSI Byte ON"; enable RSSI reporting | **Verify-and-repair every boot** | Ensures RSSI byte is valid for link metrics. | Critical. |
| **WOR cycle** | Set only if WOR used | One-time init or init-on-change | Low-power listen cycle; not required for basic beacon cadence. | Only if used; otherwise leave default or disable. |
| **Key bytes** (e.g. AES) | Set only if encryption used | One-time init (only if used) | Out of scope for v0 unless product requires. | Mark "only if used"; likely out of scope v0. |

**Boot strategy summary (E220):** Critical parameters (RSSI Byte, LBT, UART, sub-packet, RSSI enable) — **verify-and-repair on every boot**. Optional (WOR, key bytes) — one-time init or product-defined.

---

## 3) GNSS boot config (u-blox M8N or abstract GNSS)

| Parameter | Required value / policy | When applied | Rationale | Notes |
|-----------|-------------------------|--------------|-----------|--------|
| **Baud / protocol** | Product-defined (e.g. NMEA or UBX) | Minimal verify-on-boot; repair on mismatch | Host–GNSS link must be consistent for fix and time. | See strategy below. |
| **Protocol** | NMEA or UBX (choose one) | Full config on first boot or when needed | Determines message set and parsing. | UBX allows tighter control; NMEA is common. |
| **Message rate / required sentences** | High-level: fix + time at rate sufficient for beacon cadence | Full config on first boot or when needed | BeaconCore needs position when fix available; Alive when no fix. | E.g. GGA + RMC (NMEA) or equivalent UBX. |
| **Required outputs** | Position (lat/lon), fix quality, time (for freshness) | As above | Core payload and first-fix bootstrap depend on valid fix. | Align with [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §2.1. |

**Boot strategy summary (GNSS):** **Minimal verify-on-boot** — baud/protocol and responsiveness (that the link is usable); **repair on mismatch**. **Full config** (message set, rate, required outputs) on **first boot or when needed**; no concrete timing constants in this policy.

---

## 4) NFC / power placeholders (TBD)

The following are **explicitly TBD** to avoid silent gaps. No normative values in v0.

| Area | Item | Status |
|------|------|--------|
| **NFC** | Init sequence (order, timing) | TBD |
| **NFC** | Role in pairing / first-time connect | TBD (see [pairing_flow_v0](../../../wip/areas/identity/pairing_flow_v0.md) for flow; NFC detail here when defined) |
| **Power** | Sleep / wake policy (when to enter low power) | TBD |
| **Power** | Relation to WOR / beacon cadence | TBD |
| **Power** | Battery / shutdown thresholds (if any) | TBD |

When defined, add parameter table and boot strategy per module; until then this section is the placeholder list.

---

## 5) Related

- **Boot pipeline (ordering):** [boot_pipeline_v0](boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) — Phase A uses this doc for "what to verify/repair."
- **User radio profile (not this doc):** [radio_profiles_policy_v0](../../../wip/areas/radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)) — channel, modulation preset, txPower are **user/profile** choices applied after module boot config.
- **Beacon / encoding:** [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md), [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md).
