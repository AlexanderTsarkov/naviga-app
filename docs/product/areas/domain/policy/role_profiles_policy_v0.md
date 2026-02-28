# Domain — Role profiles policy v0 (defaults, persistence, factory reset)

**Status:** Canon (policy).  
**Work Area:** Product Specs WIP · **Parent:** [#219](https://github.com/AlexanderTsarkov/naviga-app/issues/219) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines the **v0 role profile** model: **operational cadence parameters** derived from role (minIntervalSec, minDisplacementM, maxSilence10s), persistence semantics (default role, user roles, CurrentRoleId/PreviousRoleId pointers), and first-boot/factory-reset rules. **This is separate from Radio Profiles** ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)) — radio profiles define channel/modulation/txPower; role profiles define **when** to send (cadence) and **maxSilence** (liveness bound). Related: [#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214) (boot pipeline), [PR #205](https://github.com/AlexanderTsarkov/naviga-app/pull/205) (Alive/RX), [PR #213](https://github.com/AlexanderTsarkov/naviga-app/pull/213) (first-fix bootstrap).

---

## 1) Purpose & scope

- **Purpose:** Formally define **normative** operational parameters per role (minIntervalSec, minDisplacementM, maxSilence10s), **invariants** (maxSilence ≥ 3×minInterval; maxSilence10s encoding), **persistence** (DefaultRole immutable; user roles; CurrentRoleId/PreviousRoleId; first boot + factory reset). Provide an **informative** appendix that maps design artifacts (e.g. speed/distance/multiplier) to normative params without making those artifacts normative.
- **Scope (v0):** Role profile model; default role (immutable); user roles (add/remove); persisted pointers; first boot and factory reset semantics. No BLE/UI implementation; no new radio encoding (maxSilence10s remains uint8, 10s step per [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md)).

---

## 2) Non-goals

- No mesh/JOIN; no channel discovery ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)).
- No conflation with **radio profiles** ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)) — radio = channel/modulation/txPower; role = cadence/maxSilence.
- No new magic numbers beyond the 10s encoding unit and the **multiplier ≥ 3** rationale for the maxSilence ≥ 3×minInterval invariant.

---

## 3) Normative: operational parameters

Each **role profile** (default or user-defined) carries the following **normative** parameters used by the scheduler and by activity/liveness semantics:

| Parameter | Meaning | Encoding / constraint |
|-----------|---------|----------------------|
| **minIntervalSec** | Minimum interval between beacon sends (rate limit). | Positive; product-defined units (seconds). Used for first-fix bootstrap and normal cadence ([field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §2.1). |
| **minDisplacementM** | Minimum displacement (meters) before sending next position-bearing Core (movement gating). Applied **after** first Core sent; not applied to first Core ([PR #213](https://github.com/AlexanderTsarkov/naviga-app/pull/213)). | Non-negative; product-defined. |
| **maxSilence10s** | Upper bound of acceptable silence (seconds) before the node **MUST** send at least one alive-bearing packet (`Node_OOTB_Core_Pos` or `Node_OOTB_I_Am_Alive`). Sent in `Node_OOTB_Informative` (`0x05`) as uint8 in **10s steps**; clamp ≤ 90 (15 min). | uint8; step 10s; 0 = not specified. See [info_packet_encoding_v0](../../nodetable/contract/info_packet_encoding_v0.md) §3.2. |

**Invariants (normative):**

- **maxSilence ≥ 3 × minInterval** (in consistent time units). Rationale: maxSilence must allow at least a small multiple of the minimum interval so that the "alive within maxSilence" guarantee is meaningful and activity derivation does not contradict the rate limit.
- **maxSilence10s** is encoded as uint8 in **10s steps**; no change to existing encoding.

### 3.1 OOTB defaults (normative)

The following **OOTB default** role profiles are normative. Product/firmware MUST provide at least these three; the device default role (used on first boot and after factory reset) is one of them (product-defined which one is the single default, e.g. Person).

| Role   | minIntervalSec | minDisplacementM | maxSilence10s |
|--------|----------------|------------------|---------------|
| **Person** | 18  | 25  | 9  |
| **Dog**    | 9   | 15  | 3  |
| **Infra**  | 360 | 100 | 255 |

**Note (Infra):** The intended maxSilence for Infra may be 3600 s (1 h), but the encoding maximum is 2550 s (255×10 s). For OOTB, the value is **clamped to 255**; receivers interpret maxSilence10s = 255 as 2550 s.

---

## 4) Normative: role kinds and persistence

| Term | Meaning |
|------|--------|
| **Default role** | Single, **immutable** role that **MUST** always exist. **Embedded in firmware** (or product); not user-editable. On first boot (or when no valid current exists), the device **stores the default as an immutable record** in the persisted store so CurrentRoleId can point to it. Used when no user choice exists and after factory reset. |
| **User roles** | Zero or more role profiles created or modified by the user (add/remove). Mutable. |
| **CurrentRoleId** | Persisted **pointer** (id or handle) to the role profile used for operational cadence — **not** the profile content. **Used on boot** to restore last chosen role ([boot_pipeline_v0](../../firmware/policy/boot_pipeline_v0.md) Phase B). |
| **PreviousRoleId** | Optional persisted **pointer** to the previously active role; used for **rollback UX**. May be empty. |

**First boot:** If no valid persisted CurrentRoleId exists, the device MUST apply the **default role** (operational params from default), persist CurrentRoleId to point to it, and optionally clear PreviousRoleId.

**Factory reset:** Remove or deactivate all **user** roles; set CurrentRoleId to the default role and persist; clear PreviousRoleId. Default role remains.

---

## 5) Normative: minimal API (conceptual)

| Operation | Meaning |
|-----------|--------|
| **list** | Return available role profiles (at least default; plus any user roles). |
| **select** | Set CurrentRoleId to the given role; optionally set PreviousRoleId to the previous current. Persist. |
| **reset** | Factory reset: remove/deactivate user roles; current = default; clear previous. |

---

## 6) Informative appendix: design artifacts and mapping

The following are **design artifacts** used to derive or explain normative parameters. They are **not normative**; implementations may use them for authoring default/user role values or for UX. Policy only requires that the **normative** parameters (minIntervalSec, minDisplacementM, maxSilence10s) and invariants are satisfied.

| Artifact | Typical meaning | Mapping to normative |
|----------|-----------------|-----------------------|
| **avgSpeedKmh** (or speedHint) | Assumed typical speed for the subject/role (km/h or m/s). | Used to derive **minIntervalSec** (e.g. from desired distance granularity: minInterval ≥ minDisplacementM / speed). Not stored as normative field. |
| **desiredTrackErrorM** (or minDist) | Desired minimum distance between position updates (meters). | Maps to **minDisplacementM** (movement gate) and can inform **minIntervalSec** (rate). |
| **multiplier** | Factor such that maxSilence = multiplier × baseMinTime (e.g. baseMinTime from minInterval or derived). | Ensures **maxSilence ≥ 3 × minInterval** when multiplier ≥ 3. Product may use multiplier to compute maxSilence10s from a base interval; policy only requires the invariant. |

**Example mapping (informative):** baseMinTimeSeconds ≈ minIntervalSec; maxSilenceSeconds = round(baseMinTimeSeconds × multiplier); then maxSilence10s = round(maxSilenceSeconds / 10) clamped to uint8 and ≤ 90. This satisfies maxSilence ≥ 3×minInterval when multiplier ≥ 3.

---

## 7) Related

- **Radio profiles (distinct):** [radio_profiles_policy_v0](../../radio/policy/radio_profiles_policy_v0.md) ([#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)) — channel, modulation preset, txPower; not cadence.
- **Boot pipeline:** [boot_pipeline_v0](../../firmware/policy/boot_pipeline_v0.md) ([#214](https://github.com/AlexanderTsarkov/naviga-app/issues/214)) — Phase B provisions role (and radio profile).
- **Field cadence / first-fix:** [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) §2.1, §6 — Core/Alive, DOG_COLLAR vs HUMAN.
- **Informative packet (maxSilence10s wire encoding):** [info_packet_encoding_v0](../../nodetable/contract/info_packet_encoding_v0.md) — `Node_OOTB_Informative` (`0x05`); `maxSilence10s` at offset 9.
