# WIP: NodeTable — Define & Research

Product Contract v0 for [Issue #147](https://github.com/AlexanderTsarkov/naviga-app/issues/147). No promotion to canon yet.

---

## 1) Definition

NodeTable is the **single source of truth / knowledge about all nodes (including self)**. Other components emit observations or events; NodeTable stores best-known facts and derived state used for product and UX decisions.

---

## 2) What product needs from NodeTable (expanded)

**Core questions:** Identity; Activity; Position; Capabilities/HW; Radio context; Trust/freshness; Debuggability.

**Expanded categories:**

1. **Identity** — Primary key (DeviceId), display id (ShortId), human naming (networkName, localAlias).
2. **Roles / Subject type** — Human / dog / repeater / infra, independent of hwType. Source may be broadcast or local.
3. **Activity** — Derived state (Online / Uncertain / Stale / Archived); requires a **policy-supplied staleness boundary** (see below).
4. **Expected beacon/update policy per node** — A policy may supply **expectedInterval**, **maxSilence**, or a **staleness boundary** (and possibly min-move/min-time) to derive activity; may be global default or per-node when known.
5. **Position** — Where (2D required; altitude optional); PositionQuality (age, optional satCount/hdop/accuracy/fixType, source-tagged).
6. **Mesh / Link metrics** — Receiver-side **RSSI + SNR**, **hops/via**, and origin-stream metadata **lastOriginSeqSeen** (last seq seen from that origin; not per-packet cache).
7. **Telemetry / Health** — Minimum: **battery level** (future) + **uptime** (already exists); optional device metrics.
8. **Capabilities / HW** — hwType (if known) + optional fwVersion; capabilities derivable from lookup tables.
9. **Radio context** — lastSeenRadioProfile (profile + txPower when available); RadioProfileId + RadioProfileKind.
10. **Relationship / Affinity** — **Self** / **Owned** / **Trusted** / **Seen** / **Ignored** to handle public OOTB channels and UI filtering.
11. **Retention & Persistence** — Snapshot/restore so reboot or battery loss doesn’t require rebuilding the map; define **map relevancy** vs **remembered nodes**.
12. **Trust / freshness** — How fresh and reliable each fact is.
13. **Debuggability** — Stable identifiers and metadata for logs and support.

---

## 3) Identity (v0)

- **DeviceId** (primary key) = ESP32-S3 MCU ID. In firmware this is the node identity (e.g. `node_id`, uint64).
- **ShortId** = CRC16(DeviceId). Display-only; may collide across nodes.
- **ShortId collision handling:** UI disambiguation via suffixes, e.g. `6DA2:1`, `6DA2:2`. DeviceId remains the source of truth.

---

## 4) Human naming (v0)

- **networkName** — Broadcast by the node; optional; has priority when present.
- **localAlias** — User-defined on the phone for a DeviceId.
- **Effective display name precedence:** `networkName` > `localAlias` > `ShortId` (+ suffix if collision).

---

## 5) Activity (v0)

- **activityState** is **derived**, not stored.
- Four states: **Online** / **Uncertain** / **Stale** / **Archived**.
- Derivation: **activityState** is derived from **lastSeenAge** relative to a **staleness boundary** supplied by policy (e.g. tracking profile, OOTB policy, or another consumer policy). Exact boundary values are policy-defined.
- NodeTable provides **lastSeenAge** for UI (e.g. “last seen 30s ago”).
- **“Grey”** is a UI convention (e.g. dimming stale nodes), not a domain field; domain exposes lastSeenAge and policy so UI can compute grey.

### Tracking Profile / Expected update policy

Activity expectations are derived from **roles + distance granularity + speed hints + channel utilization**. A **Tracking Profile** (or role-driven profile) encodes this. Terminology: **beacon scheduler timings** (when to send) vs **activity interpretation timings** (how to interpret silence).

**1) Tracking Profile (concept)**
- **trackingProfileId** (or role-driven profile) defines:
  - **minDistMeters** — spatial granularity (how often we expect position to “move” meaningfully).
  - **speedHintMps** — assumed typical speed for the subject/role (m/s); part of the profile.
  - **roleMultiplier** — produces **maxSilenceSeconds** (upper bound of acceptable beacon silence; used by scheduler and activity interpretation).
  - **priority** — how strongly this node resists airtime throttling when channel is busy.

**2) Derived timings (beacon scheduler)**
- **baseMinTimeSeconds** ≈ minDistMeters / speedHintMps (rounded to sensible steps). Example: 100 m at ~5 km/h ≈ 1.4 m/s → ~72 s; round to e.g. 60 s or 90 s.
- **maxSilenceSeconds** = baseMinTimeSeconds × roleMultiplier (upper bound of beacon silence).
- **expectedIntervalNowSeconds** is **adaptive** and **clamped** between baseMinTimeSeconds and maxSilenceSeconds: if **channelUtilization** > threshold, increase stepwise toward maxSilenceSeconds; decrease back when utilization drops.

**3) Activity interpretation (one policy mechanism)**
- A policy may define the staleness boundary using an **activitySlack**-style extension: e.g. **activitySlackSeconds = kSlack × maxSilenceSeconds**, **kSlack ≥ 1**, as an extra window beyond maxSilence before treating silence as “stale”. This is one possible way to compute the boundary, not a required domain field.

**4) NodeTable usage**
- NodeTable derives **activityState** (Online / Uncertain / Stale / Archived) from **lastSeenAge** and the **staleness boundary** supplied by the selected policy. A policy may use **expectedIntervalNow**, **maxSilence**, **activitySlack**, or other inputs to compute that boundary; NodeTable does not require any specific set of fields unless the chosen policy does.
- Silence is only “unexpected” relative to the **policy-supplied** expectation/boundary.

**Example profiles (illustrative / TBD)**
- **Hiking OOTB:** minDist 50–100 m, speedHint ~5 km/h.
- **Dog tracking:** dog minDist ~25 m, higher speedHint; handler minDist ~250 m.
- **Hunting:** dog 5–10 m; beater 15–20 m; shooter 25–50 m (each with role multipliers / priorities).

---

## 6) Position (v0)

- **2D required;** altitude optional.
- **PositionQuality:** age required; optional satCount, hdop, accuracy, fixType; **source-tagged** (e.g. GNSS vs network).

---

## 7) Capabilities / HW (v0)

- **hwType** (if known) + optional **fwVersion**.
- Capabilities may be derived from hwType/version lookup tables (e.g. “supports X”).

---

## 8) Radio context (v0)

- **lastSeenRadioProfile** includes profile and txPower when available.
- Multiple profile kinds: e.g. UART presets vs full LoRa params.
- Store normalized **RadioProfileId** + **RadioProfileKind**.
- UI shows only valid controls; mapping tables exist (WIP).

---

## 9) Boundaries

- **NodeTable** stores **node-level** knowledge only.
- **Per-packet mesh cache** — e.g. PacketState(origin_id, seq) for dedup / ordering — belongs to **Mesh protocol state**, not NodeTable.
- **Seq in NodeTable** is only **stream-level metadata**: e.g. **lastOriginSeqSeen** (last sequence number seen from that origin). Not per-packet state.

---

## 10) Decisions log (v0)

- NodeTable = single source of truth for node-level facts; others emit observations.
- Identity: DeviceId (primary), ShortId = CRC16(DeviceId), display precedence networkName > localAlias > ShortId.
- Roles/Subject type: human/dog/repeater/infra; source broadcast or local.
- Activity: derived (Online/Uncertain/Stale/Archived) from lastSeenAge vs a **policy-supplied staleness boundary**; lastSeenAge for UI; “grey” is UI, not domain. A policy may use tracking profile (expectedIntervalNow, maxSilence, activitySlack) or other means to define the boundary.
- Mesh/link: RSSI, SNR, hops/via, lastOriginSeqSeen; no per-packet state in NodeTable.
- Telemetry/health: uptime + battery (future); optional device metrics.
- Relationship: Self / Owned / Trusted / Seen / Ignored for OOTB and UI filtering.
- Retention: snapshot/restore; map relevancy vs remembered nodes.
- Boundary: PacketState(origin_id, seq) in Mesh; NodeTable holds only node-level and lastOriginSeqSeen.

---

## 11) Open questions / Follow-ups

- **Role / policy source & precedence** — Broadcast vs local; how to merge.
- **Tracking profile** — Where it is set (broadcast vs local assignment) and precedence; which defaults ship OOTB.
- **Channel utilization** — How it is computed and exposed (firmware-only vs shared with mobile/UI).
- **Persistence mechanism and snapshot cadence** — When to snapshot, restore semantics, map relevancy vs remembered nodes.
- networkName propagation cadence and payload.
- Exact activity thresholds (bounds for Online / Uncertain / Stale / Archived).

---

## Status (registries #159)

- **Decisions captured (v0):** HW Capabilities registry (hw_profile_id, adapter_type, capability, confidence; local vs remote disclosure; schema rev, unknown hw id → prompt for update) and RadioProfiles + ChannelPlan registry (user abstraction Default/LongDist/Fast; profile–channel compatibility; registries = facts, SelectionPolicy = choice rules; non-goals: no raw LoRa UI, no real CAD/LBT here, sense OFF + jitter default for UART).
- **Follow-up issues / docs needed:** SelectionPolicy (choice rules, autopower, throttling, defaults); autopower algorithm; identity/pairing flow and use of local vs remote disclosure; registry ownership and format (firmware vs mobile vs backend).

---

## Promotion criteria (WIP → canon)

When decisions above are stable and reflected in implementation, promote to `docs/product/areas/nodetable.md`.

---

## Glossary (v0)

| Term | Meaning |
|------|--------|
| **DeviceId** | Primary key for a node (e.g. ESP32-S3 MCU ID; uint64). |
| **ShortId** | Display id derived from DeviceId (e.g. CRC16); may collide. |
| **expectedIntervalNow** | Current expected time between beacons for this node (adaptive; clamped between baseMinTime and maxSilence). |
| **maxSilence** | Upper bound of acceptable beacon silence for this node (from tracking profile). |
| **activitySlack** | One policy mechanism to extend a boundary beyond maxSilence; not a required domain field unless a selected policy uses it. Example: activitySlackSeconds = kSlack × maxSilenceSeconds, kSlack ≥ 1. |
| **lastSeenAge** | Time since last RX from this node (seconds); used for activity derivation and UI. |
| **role** | Subject/device type (e.g. human, dog, repeater, infra); may drive tracking profile. |
| **trackingProfileId** | Id of the tracking profile (minDist, speedHint, roleMultiplier, priority) for this node. |

---

## Links / References

- Research: [research/](research/)
- Policy: [policy/source-precedence-v0.md](policy/source-precedence-v0.md) (source precedence v0).
- Policy: [policy/ootb-profiles-v0.md](policy/ootb-profiles-v0.md) (OOTB tracking profiles & activity/adaptation policy v0; non-normative example policy).
- Policy: [policy/persistence-v0.md](policy/persistence-v0.md) (retention tiers & eviction v0, [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157)).
- Policy: [policy/snapshot-semantics-v0.md](policy/snapshot-semantics-v0.md) (snapshot/restore semantics v0, [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 2).
- Policy: [policy/restore-merge-rules-v0.md](policy/restore-merge-rules-v0.md) (restore/merge rules v0, [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 3).
- Policy: [policy/persistence-cadence-limits-v0.md](policy/persistence-cadence-limits-v0.md) (persistence cadence & limits v0, [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 4).
- Contract: [contract/link-telemetry-minset-v0.md](contract/link-telemetry-minset-v0.md) (Link/Metrics & Telemetry/Health minset v0, [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158)).
- Registry: [../hardware/registry_hw_capabilities_v0.md](../hardware/registry_hw_capabilities_v0.md) (HW Capabilities registry v0, [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)).
- Registry: [../radio/registry_radio_profiles_v0.md](../radio/registry_radio_profiles_v0.md) (RadioProfiles & ChannelPlan registry v0, [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)).
- Issue: [#147 NodeTable — Define & Research (Product WIP)](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
