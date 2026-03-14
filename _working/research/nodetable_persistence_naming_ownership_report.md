# NodeTable persistence, naming, and ownership — archaeology report

**Scope:** Repo-only (docs + code). No implementation changes.  
**Date:** 2026-03-03.

---

## 1) What the docs say (paths + short quoted fragments)

### A) NodeTable persistence

- **Is NodeTable persisted to flash/NVS?**  
  **Docs:** Policy WIP describes *intent* for snapshot/restore and where data *would* be persisted, but **no doc states that the firmware currently persists the NodeTable to NVS**.
  - `docs/product/areas/nodetable/index.md` §5: *"Persistence mechanism, snapshot cadence, map relevancy vs remembered nodes"* — listed as **open / future (post V1-A)**.
  - `docs/product/wip/areas/nodetable/policy/snapshot-semantics-v0.md` §3: Defines **persisted (must-save)** vs **derived** vs **prohibited** fields (e.g. DeviceId, ShortId, networkName, localAlias, role, tier, lastKnownPosition, lastSeen, etc.) for a *future* snapshot.
  - `docs/product/wip/areas/nodetable/policy/restore-merge-rules-v0.md`: Merge rules for **RestoredSnapshot** + **ObservedRx** (LocalUser vs node-owned).
  - `docs/product/wip/areas/nodetable/policy/persistence-cadence-limits-v0.md`: Write triggers (immediate for LocalUser changes, rate-limited for session/position), debounce, T2 cap 100, T1 ~50, overall cap.
  - `docs/product/wip/areas/nodetable/policy/persistence-v0.md`: T0/T1/T2 retention tiers; eviction rules (T0 aggressive, T1 inactivity-only, T2 never auto-evicted).
  - `docs/product/wip/areas/nodetable/master_table/README.md`: *"persistence_notes … e.g. BLE snapshot, flash"*; *"WIP: … source-precedence, snapshot-semantics, restore-merge-rules"*.
  - **Conclusion (docs):** Persistence is **designed in WIP policy** (what to persist, merge, cadence, tiers). **Not stated as implemented in firmware.**

- **Eviction rules vs pinned/persisted:**  
  - `docs/product/wip/areas/nodetable/policy/persistence-v0.md`: **T0** = ephemeral, aggressive eviction; **T1** = session, evict only on inactivity threshold (not capacity); **T2** = pinned, **only by user action**, never auto-evicted.
  - `docs/product/wip/areas/nodetable/master_table/fields_v0_1.csv`: **in_use** — *"Internal; not in BLE"*, persisted N, *"internal slot only"*. **is_self** — persisted Y, *"BLE snapshot (flags bit0)"*.
  - Policy docs refer to **tier** (T0/T1/T2), **pin**, **LocalUser**; **no firmware struct field** named "tier" or "pinned" appears in domain NodeTable.

- **Restore/merge/source-precedence:**  
  - `docs/product/wip/areas/nodetable/policy/restore-merge-rules-v0.md`: Merge by DeviceId; node-owned fields = incoming wins; LocalUser (localAlias, tier/pin) never overwritten by incoming.
  - `docs/product/wip/areas/nodetable/policy/source-precedence-v0.md`: **LocalUser** = localAlias, Relationship/Affinity, tier/pin. **Node-owned** = role, trackingProfileId, networkName, position, telemetry. Display: networkName > localAlias > ShortId.

### B) Human naming

- **localAlias:**  
  - `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md`: *"**localAlias** | User-defined on phone for a DeviceId. … Local-only; not sent over radio."*
  - `docs/product/wip/areas/nodetable/master_table/fields_v0_1.csv`: *"localAlias … User, LocalOnly, Canon, AppLocal … persisted Y, app local storage … User-defined on phone; not over radio."*
  - `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` §2.3: *"Human-readable names (networkName, localAlias, any string used for display)"* — **MUST NOT** appear in v0 beacon payload.
  - `docs/product/wip/areas/nodetable/policy/source-precedence-v0.md`: *"Naming (networkName vs localAlias) … Display precedence: networkName > localAlias > ShortId."*
  - `docs/product/wip/areas/session/contract/session_identity_exchange_v0_1.md`: *"**localAlias** and **ShortId** remain local-only; not sent."*
  - **Intended meaning:** User-defined label on the phone for a DeviceId; LocalUser-owned; local-only; not sent over radio; stored in app (persisted in snapshot in policy intent).

- **Node_User_Name / user-assigned name for local (BLE) node to propagate later:**  
  - **Not found** as a defined contract field or packet type.  
  - `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md`: **networkName** — *"Concept: broadcast name; optional; … NOT sent in v0 beacon; MUST NOT be used on OOTB public; future session-join exchange."* So a *broadcast* name exists as concept; not a "name for the local node set by user and later propagated."
  - `docs/firmware/ootb_node_table_v0.md` §5: *"Alias/name: свой alias (self) может храниться на устройстве; alias удалённых нод в seed обрабатываются на стороне приложения (app-local mapping). Будущая синхронизация alias по эфиру (control messages) — вне scope seed."* So **self alias may be stored on device** and future over-air sync is out of scope; no explicit "Node_User_Name" or "local node name to propagate" field name.
  - **Conclusion:** No defined **Node_User_Name** or equivalent field for "user-assigned name for the local (BLE-connected) node intended to be propagated over radio/mesh later." networkName is broadcast/shared concept; localAlias is phone-only for *any* node.

### C) Ownership / claim / pairing

- **Device ownership / claim / binding to account:**  
  - `docs/product/ootb_gap_analysis_v0.md`: *"Device ownership / access control / pairing … Who can connect over BLE and change settings? How does a user 'claim' a device (personal ownership)? … **Seed scope:** out of scope; document as **planned work** (Phase 2+)."*
  - `docs/product/wip/areas/identity/pairing_flow_v0.md`: *"No cryptographic secure claim/provisioning in v0; no BLE/NFC implementation; no code changes. Pairing UX only."* Defines first-time connect, sticker + BLE list, human label (networkName/localAlias), **no** claim/owner/account/binding.
  - **Conclusion (docs):** **No** defined parameter or flow for device ownership (binding phone account/user identity to node_id), preventing others from overwriting settings over BLE, or "claimed/paired/owner" state, keying, PIN, or lock. All deferred to Phase 2+.

- **Provisioning (serial):**  
  - `docs/product/areas/firmware/policy/provisioning_interface_v0.md`: Serial commands for **role** and **radio profile** get/set/reset, **factory reset**; **immediate persistence** of pointers to NVS. No node ownership or BLE write-protect.

---

## 2) What the code does today (paths + key structs/flows)

### A) NodeTable persistence in firmware

- **Location:** `firmware/src/domain/node_table.h`, `firmware/src/domain/node_table.cpp`.
- **Persistence:** **NodeTable is NOT persisted to flash/NVS.** It lives only in RAM (`std::array<NodeEntry, kMaxNodes> entries_`). On reboot the table is empty except for self after `init_self()`.
- **Snapshot in firmware:** `create_snapshot()` / `get_snapshot_page()` build a **consistent BLE export** (snapshot_id, paged 26-byte records). Used only for BLE read; **no NVS write**.
- **NVS in firmware:** `firmware/src/platform/naviga_storage.cpp` (.h): **Only** role/radio **pointers** and **current role profile record** (cadence params) are loaded/saved. Namespace `"naviga"`. No NodeTable or node entries.

### B) Eviction and slots

- **Location:** `firmware/src/domain/node_table.cpp`.
- **Struct:** `NodeEntry` has `in_use`, `is_self` (and identity, position, telemetry fields). **No** tier, pin, or Relationship field.
- **Eviction:** When table is full, `find_free_index()` fails; then `evict_oldest_grey(now_ms)` is used (in `init_self`, `upsert_remote`, `apply_tail1`, `apply_tail2`, `apply_info`). **Rules:** Only **non-self** entries are considered; only **grey** entries (lastSeenAge > expected_interval_s + grace_s) are evictable; among those, **oldest last_seen_ms** is evicted. So: **self is never evicted**; no T2/pinned or T1/session; eviction is "oldest grey first."

### C) Restore/merge

- **Firmware:** No restore of NodeTable from NVS; no merge step. Table is rebuilt from RX only.
- **App:** `app/lib/features/nodes/node_table_cache.dart` — **SharedPreferences** cache of **NodeTable snapshot per deviceId** (key `naviga.node_table_cache.<deviceId>`). Holds `CachedNodeTableSnapshot` (records = list of `NodeRecordV1`). **TTL 10 min**; expired cache is removed and not used. So app has **short-lived cache** of BLE snapshot, not tier/merge/restore from policy.

### D) Human naming in code

- **Firmware:** **No** localAlias, networkName, or name field in `NodeEntry` or in the 26-byte BLE record layout (`node_table.cpp` serialize to 26 B, `kRecordBytes = 26`). Only identity/short_id, flags (is_self, pos_valid, is_grey, short_id_collision), position, lastSeenAge, rssi, lastSeq.
- **App:** `NodeRecordV1` (`app/lib/features/connect/connect_controller.dart`): **No** localAlias or name field. Fields: nodeId, shortId, isSelf, posValid, isGrey, shortIdCollision, lastSeenAgeS, latE7, lonE7, posAgeS, lastRxRssi, lastSeq. BLE list uses **BLE scan device name** (`_deviceName(result)`) for the connect list, not a NodeTable alias.
- **Conclusion:** **localAlias is not implemented** in firmware or app (no field, no storage). Policy and fields_v0_1 say "app local storage"; app has no per-node alias store or NodeRecordV1 alias.

### E) Ownership / claim in code

- **Firmware:** No owner, claim, paired, bond, PIN, or lock. Provisioning shell: role/radio/factory reset only (`firmware/src/services/provisioning_shell.cpp`, `naviga_storage`).
- **App:** No ownership, claim, or write-protect logic found; pairing_flow is UX-only in docs.

---

## 3) Existing fields/concepts found

| Concept | Docs | Firmware | App |
|--------|------|----------|-----|
| **localAlias** | User-defined on phone for a DeviceId; LocalUser; local-only; not over radio; persisted in snapshot (policy intent). | **Not found.** No field in NodeEntry or BLE record. | **Not found.** NodeRecordV1 has no alias; no per-node alias store. |
| **Node_User_Name** (user-assigned name for local node to propagate later) | **Not found** as defined field. networkName = broadcast concept; self alias "may be stored on device" (future) in ootb_node_table_v0. | **Not found.** | **Not found.** |
| **Ownership / claim / pair / lock** | Deferred (Phase 2+). pairing_flow_v0 = UX only; no secure claim. | **Not found.** | **Not found.** |

---

## 4) Mismatches / gaps

- **"Docs claim X, code does Y":**
  - **Persistence:** Policy (snapshot-semantics, restore-merge, persistence-cadence, persistence-v0) describes **what and when to persist** and **tiers (T0/T1/T2)**. **Firmware does not persist NodeTable**; no tiers or pin in struct; eviction is "oldest grey" only, not tier-based.
  - **localAlias:** Fields inventory and master table say localAlias is **Canon**, **AppLocal**, **persisted** (app local storage). **App does not implement** localAlias (no field, no storage). Firmware does not send or store it.
  - **Restore/merge:** Policy defines merge of RestoredSnapshot + ObservedRx. **Firmware has no restore**; app has only a TTL cache of BLE snapshot, no merge with LocalUser/tier.

- **Unclear / not found:**
  - **Where** (if anywhere) snapshot-to-flash or app-side "persisted NodeTable" is planned to be implemented (no ticket or path stated in the read docs).
  - **Explicit** definition of a **user-assigned name for the local (BLE) node** that is **intended to be propagated over radio** later (no Node_User_Name; networkName is broadcast; self alias is mentioned as future).

---

## 5) Minimal clarification questions (no new designs)

1. **NodeTable persistence:** Is the intent to implement NodeTable snapshot/restore in firmware (NVS) and/or in the app (long-lived store with merge), or to keep the current "RAM-only in FW + short TTL cache in app" and document that as the v0 state?

2. **localAlias:** Should the app implement localAlias (per-node user label, app-local storage, display precedence with networkName) as the next step to align with the fields inventory, or is it explicitly deferred?

3. **User-assigned name for local node to propagate:** Is there a product requirement for a single "my node name" (set by user when BLE-connected) that will later be broadcast (e.g. in session or Informative), and if so, is it the same as networkName, or a distinct concept (e.g. "Node_User_Name") that still needs to be defined?

4. **Tiers (T0/T1/T2) and eviction:** Should firmware eviction remain "oldest grey only" for v0, with tier/pin only in app (when/if app persists and merges), or is there a plan to add tier/pinned in the firmware NodeEntry and eviction logic?

5. **Ownership/claim:** Beyond documenting as Phase 2+, is there a chosen direction (e.g. BLE bonding, PIN, account binding, or "no claim in v0") that should be written down so that future work does not assume a different model?

---

*End of report. Sources: docs/product/areas/nodetable/**, docs/product/wip/areas/nodetable/**, docs/product/ootb_gap_analysis_v0.md, docs/product/wip/areas/identity/pairing_flow_v0.md, docs/firmware/ootb_node_table_v0.md; firmware/src/domain/node_table.{h,cpp}, firmware/src/platform/naviga_storage.*{h,cpp}, firmware/src/app/node_table.*; app/lib/features/connect/connect_controller.dart, app/lib/features/nodes/node_table_cache.dart.*
