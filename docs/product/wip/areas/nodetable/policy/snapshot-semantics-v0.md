# NodeTable — Snapshot/restore semantics (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 2

This doc defines **what** NodeTable data is persisted in a snapshot, what is **derived** (recomputed on restore), and what **must not** be persisted. It does **not** define merge rules between snapshot and new RX events (Step 3), nor write cadence, flash limits, or batching (Step 4).

---

## 1) Scope guardrails

- **In scope (Step 2 only):** Classification of NodeTable fields into **persisted** (must-save), **derived** (recomputed on restore), and **prohibited** (must NOT persist); snapshot unit and identity; restore invariants (no merge rules).
- **Out of scope:** Merge rules when combining RestoredSnapshot with new ObservedRx (Step 3). Write cadence, flash limits, batching, eviction algorithms (Step 4). Serialization format or schema.

---

## 2) Snapshot unit & identity

- **Unit:** One snapshot contains a set of **node records**. Each record corresponds to one node present in the live NodeTable at snapshot time.
- **Identity (key):** Per the NodeTable contract, the stable identity for a node is **DeviceId** (primary key). **ShortId** = CRC16(DeviceId) is derivable and may be stored for convenience; the canonical key for persist/restore is **DeviceId**. No separate “nodeId” beyond DeviceId.

---

## 3) Persisted vs Derived vs Prohibited

| Category | Fields / examples | Rationale |
|----------|-------------------|-----------|
| **Persisted (must-save)** | **Identity:** DeviceId (required), ShortId (optional, derivable). **Naming:** networkName (from BroadcastClaim at snapshot time), localAlias (LocalUser). **Role/profile:** role, trackingProfileId as last known (BroadcastClaim or FactoryDefault). **LocalUser annotations:** Relationship/Affinity (Owned/Trusted/Seen/Ignored), tier (T0/T1/T2), pin/session markers as defined by policy. **Position:** lastKnownPosition (coordinates + quality/age) and optional position timestamp. **Time:** lastSeen (timestamp or equivalent) so lastSeenAge can be derived after restore. **Session membership:** markers indicating T1 membership if policy models them. **Telemetry/health:** uptime, battery (if policy includes them in snapshot). | Needed to restore “last known” state and to respect tier/affinity so eviction and display behave correctly. Precedence (node-owned vs LocalUser) follows [#156](source-precedence-v0.md). |
| **Derived (recomputed on restore)** | **Activity:** activityState (Online/Uncertain/Stale/Archived), lastSeenAge, staleness; any policy-supplied boundary inputs used for derivation. **Scheduler expectations:** expectedIntervalNow, maxSilence, activitySlack (if a policy uses them). **UI/display:** isGrey or equivalent. **Mesh/link metrics:** RSSI, SNR, hops/via, lastOriginSeqSeen — recomputed or overwritten on first ObservedRx after restore; no need to treat as authoritative in snapshot. | Not stored as source of truth; recomputed from persisted facts + policy (and, for link metrics, from next RX). |
| **Prohibited (must NOT persist)** | **Per-packet / mesh-dedup state:** e.g. PacketState(origin_id, seq) for dedup/ordering — belongs to Mesh protocol state, not NodeTable. **Transient caches:** any short-lived cache that is invalid across reboot. **“Evicted” as a field:** eviction is an outcome (record removed); snapshot contains only nodes that are in the table at snapshot time — do not persist an “evicted” flag for nodes no longer in the table. | Keeps snapshot aligned with NodeTable boundaries and avoids persisting ephemeral or protocol-internal state. |

---

## 4) Precedence & ownership note

- Persisted data **must** respect source precedence ([source-precedence-v0](source-precedence-v0.md), [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)): **node-owned** (role, trackingProfileId) vs **LocalUser** (localAlias, Relationship, tier/pin). When writing a snapshot, store the **effective** values that reflect that precedence (e.g. role from BroadcastClaim or FactoryDefault; localAlias and affinity from LocalUser). Restore loads these as last-known; merge with new ObservedRx is Step 3.

---

## 5) Restore invariants (no merge rules)

- **Derived fields are recomputed** after load; persisted fields are loaded as **last-known** state. No requirement here for how merge with new RX works; only that restore produces a consistent NodeTable state from the snapshot alone for persisted fields.
- **Restoring a snapshot must not resurrect evicted nodes into a higher tier without a policy signal.** If a node was evicted before the snapshot (and thus not in the snapshot), restore does not add it. If a node is in the snapshot with tier T0/T1/T2, restore restores that tier; policy (and Step 3 merge rules) govern what happens when new RX arrives later.
- **Self (DeviceId match)** is determined by identity, not restored from a “self” flag in isolation — the node record whose DeviceId matches the local device is self.

---

## 6) Open questions / TODO (Step 3–4)

- **Merge rules (Step 3):** How RestoredSnapshot and ObservedRx combine for each category (overwrite, keep-if-newer, LocalUser wins, etc.); when to replace restored position/telemetry/link metrics with new RX.
- **Cadence & flash (Step 4):** When to take a snapshot; flash limits; batching; eviction ordering when at capacity.
- **Session markers:** Exact representation of T1 session membership in persisted form (if stored).
- **Telemetry in snapshot:** Policy decision on whether uptime/battery are included in snapshot or only live.

---

## 7) Related

- **NodeTable contract (WIP):** [../index.md](../index.md)
- **Retention tiers & eviction:** [persistence-v0.md](persistence-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 1
- **Source precedence:** [source-precedence-v0.md](source-precedence-v0.md) — [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)
