# NodeTable — Restore/merge rules (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 3

This doc defines how to merge **persisted snapshot** (loaded on startup) with **newly received RX updates/events** after restore, while respecting source precedence ([#156](source-precedence-v0.md)) and retention tiers ([persistence-v0](persistence-v0.md)). It does **not** define cadence, flash limits, or write batching (Step 4), nor snapshot format/schema (Step 2).

---

## 1) Scope guardrails

- **In scope (Step 3 only):** Deterministic per-field merge rules when combining **RestoredSnapshot** with **ObservedRx** (incoming node-origin updates). Tier interaction with merge; identity, ownership, timestamp, and null handling.
- **Out of scope:** Snapshot format/schema (Step 2). Write cadence, flash limits, batching, eviction algorithms (Step 4). No normative OOTB/UI; examples only where needed.

---

## 2) Concepts

- **Snapshot record:** A node record loaded from a persisted snapshot (RestoredSnapshot). Contains last-known values at snapshot time; may be stale.
- **Incoming update:** Data from a newly received RX (ObservedRx) or from BroadcastClaim in that RX — i.e. node-origin data. Does **not** include LocalUser actions (those are applied separately by the user/phone).
- **Field ownership/source (per [#156](source-precedence-v0.md)):** **Node-owned** fields (role, trackingProfileId, networkName, position, telemetry, mesh/link metrics) come from the node (BroadcastClaim or ObservedRx). **LocalUser** fields (localAlias, Relationship/Affinity, tier/pin, session markers as user-set) are phone/user annotations and are **never** overwritten by incoming node data.
- **Conflict:** The same logical field has a value from the snapshot and a value from an incoming update; merge must decide which wins or how to combine.

---

## 3) Merge rules (normative, deterministic)

### 3.1 Per-field precedence

| Ownership | Rule |
|-----------|------|
| **Node-owned fields** | Accept **incoming** node-origin update over snapshot when the incoming value is present and applicable. Snapshot is **last-known** only; incoming ObservedRx/BroadcastClaim is the live source of truth for node-owned data. |
| **LocalUser fields** | **Never** overwritten by incoming node data. Only changed by LocalUser action (e.g. user edits alias, pins node, changes affinity). Restored snapshot value is kept until the user changes it. |

### 3.2 Timestamp / recency

- For **node-owned** fields (position, telemetry, lastSeen, link metrics): prefer **newer** when timestamps are comparable (reception time of packet or payload timestamp). **Incoming** RX is assumed newer than snapshot unless known otherwise.
- **When timestamps are missing:** Incoming wins if it is plausibly fresher (e.g. we just received the packet). Otherwise keep snapshot. Simple rule: **incoming wins** for node-owned fields when the update is from a live RX; snapshot wins only when there is no incoming value for that field (see null/unknown below).

### 3.3 Null / unknown

- **Incoming “unknown” or missing value must NOT erase a known snapshot value** unless there is an explicit delete/reset signal (e.g. node reports “no position” in a defined way). Example: RX updates last_heard and link metrics but has no position — **keep** lastKnownPosition from snapshot; do not replace with null.
- Apply **per field**: only overwrite a snapshot field when the incoming update **carries a value** for that field (or an explicit clear). Missing/absent in payload is not a write.

### 3.4 Identity

- **Merge by DeviceId only.** The canonical key for matching a snapshot record to an incoming update is **DeviceId**. No merge key on ShortId.
- **ShortId** is not identity; it may be updated as **last-known/cached** when incoming provides a value (e.g. derived from same DeviceId or from payload). ShortId may change for collision disambiguation; stability of identity is DeviceId.

---

## 4) Tier interaction rules

- **Restoring snapshot must not auto-upgrade tiers** without a valid signal. A node restored as T0 stays T0 until tier assignment rules (e.g. session membership or user pin) promote it. A node restored as T1 or T2 keeps that tier until policy or user action changes it.
- **T2 (pinned)** persists across restore; merge does not demote T2 unless the user unpins. Incoming node data does not change tier; only LocalUser or session/tier policy does.
- **T1 (session)** membership ends only by **explicit** membership end or by the inactivity policy defined in [persistence-v0](persistence-v0.md) (session-level inactivity threshold). Do not invent thresholds here; reference persistence-v0 for when T1 may be evicted or demoted.
- **Incoming data** may not downgrade or upgrade tier by itself. Tier changes follow [persistence-v0](persistence-v0.md): T2 only by user action; T1 by session/inactivity policy; T0 by default for new RX. Merging in new RX updates **field values** (position, telemetry, etc.) but does not by itself change tier.

---

## 5) Position / activity / telemetry guidelines

- **Position:** Incoming **newer** position overwrites lastKnownPosition. **Missing** position in incoming RX does **not** delete or clear lastKnownPosition; keep snapshot (or previous) value. Keep-if-newer by reception time or position timestamp when available.
- **Activity / stale:** **Derived** (activityState, lastSeenAge, staleness). Recomputed after merge from lastSeen and policy-supplied boundary; **not** persisted. Merge does not carry derived values; they are recomputed from the merged stored facts.
- **Telemetry (uptime, battery, etc.):** Treat as **last-known**. Incoming overwrites if newer (and value present). Missing telemetry in incoming does **not** erase snapshot value.

---

## 6) Examples

**Example 1: Pinned node restored, then RX updates position**  
Snapshot has node (DeviceId X) in T2 with old lastKnownPosition and lastSeen. After restore, an RX from X arrives with new position. **Merge:** Update position, lastSeen, and link metrics from RX. Keep T2, localAlias, and all LocalUser fields from snapshot. Tier unchanged; position refreshed.

**Example 2: Node renames itself; LocalUser alias preserved**  
Snapshot has networkName “OldName”, localAlias “My Dog”. Incoming RX carries BroadcastClaim networkName “NewName”. **Merge:** Store networkName = “NewName” (incoming wins for node-owned). Keep localAlias = “My Dog” (LocalUser; not overwritten by incoming). Display precedence (networkName > localAlias) is unchanged; display shows “NewName” unless product chooses to show local alias in some contexts.

**Example 3: ShortId changes; DeviceId stable**  
Snapshot has DeviceId D, ShortId S1 (e.g. from CRC16(D) or cached). After restore, incoming RX from same DeviceId D carries or implies a different ShortId S2 (e.g. collision disambiguation). **Merge:** Identity is DeviceId D; record stays keyed by D. Update ShortId to S2 as last-known/cached. No new node record; same logical node.

---

## 7) Open questions / TODO (Step 4 and future)

- **Step 4 (cadence/limits):** When to take snapshot; flash/write limits; batching; eviction order at capacity. Out of scope for Step 3.
- **Out-of-order RX:** When reception time or seq suggests an “older” packet arrives after a “newer” one; optional tie-breaking or ignore-old rules (implementation detail; keep merge deterministic).
- **Future identity merge:** Cross-device or multi-source identity resolution (e.g. same node from two radios) — out of scope for v0.

---

## 8) Related

- **Source precedence:** [source-precedence-v0.md](source-precedence-v0.md) — [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)
- **Retention tiers:** [persistence-v0.md](persistence-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 1
- **Snapshot semantics (what is persisted/derived):** [snapshot-semantics-v0.md](snapshot-semantics-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 2
- **NodeTable contract (WIP):** [../index.md](../index.md)
