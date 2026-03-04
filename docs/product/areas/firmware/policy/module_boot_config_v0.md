# Firmware — Module boot configuration v0 (policy)

**Status:** Canon (policy).  
**Work Area:** Product Specs WIP · **Parent:** [#215](https://github.com/AlexanderTsarkov/naviga-app/issues/215) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines **device-level** module boot configuration: parameters to set/verify on startup and the **boot strategy** (verify-and-repair vs one-time init) per module. Ordering of boot phases is in [boot_pipeline_v0](boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) — **Phase A = hardware provisioning** (this doc); Phase B = role/profile selection + persistence. **S03:** [#381](https://github.com/AlexanderTsarkov/naviga-app/issues/381), [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353), [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351). **Module-critical vs profile-applied:** Parameters that are **module-critical** (e.g. RSSI append, UART framing) are required for the module to operate correctly regardless of user choice. Parameters that are **profile-applied** (channel, rate tier, tx power baseline) are set from the **FACTORY default RadioProfile** in Phase A to achieve OOTB operability; user-selectable profiles (future UI/BLE) are applied via Phase B and [provisioning_interface_v0](provisioning_interface_v0.md). See §2 (E220) and §3 (GNSS) for the verify/heal contract; §5 for failure behavior.

---

## 1) Purpose and scope

- **Purpose:** Specify which parameters FW must set or verify on boot for E220/E22 modem, GNSS, and (as placeholders) NFC/power, and whether each uses verify-and-repair on every boot vs one-time init.
- **Scope (v0):** E220/E22 device-level config; GNSS (u-blox M8N or abstract); NFC/power as explicit TBD list only.
- **Non-goals:** No user radio profile selection ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)); no channel discovery ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)); no CAD/LBT implementation (sense unsupported; jitter-only per policy).

---

## 2) E220 / E22 modem boot config (device-level)

**Verify/heal contract:** On every boot, FW **reads** the module configuration (getConfiguration). If **critical** parameters do not match the expected values (from FACTORY default profile where applicable), FW **applies** the correct values (setConfiguration, WRITE_CFG_PWR_DWN_SAVE) and **re-reads** to verify. Outcome is **Ok** (no change needed), **Repaired** (mismatch corrected), or **RepairFailed** (apply or verify failed). See §5 for behavior when RepairFailed.

**Module-critical vs profile-applied:** **Module-critical** parameters are those required for the modem to function correctly regardless of user profile: UART baud/parity, RSSI enable, LBT, sub-packet setting, RSSI ambient. **Profile-applied** parameters are those that define OOTB operability and come from the **FACTORY default RadioProfile** (product-defined): **channel**, **air_data_rate** (rate tier), and **tx power baseline**. In S03 OOTB, only the FACTORY default profile is applied at boot; user profiles (future UI/BLE) are not applied in Phase A. Tx power baseline: where the module supports it via UART config, it MUST be set from the FACTORY default in Phase A; where the module does not expose tx power in config (e.g. E220 UART), document as "module default only" until product mapping exists.

**Critical parameters (verify-and-repair every boot):** The following **MUST** be verified on every boot and repaired on mismatch: **Enable RSSI Byte**, **LBT Enable**, **UART baud/parity**, **air_data_rate (preset)**, **sub-packet**, **RSSI ambient/enable**, **channel**. All other parameters may use one-time init or init-on-change (see table).

| Parameter | Required value / policy | When applied | Rationale | Notes |
|-----------|-------------------------|--------------|-----------|--------|
| **Enable RSSI Byte** | ON | **Verify-and-repair every boot** | Link metrics / receiver-side RSSI; required for NodeTable and activity. | Critical. |
| **LBT Enable** | OFF | **Verify-and-repair every boot** | Policy: sense unsupported; mitigation is jitter-only, no CAD/LBT. | Critical. |
| **UART baud / parity** | Product-defined; must match host | **Verify-and-repair every boot** | Consistent host–modem link; wrong baud breaks comms. | Critical. |
| **Air data rate (airRate)** | From active `RadioPreset` (default: 2 = 2.4 kbps); normalized if < 2 | **Verify-and-repair every boot** | Ensures all nodes on same air speed; E22 V2 clamps < 2 to 2 (see §2a). | Critical. Normalized via `normalize_air_rate()` before apply. |
| **Channel** | From active `RadioPreset` (default: 1) | **Verify-and-repair every boot** | Nodes must be on same channel to communicate. | Critical. |
| **Sub-packet setting** | Product-defined; consistent for TX/RX | **Verify-and-repair every boot** | Payload and framing must match encoding contract. | Critical. Align with [beacon_payload_encoding](../../nodetable/contract/beacon_payload_encoding_v0.md). |
| **RSSI ambient noise / enable** | Align with "RSSI Byte ON"; enable RSSI reporting | **Verify-and-repair every boot** | Ensures RSSI byte is valid for link metrics. | Critical. |
| **WOR cycle** | Set only if WOR used | One-time init or init-on-change | Low-power listen cycle; not required for basic beacon cadence. | Only if used; otherwise leave default or disable. |
| **Key bytes** (e.g. AES) | Set only if encryption used | One-time init (only if used) | Out of scope for v0 unless product requires. | Mark "only if used"; likely out of scope v0. |

**Boot strategy summary (E220/E22):** Critical parameters (RSSI Byte, LBT, UART, airRate, channel, sub-packet, RSSI enable) — **verify-and-repair on every boot**. Optional (WOR, key bytes) — one-time init or product-defined.

### 2a) Air data rate normalization and readback verify (E22/E220, PR #341)

The boot sequence for `E22Radio::begin(preset)` / `E220Radio::begin(preset)`:

1. **Normalize** requested `airRate`: if `airRate < kMinAirRate` (2), clamp to 2 and log `"air_rate clamped: req=X norm=2"`.
2. **Apply** full critical config via `setConfiguration()`.
3. **Readback verify**: call `getConfiguration()` and compare `airRate` + `channel` against normalized target.
4. **Log outcome**: `"E22 boot: config ok"` (no repair needed) / `"E22 boot: repaired (<detail>)"` / `"E22 boot: repair failed (<detail>)"`.

**E22-400T30D V2 constraint:** Module firmware clamps `airRate` 0 and 1 to 2 (2.4 kbps) in the ACK frame itself. Proven by controlled raw-UART harness (issue [#336](https://github.com/AlexanderTsarkov/naviga-app/issues/336)). `kMinAirRate = 2` is the enforced minimum for this module family.

---

## 3) GNSS boot config (u-blox M8N or abstract GNSS)

| Parameter | Required value / policy | When applied | Rationale | Notes |
|-----------|-------------------------|--------------|-----------|--------|
| **Baud / protocol** | Product-defined (e.g. NMEA or UBX) | Minimal verify-on-boot; repair on mismatch | Host–GNSS link must be consistent for fix and time. | See strategy below. |
| **Protocol** | NMEA or UBX (choose one) | Full config on first boot or when needed | Determines message set and parsing. | UBX allows tighter control; NMEA is common. |
| **Message rate / required sentences** | High-level: fix + time at rate sufficient for beacon cadence | Full config on first boot or when needed | BeaconCore needs position when fix available; Alive when no fix. | E.g. GGA + RMC (NMEA) or equivalent UBX. |
| **Required outputs** | Position (lat/lon), fix quality, time (for freshness) | As above | Core payload and first-fix bootstrap depend on valid fix. | Align with [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §2.1. |

**Boot strategy summary (GNSS):** **Minimal verify-on-boot** — baud/protocol and responsiveness (that the link is usable); **repair on mismatch**. **Full config** (message set, rate, required outputs) on **first boot or when needed**; no concrete timing constants in this policy.

**Verify/heal contract (GNSS):** FW opens the GNSS UART at product-defined baud, sends the required UBX config (e.g. enable NAV-PVT) each boot. **Verify** = at least one byte received within a timeout (link alive). If verify fails, FW may retry init/verify once. There is no readback of baud/protocol from the receiver; "repair" is re-send config and retry. Outcome: **Ok** (link responsive), **Repaired** (retry succeeded), or **RepairFailed** (link not responsive after retries). See §4 for RepairFailed behavior.

---

## 4) Failure behavior (Ok / Repaired / RepairFailed)

Embedded devices have no screen; the policy MUST define how failures are observable and that the device never bricks.

**Outcomes:**

| Outcome | Meaning |
|--------|---------|
| **Ok** | No repair needed; module config already matched expected state. |
| **Repaired** | Mismatch was detected and successfully corrected; readback verify passed. |
| **RepairFailed** | Apply failed, or readback verify failed after apply (e.g. module did not accept config). |

**RepairFailed requirement:** **MUST NOT brick the device.** FW MUST continue best-effort: e.g. allow Phase B and Phase C to run so that the device may still participate on the air with whatever state the module is in, or degrade gracefully (e.g. radio comms disabled but diag available). The choice of "continue with degraded radio" vs "do not start comms" is implementation-defined but MUST be documented; in all cases the device MUST set an **observable fault state** so that diagnostics and future signaling can inform the user.

**Observable fault state:** FW MUST set a **fault/diag flag or status** (e.g. boot_config_result = RepairFailed) that can be read by diagnostics, provisioning status, or BLE. This state MUST be available for **progressive signaling**:

- **Now:** Logs and a diag field (e.g. status command, or field exposed to host). No protocol design required; only the requirement that the outcome is observable.
- **Later:** Emergency or fault LED (product-defined), so that a user can see "something is wrong" without a phone.
- **Later:** User-visible notification when the phone connects (e.g. "Radio config repair failed at boot"). No BLE protocol or UI design in this doc; only the requirement that the fault state can be used for such notification when the stack exists.

This ensures that RepairFailed is never silent and that the product can evolve from log-only to LED to phone notification without a policy change.

---

## 5) NFC / power placeholders (TBD)

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

## 6) Related

- **Boot pipeline (ordering):** [boot_pipeline_v0](boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) — Phase A = hardware provisioning using this doc; Phase B = role/profile persistence. OOTB invariant: Phase A applies FACTORY default RadioProfile (incl. tx power baseline).
- **Radio profiles:** [radio_profiles_policy_v0](../../radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)) — default vs user profiles; **FACTORY default** is the product's default applied in Phase A for OOTB; user/profile selection and persistence are Phase B and [provisioning_interface_v0](provisioning_interface_v0.md).
- **Provisioning interface:** [provisioning_interface_v0](provisioning_interface_v0.md) ([#221](https://github.com/AlexanderTsarkov/naviga-app/issues/221)) — writes role/radio pointers and records; Phase B reads them.
- **Beacon / encoding:** [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md), [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md).
- **S03:** [#381](https://github.com/AlexanderTsarkov/naviga-app/issues/381) (hardware provisioning + boot placement), [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353) (epic), [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) (umbrella).
