# NodeTable — Source precedence rules (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)

This policy removes ambiguity about “who wins” when the same NodeTable fact can come from multiple sources (broadcast vs local vs derived vs restored snapshot vs live observation).

---

## 1) Sources taxonomy

| Source | Definition |
|--------|------------|
| **ObservedRx** | Live RX observation: data extracted from a packet just received from the node (position, last_heard, RSSI/SNR, seq, etc.). Always tied to a reception time and optional seq. |
| **BroadcastClaim** | Data carried in the node’s own beacons (e.g. networkName, role, trackingProfileId). Represents what the node claims about itself. May be absent if not in payload. |
| **LocalUser** | Phone/user annotations: localAlias, Relationship/Affinity (Owned/Trusted/Ignored), manual role or profile override when the product allows it. Stored on the device or in app state. |
| **RestoredSnapshot** | Persisted NodeTable snapshot loaded after reboot or app restart. Contains whatever was saved at snapshot time; may be stale. |
| **Derived** | Computed fields: activityState, expectedIntervalNow, activitySlack, isGrey (UI). Not stored as a source; recomputed from stored facts + policy. |
| **FactoryDefault** | OOTB defaults when no other source provides a value (e.g. default tracking profile, default relationship “Seen”). |

---

## 2) Precedence table by category

For each category, the table gives **effective precedence** (highest wins) and **merge rule** where relevant.

| Category | Primary source(s) | Precedence / rule |
|----------|-------------------|-------------------|
| **Identity (DeviceId / ShortId)** | ObservedRx (from packet from) or RestoredSnapshot. | Identity is immutable per node. DeviceId from first observation or snapshot; ShortId = CRC16(DeviceId). No conflict: only one canonical identity. |
| **Naming (networkName vs localAlias)** | BroadcastClaim (networkName) vs LocalUser (localAlias). | **Display precedence:** networkName > localAlias > ShortId. For storage: both can be stored; display layer applies precedence. If only one is set, use it. |
| **Role / subject type** | BroadcastClaim (role in beacon) vs LocalUser (manual override). | **LocalUser overrides BroadcastClaim** when user has set an override. Otherwise BroadcastClaim. If neither: Derived from hwType or FactoryDefault (e.g. “human”). |
| **trackingProfileId** | BroadcastClaim (profile in beacon) vs LocalUser (user-selected profile). | **LocalUser overrides BroadcastClaim** when user has set a profile. Otherwise BroadcastClaim. If neither: FactoryDefault (OOTB profile). |
| **Scheduler expectations (baseMinTime, maxSilence, expectedIntervalNow)** | Derived from trackingProfileId + channelUtilization. | **Derived** only. No direct BroadcastClaim/LocalUser for these; they come from profile lookup + adaptive logic. Profile choice itself follows role/trackingProfileId precedence above. |
| **Position** | ObservedRx (position in RX) vs RestoredSnapshot (saved position). | **ObservedRx overwrites** when the new RX contains position. RestoredSnapshot position is used only until a newer ObservedRx (or BroadcastClaim position if ever) arrives. Position is **keep-if-newer** by reception time (or position timestamp when available). |
| **Telemetry / Health** | ObservedRx (telemetry in RX) vs RestoredSnapshot. | **ObservedRx overwrites** for uptime, battery, etc. RestoredSnapshot used only until newer ObservedRx. **Keep-if-newer** by reception time. |
| **Relationship / Affinity (Self, Owned, Trusted, Seen, Ignored)** | LocalUser (user marks node) vs RestoredSnapshot (saved affinity). | **LocalUser wins** over RestoredSnapshot. Self is special (determined by DeviceId match). RestoredSnapshot restores user’s past affinity; LocalUser changes overwrite. |
| **Mesh / Link metrics (RSSI, SNR, hops/via, lastOriginSeqSeen)** | ObservedRx only. | **ObservedRx overwrites** each time we get a new RX from that node. No BroadcastClaim or LocalUser for link metrics. RestoredSnapshot may hold last known values but they are stale; first ObservedRx after restore overwrites. |
| **Persistence / Retention flags** | LocalUser (e.g. “keep in map”) vs RestoredSnapshot (saved flags). | **LocalUser wins.** What survives snapshot and how it’s aged is policy (see persistence doc); within that, user-set retention flags override restored values when user has changed them. |

---

## 3) Merge rules (summary)

- **Overwrite:** ObservedRx overwrites stored position, telemetry, and mesh/link metrics when a new RX arrives. No merge; new value replaces old.
- **Keep-if-newer:** For position and telemetry, “newer” is determined by reception time (or payload timestamp when available). RestoredSnapshot is older than any subsequent ObservedRx.
- **RestoredSnapshot vs fresh ObservedRx:** After restore, treat RestoredSnapshot as initial state. Any ObservedRx that applies to that node **updates** the corresponding fields (position, telemetry, last_heard, RSSI/SNR, seq, etc.). RestoredSnapshot does not “block” ObservedRx; ObservedRx always wins for live-observation fields.
- **BroadcastClaim vs LocalUser:** For **naming**, store both; display uses networkName > localAlias. For **role** and **trackingProfileId**, **LocalUser overrides BroadcastClaim** when the user has set an override. Otherwise BroadcastClaim wins. If neither, use FactoryDefault.
- **Time/recency keys:** Use **lastSeenAge** (or equivalent “last heard” time) for activity derivation. For position/telemetry “newer”, use **reception time** of the packet; optionally **position timestamp** or **seq** when available to break ties or detect out-of-order.

---

## 4) Concrete conflict examples

**Example 1: Node broadcasts networkName; user set localAlias**  
- Store both. **Display:** networkName wins (show broadcast name). If networkName is empty, show localAlias, then ShortId. No “conflict” in storage; only display precedence.

**Example 2: Node broadcasts role/profile; user overrides role locally**  
- **LocalUser wins** for role (and profile if user set it). Use user’s role for UI and for deriving tracking profile unless user cleared override. BroadcastClaim is ignored for that field while override is set.

**Example 3: Restored snapshot has old position; live RX arrives without position but updates last_heard**  
- Apply ObservedRx: update last_heard (and RSSI/SNR, lastOriginSeqSeen). **Do not** overwrite position with “no position”; keep existing (restored) position. Position is updated only when the RX **contains** position. So: stored position stays old; lastSeenAge and activityState reflect the new RX (node is “heard” again).

**Example 4: Two RXs in a row — first with position, second without**  
- First RX: store position, last_heard, link metrics. Second RX: update last_heard and link metrics only; **keep** position from first RX (keep-if-newer by reception time: we don’t replace with “unknown”). Position is only replaced when a newer RX carries a position.

**Example 5: Multiple phones annotate the same node (v1 / backlog)**  
- In v0, each phone has its own LocalUser state (localAlias, Owned/Trusted/Ignored). No cross-device sync. “Multiple phones” is out of scope for v0; document as backlog: future sync/merge of LocalUser across devices TBD.

---

## 5) Exit criteria checklist

- [ ] Sources taxonomy (ObservedRx, BroadcastClaim, LocalUser, RestoredSnapshot, Derived, FactoryDefault) defined.
- [ ] Precedence table by category (identity, naming, role, trackingProfileId, scheduler, position, telemetry, relationship, mesh/link, persistence) documented.
- [ ] Merge rules (overwrite, keep-if-newer, RestoredSnapshot vs ObservedRx, BroadcastClaim vs LocalUser, time/recency keys) stated.
- [ ] At least 3–5 concrete conflict examples described.
- [ ] Policy linked from NodeTable WIP index (Links/References).
