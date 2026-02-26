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

**Freshness marker (Tier A):** Tier A **MUST** include a **freshness marker** (e.g. sequence number or monotonic tick) so receivers can order updates and detect staleness. **seq16** is canonical; byte layout in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §4.1. Tail-1 **MUST** carry **core_seq16** when sent separately; receiver applies only if core_seq16 == lastCoreSeq (CoreRef-lite). See encoding §4.2.

**Core transmit precondition:** BeaconCore is position-bearing and **MUST** only be transmitted when the sender has a valid GNSS fix. When the sender has no valid fix, it **MUST** satisfy the maxSilence liveness requirement via an **Alive packet** (see [alive_packet_encoding_v0.md](../contract/alive_packet_encoding_v0.md)); Alive is alive-bearing, non-position-bearing.

### 2.1 Bootstrap: first valid fix (BeaconCore)

While the sender has **no** valid GNSS fix, BeaconCore is **not** sent; liveness is provided by **Alive (no-fix)** per maxSilence. When the sender **first** obtains a valid fix and has **no baseline** (no lastPublishedPosition, i.e. first BeaconCore not yet sent), it **MUST** send the first BeaconCore at the next opportunity, limited only by the applicable rate limit (e.g. minInterval); **min displacement** does **not** apply to this first Core. After the first BeaconCore has been sent, movement gating (min displacement) applies as usual to subsequent Core sends.

### 2.2 Tail-2 scheduling classes (v0)

Tail-2 (BeaconTail-2) is split into two scheduling classes:

- **Tail-2 Operational:** Send **on change** (when any operational Tail-2 field value changes) **and at forced Core** (at least every N Core beacons as fallback; N and bootstrap/backoff are implementation-defined within product constraints). Keeps operational Tail-2 state eventually consistent without a fixed slow interval.
- **Tail-2 Informative:** Bootstrap/backoff → fixed cadence (default **10 min**), **and on change**. Informative fields (e.g. maxSilence10s) are sent at the informative cadence or when the value changes; they **MUST NOT** be required on every operational Tail-2 send.

---

## 3) Field → Tier → Cadence → Rationale

Source: fields from [link-telemetry-minset](../contract/link-telemetry-minset-v0.md) and [beacon_payload_encoding](../contract/beacon_payload_encoding_v0.md); no new semantics invented.

| Field | Tier | Default cadence | Source | Rationale |
|-------|------|-----------------|--------|------------|
| **nodeId** | A | Every beacon tick | Encoding | WHO; identity is required. |
| **positionLat** / **positionLon** (and position-valid semantics) | A | Every beacon tick when position valid | Encoding, NodeTable | WHERE; core for map. |
| **freshness marker** (seq16) | A | Every beacon tick | [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) §4.1 | Ordering and staleness; required for Core. seq16 (uint16, 2 B, LE) is canonical. |
| **posFlags** | B | Every Tail-1 (when position valid or every Tail-1) | Encoding §4.2, [position_quality_v0](position_quality_v0.md) | Position quality attached to Core sample. |
| **sats** | B | Every Tail-1 (when position valid or every Tail-1) | Encoding §4.2, [position_quality_v0](position_quality_v0.md) | Position quality attached to Core sample. |
| **hwProfileId** | B | Every N Core beacons OR every 60–120 s | Encoding, minset | Operational; capability lookup. |
| **fwVersionId** | B/C | Every 60–120 s (B) or 10 min (C) | Encoding, minset | Operational/diagnostic. |
| **uptimeSec** | B | Every 60–120 s | Encoding, minset | Timing; operational. |
| **maxSilence10s** | C | Tail-2 Informative: on change + every 10 min (bootstrap allowed) | Encoding §4.3, Policy | Helps activity/staleness; uint8, 10s steps, clamp ≤ 90. Not sent on every operational Tail-2. |
| **batteryPercent** | C | Every 10 min OR event-driven | Encoding, minset | Diagnostic; slow. |
| **txPowerStep hint** | B/C | Stub; 60–120 s or 10 min if used | Policy stub | Optional; diagnostic. |

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

- **Freshness marker encoding:** **Decided.** seq16 (uint16, 2 bytes, little-endian) is canonical. Byte layout is in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §4.1 (BeaconCore) and §4.2 (Tail-1 core_seq16). Scope: single per-node counter across all packet types (Core, Tail-1/2, Alive) during uptime; see [rx_semantics_v0.md](rx_semantics_v0.md) §1.
- **Beacon encoding:** Core/Tail split and byte layouts are in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3–5 (Core 19 B, Tail-1 core_seq16 + optional posFlags/sats, Tail-2 maxSilence10s; Tail-2 scheduling per §2.2 above).

---

## 9) Related

- **Traffic model:** [../../../wip/areas/radio/policy/traffic_model_v0.md](../../../wip/areas/radio/policy/traffic_model_v0.md) — BeaconCore/BeaconTail, ProductMaxFrameBytes, OOTB vs Session/Group.
- **Beacon encoding (layout):** [../contract/beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) — byte layout only; Core/Tail split driven by this policy.
- **Minset:** [../contract/link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) — field set and semantics.
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
