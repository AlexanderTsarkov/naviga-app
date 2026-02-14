# NodeTable — Source precedence rules (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)

This policy removes ambiguity about “who wins” when the same NodeTable fact can come from multiple sources (broadcast vs local vs derived vs restored snapshot vs live observation).

**Design intent:** Split **node-owned configuration** (device behavior: role, tracking profile) from **phone-owned annotations** (address book, display names, filtering). Role and trackingProfile are node-owned and cannot be overridden remotely; changes require a BT-connected settings update. This prevents accidental remote reclassification of unknown nodes on public channels.

---

## 1) Sources taxonomy

| Source | Definition |
|--------|------------|
| **ObservedRx** | Live RX observation: data extracted from a packet just received from the node (position, last_heard, RSSI/SNR, seq, etc.). Always tied to a reception time and optional seq. |
| **BroadcastClaim** | Data carried in the node’s own beacons (e.g. networkName, role, trackingProfileId). Represents what the node claims about itself. May be absent if not in payload. After a **NodeConfigUpdate**, the node’s persisted config is the source of what it will broadcast; BroadcastClaim reflects that. |
| **NodeConfigUpdate** | Local, BT-connected settings change applied to the node; requires physical/connected access. Results in persisted node config; the node then broadcasts the new values (BroadcastClaim). Not a “remote” override. |
| **LocalUser** | Phone/user annotations: localAlias, Relationship/Affinity (Owned/Trusted/Ignored), and optional UI-only tags (e.g. for grouping). LocalUser does **not** override node-owned effective role or trackingProfileId; those come from BroadcastClaim or FactoryDefault. |
| **RestoredSnapshot** | Persisted NodeTable snapshot loaded after reboot or app restart. Contains whatever was saved at snapshot time; may be stale. |
| **Derived** | Computed fields: activityState, expectedIntervalNow, activitySlack, isGrey (UI). Not stored as a source; recomputed from stored facts + policy. |
| **FactoryDefault** | OOTB defaults when no other source provides a value (e.g. default tracking profile, default relationship “Seen”). For role/profile, FactoryDefault applies only when the node has no configured claim. |

---

## 2) Precedence table by category

For each category, the table gives **effective precedence** (highest wins) and **merge rule** where relevant.

| Category | Primary source(s) | Precedence / rule |
|----------|-------------------|-------------------|
| **Identity (DeviceId / ShortId)** | ObservedRx (from packet from) or RestoredSnapshot. | Identity is immutable per node. DeviceId from first observation or snapshot; ShortId = CRC16(DeviceId). No conflict: only one canonical identity. |
| **Naming (networkName vs localAlias)** | BroadcastClaim (networkName) vs LocalUser (localAlias). | **Display precedence:** networkName > localAlias > ShortId. For storage: both can be stored; display layer applies precedence. If only one is set, use it. |
| **Role / subject type** | BroadcastClaim (role in beacon) or FactoryDefault. | **Effective value MUST come from BroadcastClaim** (or FactoryDefault if no claim). LocalUser may store an annotation/tag for UI grouping but **MUST NOT** override effective role. Changing role requires **NodeConfigUpdate** via BT settings. |
| **trackingProfileId** | BroadcastClaim (profile in beacon) or FactoryDefault. | Same as role: **effective from BroadcastClaim or FactoryDefault**. LocalUser annotation allowed for UI but not override. OOTB defaults apply only when the node has no configured profile/claim. Changing profile requires NodeConfigUpdate via BT. |
| **Scheduler expectations (baseMinTime, maxSilence, expectedIntervalNow)** | Derived from trackingProfileId + channelUtilization. | **Derived** only. No direct BroadcastClaim/LocalUser for these; they come from profile lookup + adaptive logic. Effective profile follows precedence above (BroadcastClaim / FactoryDefault). |
| **Position** | ObservedRx (position in RX) vs RestoredSnapshot (saved position). | **ObservedRx overwrites** when the new RX contains position. RestoredSnapshot position is used only until a newer ObservedRx (or BroadcastClaim position if ever) arrives. Position is **keep-if-newer** by reception time (or position timestamp when available). |
| **Telemetry / Health** | ObservedRx (telemetry in RX) vs RestoredSnapshot. | **ObservedRx overwrites** for uptime, battery, etc. RestoredSnapshot used only until newer ObservedRx. **Keep-if-newer** by reception time. |
| **Relationship / Affinity (Self, Owned, Trusted, Seen, Ignored)** | LocalUser (user marks node) vs RestoredSnapshot (saved affinity). | **LocalUser wins** over RestoredSnapshot (per-phone). Self is special (determined by DeviceId match). **Gating:** Node-owned settings changes (role, profile) are only possible when device is connected (BT) and typically when relationship is Owned; Trusted may require explicit confirmation; Seen cannot change node settings. |
| **Mesh / Link metrics (RSSI, SNR, hops/via, lastOriginSeqSeen)** | ObservedRx only. | **ObservedRx overwrites** each time we get a new RX from that node. No BroadcastClaim or LocalUser for link metrics. RestoredSnapshot may hold last known values but they are stale; first ObservedRx after restore overwrites. |
| **Persistence / Retention flags** | LocalUser (e.g. “keep in map”) vs RestoredSnapshot (saved flags). | **LocalUser wins.** What survives snapshot and how it’s aged is policy (see persistence doc); within that, user-set retention flags override restored values when user has changed them. |

---

## 3) Merge rules (summary)

- **Overwrite:** ObservedRx overwrites stored position, telemetry, and mesh/link metrics when a new RX arrives. No merge; new value replaces old.
- **Keep-if-newer:** For position and telemetry, “newer” is determined by reception time (or payload timestamp when available). RestoredSnapshot is older than any subsequent ObservedRx.
- **RestoredSnapshot vs fresh ObservedRx:** After restore, treat RestoredSnapshot as initial state. Any ObservedRx that applies to that node **updates** the corresponding fields (position, telemetry, last_heard, RSSI/SNR, seq, etc.). RestoredSnapshot does not “block” ObservedRx; ObservedRx always wins for live-observation fields.
- **BroadcastClaim vs LocalUser:** For **naming**, store both; display uses networkName > localAlias. For **role** and **trackingProfileId**, **LocalUser annotations do not override** node-owned effective values; effective value is always from BroadcastClaim or FactoryDefault. To change role/profile, the user must connect via BT and apply a **NodeConfigUpdate**; the node’s persisted config and subsequent BroadcastClaim then become the effective truth.
- **Time/recency keys:** Use **lastSeenAge** (or equivalent “last heard” time) for activity derivation. For position/telemetry “newer”, use **reception time** of the packet; optionally **position timestamp** or **seq** when available to break ties or detect out-of-order.

---

## 4) Concrete conflict examples

**Example 1: Node broadcasts networkName; user set localAlias**  
- Store both. **Display:** networkName wins (show broadcast name). If networkName is empty, show localAlias, then ShortId. No “conflict” in storage; only display precedence.

**Example 2: Node broadcasts role/profile; user wants to “override” or change it**  
- **LocalUser** can store a tag/label for UI grouping only; the **effective role** (and trackingProfileId) **remains from BroadcastClaim**. To change role or profile, the user must connect to the node via BT and update settings (**NodeConfigUpdate**); the node then broadcasts the new values.

**Example 3: Restored snapshot has old position; live RX arrives without position but updates last_heard**  
- Apply ObservedRx: update last_heard (and RSSI/SNR, lastOriginSeqSeen). **Do not** overwrite position with “no position”; keep existing (restored) position. Position is updated only when the RX **contains** position. So: stored position stays old; lastSeenAge and activityState reflect the new RX (node is “heard” again).

**Example 4: Two RXs in a row — first with position, second without**  
- First RX: store position, last_heard, link metrics. Second RX: update last_heard and link metrics only; **keep** position from first RX (keep-if-newer by reception time: we don’t replace with “unknown”). Position is only replaced when a newer RX carries a position.

**Example 5: Multiple phones annotate the same node (v1 / backlog)**  
- In v0, each phone has its own LocalUser state (localAlias, Owned/Trusted/Ignored). No cross-device sync. “Multiple phones” is out of scope for v0; document as backlog: future sync/merge of LocalUser across devices TBD.

---

## 5) Exit criteria checklist

- [ ] Sources taxonomy (ObservedRx, BroadcastClaim, NodeConfigUpdate, LocalUser, RestoredSnapshot, Derived, FactoryDefault) defined.
- [ ] Precedence table by category (identity, naming, role, trackingProfileId, scheduler, position, telemetry, relationship, mesh/link, persistence) documented.
- [ ] Merge rules (overwrite, keep-if-newer, RestoredSnapshot vs ObservedRx, BroadcastClaim vs LocalUser, time/recency keys) stated.
- [ ] At least 3–5 concrete conflict examples described.
- [ ] Policy linked from NodeTable WIP index (Links/References).
