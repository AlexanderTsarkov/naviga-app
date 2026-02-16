# NodeTable — Field criticality & cadence v0 (Policy)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines the **v0 mapping** of NodeTable-related fields into delivery tiers (Core / Tail-1 / Tail-2), default cadences, degrade-under-load order, mesh-lite priority by tier, and role-based delivery modifiers. It is the “meaning layer” that drives the BeaconCore/BeaconTail split; byte layout remains in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md). Semantic truth is this policy; no OOTB or UI as normative source.

---

## 1) Overview

- **Tiers:** Tier A (Core), Tier B (Tail-1), Tier C (Tail-2) define **what** must be delivered with what urgency. Core = WHO + WHERE + freshness; if Core is not delivered, product value collapses. Tail tiers are operational or diagnostic and can lag.
- **Cadences:** Each tier has **numeric** default cadences (not “frequent”/“rare” only). Under load, a fixed **degrade order** applies: keep Tier A, then reduce Tier B rate, then drop Tier C first.
- **Why this exists:** Prevents scope creep, keeps Core payload small for [traffic model](../../radio/policy/traffic_model_v0.md) and ProductMaxFrameBytes=96, and gives mesh-lite a clear priority ordering (Core > Tail).

---

## 2) Tier definitions (normative)

| Tier | Name | Meaning | If not delivered |
|------|------|---------|-------------------|
| **A** | Core | WHO (identity) + WHERE (position when available) + **freshness marker**. Required for presence, map updates, and activity derivation. | Product value collapses (no reliable “who is where, how fresh”). |
| **B** | Tail-1 | Operational data; delay-tolerant (seconds–minutes). Capability/version and timing hints. | Degraded diagnostics; core tracking still works. |
| **C** | Tail-2 | Slow/diagnostic (minutes) or event-driven. Health, power hints. | Only diagnostics/UX suffer. |

**Freshness marker (Tier A):** Tier A **MUST** include a **freshness marker** (e.g. sequence number or monotonic tick) so receivers can order updates and detect staleness. **seq16** is canonical; byte layout in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §4.1. Tail-1 **MUST** carry **core_seq16** when sent separately; receiver applies only if core_seq16 == lastCoreSeq (CoreRef-lite). See encoding §4.2.

### 2.1 Tail-2 Operational scheduling (v0)

**Tail-2 (BeaconTail-2)** MUST be sent **on change** (when any Tail-2 field value changes) **and at forced Core** (at least every N Core beacons as fallback; N and bootstrap/backoff are implementation-defined within product constraints). This keeps Tail-2 state eventually consistent without requiring a fixed slow interval.

---

## 3) Field → Tier → Cadence → Rationale

Source: fields from [link-telemetry-minset](../contract/link-telemetry-minset-v0.md) and [beacon_payload_encoding](../contract/beacon_payload_encoding_v0.md); no new semantics invented.

| Field | Tier | Default cadence | Source | Rationale |
|-------|------|-----------------|--------|------------|
| **nodeId** | A | Every beacon tick | Encoding | WHO; identity is required. |
| **positionLat** / **positionLon** (and position-valid semantics) | A | Every beacon tick when position valid | Encoding, NodeTable | WHERE; core for map. |
| **freshness marker** (seq or equivalent) | A | Every beacon tick | TBD (decision point) | Ordering and staleness; required for Core. |
| **hwProfileId** | B | Every N Core beacons OR every 60–120 s | Encoding, minset | Operational; capability lookup. |
| **fwVersionId** | B/C | Every 60–120 s (B) or 10 min (C) | Encoding, minset | Operational/diagnostic. |
| **uptimeSec** | B | Every 60–120 s | Encoding, minset | Timing; operational. |
| **maxSilence10s** | C | Every Tail-2 send (on change + at forced Core) | Encoding §4.3, Policy | Helps activity/staleness; uint8, 10s steps, clamp ≤ 90. |
| **batteryPercent** | C | Every 10 min OR event-driven | Encoding, minset | Diagnostic; slow. |
| **txPowerStep hint** | B/C | Stub; 60–120 s or 10 min if used | Policy stub | Optional; diagnostic. |

*Receiver-side only (lastRxAt, rssiLast, snrLast, etc.) are not sent in beacon; they are observed on RX. They are not tiered in this table.*

---

## 4) Degrade-under-load rules

**Order (fixed):**

1. **Keep Tier A.** Core cadence and delivery are preserved; Core payload MUST stay within strict size (see § Frame limits interaction).
2. **Drop Tier B rate.** Reduce Tail-1 cadence (e.g. from every 60 s to every 120 s, or from every N Core to every 2N Core).
3. **Drop Tier C first.** Omit or greatly reduce Tail-2 (e.g. battery, fwVersionId at slow cadence) before touching Tier B further.

**Under-load behavior:** When channel utilization or product policy triggers “degrade”, apply the order above. No Tier A field may be moved to Tail to “save” Tier C.

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
| **HUMAN** | BeaconCore / BeaconTail; extras allowed only in Session/Group mode per [traffic_model_v0](../../radio/policy/traffic_model_v0.md). | Normal. | Only when in Session/Group. |

**DOG_COLLAR rationale:** Collar is highest reliability priority and is not a comms device; no extras on public or otherwise. HUMAN: normal rules; extras only when user has selected Session/Group and there is a presumed recipient.

---

## 7) Frame limits interaction

- **ProductMaxFrameBytes v0 = 96** (from [traffic_model_v0](../../radio/policy/traffic_model_v0.md)). **Fragmentation is not allowed.**
- **Tiering** MUST be used so that **Tier A (Core) fits within strict size budgets**. Core payload MUST stay short enough to meet profile-class limits in the encoding contract; Tail content is carried in BeaconTail and may be omitted under load per degrade order.

---

## 8) Open decisions (explicit)

- **Freshness marker encoding:** Tier A MUST include a freshness marker; **exact encoding (e.g. seq8 vs seq16, field order)** is **TBD** and will be decided in a follow-up (encoding doc or separate decision). Discoverable here so implementers do not invent ad hoc.
- **Beacon encoding:** Core/Tail split and byte layouts are in [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3–5 (Core 19 B, Tail-1 core_seq16 + optional posFlags/sats, Tail-2 maxSilence10s; Tail-2 scheduling per §2.1 above).

---

## 9) Related

- **Traffic model:** [../../radio/policy/traffic_model_v0.md](../../radio/policy/traffic_model_v0.md) — BeaconCore/BeaconTail, ProductMaxFrameBytes, OOTB vs Session/Group.
- **Beacon encoding (layout):** [../contract/beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) — byte layout only; Core/Tail split driven by this policy.
- **Minset:** [../contract/link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) — field set and semantics.
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
