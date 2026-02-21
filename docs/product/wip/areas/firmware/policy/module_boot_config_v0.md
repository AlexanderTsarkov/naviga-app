# Firmware — Module boot configuration v0 (policy)

**Work Area:** Product Specs WIP · **Parent:** [#215](https://github.com/AlexanderTsarkov/naviga-app/issues/215) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines **device-level** module boot configuration: parameters to set/verify on startup and the **boot strategy** (verify-and-repair vs one-time init) per module. **This is not user radio profile selection** — that is [radio_profiles_policy_v0](../../radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)). User profile (channel, modulation preset, txPower) is applied **after** module config; this doc covers the modem/GNSS/NFC/power **device state** required for predictable operation. Ordering of boot phases (when this runs) is in [boot_pipeline_v0](boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) Phase A.

---

## 1) Purpose and scope

- **Purpose:** Specify which parameters FW must set or verify on boot for E220/E22 modem, GNSS, and (as placeholders) NFC/power, and whether each uses verify-and-repair on every boot vs one-time init.
- **Scope (v0):** E220/E22 device-level config; GNSS (u-blox M8N or abstract); NFC/power as explicit TBD list only.
- **Non-goals:** No user radio profile selection ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)); no channel discovery ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)); no CAD/LBT implementation (sense unsupported; jitter-only per policy).

---

## 2) E220 / E22 modem boot config (device-level)

**Strategy note:** Prefer **verify-and-repair on boot** for parameters critical to comms (RSSI, LBT, UART, sub-packet). One-time init + “configured” flag may be used for parameters that rarely change and are persisted; document per row below.

| Parameter | Required value / policy | When applied | Rationale | Notes |
|-----------|-------------------------|--------------|-----------|--------|
| **Enable RSSI Byte** | ON | Verify-on-boot (recommended) or one-time init | Link metrics / receiver-side RSSI; required for NodeTable and activity. | If one-time: persist “configured” and re-verify after factory reset. |
| **LBT Enable** | OFF | Verify-on-boot (recommended) or one-time init | Policy: sense unsupported; mitigation is jitter-only, no CAD/LBT. | Must match product policy. |
| **UART baud / parity** | Product-defined; must match host | Verify-on-boot recommended | Consistent host–modem link; wrong baud breaks comms. | Same as “when applied” for other critical UART params. |
| **Sub-packet setting** | Product-defined; consistent for TX/RX | Verify-on-boot recommended | Payload and framing must match encoding contract. | Align with [beacon_payload_encoding](../../nodetable/contract/beacon_payload_encoding_v0.md) and modem capability. |
| **RSSI ambient noise / enable** | Align with “RSSI Byte ON”; enable RSSI reporting | Verify-on-boot recommended | Ensures RSSI byte is valid for link metrics. | May be single register; keep consistent with Enable RSSI Byte. |
| **WOR cycle** | Set only if WOR used | One-time init or verify-on-boot (product choice) | Low-power listen cycle; not required for basic beacon cadence. | Only if used; otherwise leave default or disable. |
| **Key bytes** (e.g. AES) | Set only if encryption used | One-time init (only if used) | Out of scope for v0 unless product requires. | Mark “only if used”; likely out of scope v0. |

**Boot strategy summary (E220):** Critical comms parameters (RSSI Byte ON, LBT OFF, UART, sub-packet, RSSI enable) — **prefer verify-and-repair on every boot**. Optional or rarely changed (WOR, key bytes) — one-time init or product-defined.

---

## 3) GNSS boot config (u-blox M8N or abstract GNSS)

| Parameter | Required value / policy | When applied | Rationale | Notes |
|-----------|-------------------------|--------------|-----------|--------|
| **Baud / protocol** | Product-defined (e.g. 9600 NMEA or UBX) | Verify-on-boot recommended | Host–GNSS link must be consistent for fix and time. | NMEA or UBX; document which in product. |
| **Protocol** | NMEA or UBX (choose one) | One-time init or verify-on-boot | Determines message set and parsing. | UBX allows tighter control; NMEA is common. |
| **Message rate / required sentences** | High-level: fix + time at rate sufficient for beacon cadence | Verify-on-boot or one-time init | BeaconCore needs position when fix available; Alive when no fix. | E.g. GGA + RMC (NMEA) or equivalent UBX; rate ≥ 1 Hz typical for tracking. |
| **Required outputs** | Position (lat/lon), fix quality, time (for freshness) | As above | Core payload and first-fix bootstrap depend on valid fix. | No new semantics; align with [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §2.1. |

**Boot strategy summary (GNSS):** Baud/protocol and message set — **verify-and-repair on boot** recommended so that after Phase A the device can obtain fix and time reliably; one-time init acceptable if product documents persistence and re-apply on factory reset.

---

## 4) NFC / power placeholders (TBD)

The following are **explicitly TBD** to avoid silent gaps. No normative values in v0.

| Area | Item | Status |
|------|------|--------|
| **NFC** | Init sequence (order, timing) | TBD |
| **NFC** | Role in pairing / first-time connect | TBD (see [pairing_flow_v0](../../identity/pairing_flow_v0.md) for flow; NFC detail here when defined) |
| **Power** | Sleep / wake policy (when to enter low power) | TBD |
| **Power** | Relation to WOR / beacon cadence | TBD |
| **Power** | Battery / shutdown thresholds (if any) | TBD |

When defined, add parameter table and boot strategy per module; until then this section is the placeholder list.

---

## 5) Related

- **Boot pipeline (ordering):** [boot_pipeline_v0](boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) — Phase A uses this doc for “what to verify/repair.”
- **User radio profile (not this doc):** [radio_profiles_policy_v0](../../radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)) — channel, modulation preset, txPower are **user/profile** choices applied after module boot config.
- **Beacon / encoding:** [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md), [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md).
