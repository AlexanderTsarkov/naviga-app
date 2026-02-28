# NodeTable — Field criticality & cadence v0 (Policy)

**Status:** Canon (policy).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines the **v0 mapping** of NodeTable-related fields into delivery tiers (Core / Tail-1 / Tail-2), default cadences, degrade-under-load order, mesh-lite priority by tier, and role-based delivery modifiers. It is the "meaning layer" that drives the BeaconCore/BeaconTail split; byte layout remains in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md). Semantic truth is this policy; no OOTB or UI as normative source.

---

## 1) Overview

- **Tiers:** Tier A (Core), Tier B (Tail-1), Tier C (Tail-2) define **what** must be delivered with what urgency. Core = WHO + WHERE + freshness; if Core is not delivered, product value collapses. Tail tiers are operational or diagnostic and can lag.
- **Cadences:** Each tier has **numeric** default cadences (not "frequent"/"rare" only). Under load, a fixed **degrade order** applies: keep Tier A, then reduce Tier B rate, then drop Tier C first.
- **Why this exists:** Prevents scope creep, keeps Core payload small for [traffic model](../../../wip/areas/radio/policy/traffic_model_v0.md) and ProductMaxFrameBytes=96, and gives mesh-lite a clear priority ordering (Core > Tail).

---

## 2) Tier definitions (normative)

| Tier | Name | Meaning | If not delivered |
|------|------|---------|-------------------|
| **A** | Core | WHO (identity) + WHERE (position when available) + **freshness marker**. Required for presence, map updates, and activity derivation. | Product value collapses (no reliable "who is where, how fresh"). |
| **B** | Tail-1 | Operational data; delay-tolerant (seconds–minutes). Capability/version and timing hints. | Degraded diagnostics; core tracking still works. |
| **C** | Tail-2 | Slow/diagnostic (minutes) or event-driven. Health, power hints. | Only diagnostics/UX suffer. |

**Freshness marker (Tier A):** Tier A **MUST** include a **freshness marker** (e.g. sequence number or monotonic tick) so receivers can order updates and detect staleness. **seq16** is canonical; byte layout in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §4.1. Tail-1 **MUST** carry **`ref_core_seq16`** when sent separately — a back-reference to the `seq16` of the Core sample it qualifies (Core linkage key, not a per-frame freshness counter); receiver applies only if `ref_core_seq16 == lastCoreSeq` (CoreRef-lite). See encoding §4.2.

**Core transmit precondition:** BeaconCore is position-bearing and **MUST** only be transmitted when the sender has a valid GNSS fix. When the sender has no valid fix, it **MUST** satisfy the maxSilence liveness requirement via an **Alive packet** (see [alive_packet_encoding_v0.md](../contract/alive_packet_encoding_v0.md)); Alive is alive-bearing, non-position-bearing.

### 2.1 Bootstrap: first valid fix (BeaconCore)

While the sender has **no** valid GNSS fix, BeaconCore is **not** sent; liveness is provided by **Alive (no-fix)** per maxSilence. When the sender **first** obtains a valid fix and has **no baseline** (no lastPublishedPosition, i.e. first BeaconCore not yet sent), it **MUST** send the first BeaconCore at the next opportunity, limited only by the applicable rate limit (e.g. minInterval); **min displacement** does **not** apply to this first Core. After the first BeaconCore has been sent, movement gating (min displacement) applies as usual to subsequent Core sends.

### 2.2 Operational vs Informative packets (v0)

Slow-state data is split across **two separate packets** with independent scheduling:

- **`Node_OOTB_Operational` (`0x04`, legacy `BEACON_TAIL2`):** Carries dynamic runtime fields (`batteryPercent`, `uptimeSec`). Send **on change** (when any Operational field value changes) **and at forced Core** (at least every N Core beacons as fallback; N implementation-defined). Full contract: [tail2_packet_encoding_v0.md](../contract/tail2_packet_encoding_v0.md).
- **`Node_OOTB_Informative` (`0x05`, legacy `BEACON_INFO`):** Carries static/config fields (`maxSilence10s`, `hwProfileId`, `fwVersionId`). Bootstrap/backoff → fixed cadence (default **10 min**), **and on change**. MUST NOT be sent on every Operational send. Full contract: [info_packet_encoding_v0.md](../contract/info_packet_encoding_v0.md).

These are independent TX paths; neither gates on the other.

---

## 3) Field → Tier → Cadence → Rationale

Source: fields from [link-telemetry-minset](../contract/link-telemetry-minset-v0.md) and [beacon_payload_encoding](../contract/beacon_payload_encoding_v0.md); no new semantics invented.

| Field | Tier | Packet | Default cadence | Source | Rationale |
|-------|------|--------|-----------------|--------|------------|
| **nodeId** | A | Common (all) | Every beacon tick | Encoding | WHO; identity is required. |
| **seq16** | A | Common (all) | Every beacon tick | [ootb_radio_v0 §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants) | Global per-node counter; ordering, dedupe. |
| **positionLat** / **positionLon** (`lat_u24` / `lon_u24`) | A | `Node_OOTB_Core_Pos` | Every beacon tick when position valid | [beacon_payload_encoding_v0 §4.1](../contract/beacon_payload_encoding_v0.md) | WHERE; core for map. `pos_valid`/`pos_age_s` are NOT in Core_Pos — see Core_Tail. |
| **posFlags** | B | `Node_OOTB_Core_Tail` | Every Core_Tail (when position valid) | Encoding, [position_quality_v0](position_quality_v0.md) | Position quality attached to Core sample. |
| **sats** | B | `Node_OOTB_Core_Tail` | Every Core_Tail (when position valid) | Encoding, [position_quality_v0](position_quality_v0.md) | Position quality attached to Core sample. |
| **batteryPercent** | C | `Node_OOTB_Operational` (`0x04`) | On change + at forced Core | Encoding, minset | Dynamic runtime: changes during operation. |
| **uptimeSec** | B | `Node_OOTB_Operational` (`0x04`) | On change + at forced Core | Encoding, minset | Dynamic runtime: changes during operation. |
| **maxSilence10s** | C | `Node_OOTB_Informative` (`0x05`) | On change + every 10 min (bootstrap allowed) | Encoding, Policy | Static/config: user-set liveness hint. uint8, 10s steps. |
| **hwProfileId** | C | `Node_OOTB_Informative` (`0x05`) | On change + every 10 min | Encoding, minset | Static: hardware identity, set at manufacture. |
| **fwVersionId** | C | `Node_OOTB_Informative` (`0x05`) | On change + every 10 min | Encoding, minset | Static: firmware version, changes only on OTA. |
| **txPower** | B/C | `Node_OOTB_Operational` (`0x04`) | On change + at forced Core (planned S03) | Policy stub | Dynamic adaptive; requires radio layer to expose tx power step. Not in canon yet. |

*Receiver-side only (lastRxAt, rssiLast, snrLast, etc.) are not sent in beacon; they are observed on RX. They are not tiered in this table.*

---

## 4) Degrade-under-load rules

**Order (fixed):**

1. **Keep Tier A.** Core cadence and delivery are preserved; Core payload MUST stay within strict size (see § Frame limits interaction).
2. **Drop Tier B rate.** Reduce Tail-1 cadence (e.g. from every 60 s to every 120 s, or from every N Core to every 2N Core).
3. **Drop Tier C first.** Omit or greatly reduce Tail-2 (e.g. battery, fwVersionId at slow cadence) before touching Tier B further.

**Under-load behavior:** When channel utilization or product policy triggers "degrade", apply the order above. No Tier A field may be moved to Tail to "save" Tier C.

---

## 5) Mesh-lite priority by tier (policy hooks)

- **Tier A (Core):** May use **higher** mesh priority and **higher** maxHops (within mesh-lite bounds). Core beacons are the last to be throttled or dropped when relaying.
- **Tier B / Tier C (Tail):** **Lower** mesh priority; **reduced** maxHops to avoid channel utilization blow-up. Tail may be dropped first when forwarding capacity is limited.

**Bounds:** Mesh-lite remains bounded (e.g. 2–3 hops max in v0). Exact maxHops per tier may be set in a future mesh-lite policy; this document fixes only the **ordering**: Core > Tail.

---

## 6) Role modifiers (DeliveryRole v0)

**DeliveryRole** influences which packet classes are allowed and which tier gets priority. Defined by policy (e.g. device type or configuration), not by UI copy.

| Role | Allowed packet classes | Mesh priority (Tier A) | Extras (UserMessage, Geo*, etc.) |
|------|------------------------|------------------------|----------------------------------|
| **DOG_COLLAR** | **BeaconCore / BeaconTail only.** No UserMessage, GeoObject, GeoObjectActivation. | Higher; Tier A may get higher maxHops for Core only. | Never (hard to reconfigure; reliability over features). |
| **HUMAN** | BeaconCore / BeaconTail; extras allowed only in Session/Group mode per [traffic_model_v0](../../../wip/areas/radio/policy/traffic_model_v0.md). | Normal. | Only when in Session/Group. |

**DOG_COLLAR rationale:** Collar is highest reliability priority and is not a comms device; no extras on public or otherwise. HUMAN: normal rules; extras only when user has selected Session/Group and there is a presumed recipient.

---

## 7) Frame limits interaction

- **ProductMaxFrameBytes v0 = 96** (from [traffic_model_v0](../../../wip/areas/radio/policy/traffic_model_v0.md)). **Fragmentation is not allowed.**
- **Tiering** MUST be used so that **Tier A (Core) fits within strict size budgets**. Core payload MUST stay short enough to meet profile-class limits in the encoding contract; Tail content is carried in BeaconTail and may be omitted under load per degrade order.

---

## 8) Encoding decisions (closed)

- **Freshness marker encoding:** **Decided.** `seq16` (uint16, 2 bytes, little-endian) is canonical. It is part of the **Common prefix** of every `Node_*` packet (see [ootb_radio_v0.md §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants)). Sender counter scope: **one global per-node counter** across ALL packet types during uptime. Dedupe key: `(nodeId48, seq16)`. `Node_OOTB_Core_Tail` additionally carries `ref_core_seq16` in its Useful payload — a back-reference to the `seq16` of the Core_Pos sample it qualifies; `seq16 ≠ ref_core_seq16`. See [rx_semantics_v0.md](rx_semantics_v0.md) §1.
- **Beacon encoding:** Core/Tail split and byte layouts are in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3–5. Common prefix (payloadVersion + nodeId48 + seq16 = 9 B) is shared by all packets. `Node_OOTB_Core_Pos`: 15 B payload / 17 B on-air. `Node_OOTB_Core_Tail`: `ref_core_seq16` + optional posFlags/sats. `Node_OOTB_Operational` (`0x04`): `batteryPercent`, `uptimeSec`. `Node_OOTB_Informative` (`0x05`): `maxSilence10s`, `hwProfileId`, `fwVersionId`. Two-packet split per §2.2 above.

---

## 9) Related

- **Traffic model:** [../../../wip/areas/radio/policy/traffic_model_v0.md](../../../wip/areas/radio/policy/traffic_model_v0.md) — BeaconCore/BeaconTail, ProductMaxFrameBytes, OOTB vs Session/Group.
- **Beacon encoding (layout):** [../contract/beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) — byte layout only; Core/Tail split driven by this policy.
- **Minset:** [../contract/link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) — field set and semantics.
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
