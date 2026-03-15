# S05 — Android mobile app (first phase) — WIP

**Status:** WIP (working document for S05)  
**Iteration:** S05__2026-03__Android_Mobile_Foundation_on_BLE.v1  
**Source of truth for BLE:** [ble_contract_s04_v0.md](../docs/product/areas/mobile/contract/ble_contract_s04_v0.md)

This document defines what the mobile app is in the first Android implementation phase, what screens it has, and what user-visible functions it exposes based on **current** S04 BLE capabilities. It is not canon; it is the working specification for S05. Promotion path: WIP → review → canon (e.g. under docs/product/areas/mobile/) → then master issue → slicing → implementation.

---

## 1. Purpose

- Single reference for the first real Android app phase built on the S04 BLE surface.
- Define scope, screens, flows, and exclusions so that implementation can be sliced without scope creep.
- Make the promotion path to canon and subsequent issue/implementation structure explicit.

---

## 2. Phase scope

- **In scope:** Android app that discovers a Naviga node over BLE, connects (with version gate), performs automatic baseline then subscription per BLE contract, shows self and remote nodes, allows self node_name read/write, lists/reads profiles (read-only on profile/config surfaces), and includes a minimal first-version Map. One device, one node; no multi-node BLE in this phase.
- **Out of scope for S05:** iOS, backend, JOIN/Mesh, hunting/scenario UX, profile select/write/create/delete, role/session product layers, full profile editing.

---

## 3. What the mobile app exists to do in S05

1. **Discover** — Scan for BLE devices advertising the Naviga service; show identity (display name + ShortID or ShortID only), RSSI, BLE contract version.
2. **Version gate** — Allow connect only when BLE contract version matches; show “incompatible” when mismatch.
3. **Connect** — Establish GATT connection to one node; discover services/characteristics per contract.
4. **Load NodeTable** — After connect: run baseline (paged NodeTable snapshot) so the app has full initial state. Then start subscription for incremental updates (2 s coalesced, full record).
5. **Show nodes** — List nodes (self + remotes) from NodeTable; show key fields (identity, position quality, last seen, etc.). Detail view for a single node when supported by data.
6. **Self node name** — Read and write the connected node’s authoritative node_name; show in UI and reflect in discovery display identity when set.
7. **Profiles (read-only)** — List radio and user profile IDs; read one profile by (type, id) and show contents (no edit/select/delete in S05). The read-only limitation applies only to profile/config UI surfaces in this iteration; NodeTable paging, read, targeted read, subscription, and update handling are in scope per BLE contract.

---

## 4. What the mobile app explicitly does not do in S05

- No JOIN/Mesh flows or UI.
- No hunting- or scenario-specific screens or logic.
- No profile select, write, create, or delete (profile/config UI is read-only in S05; BLE contract and NodeTable operations are not limited).
- No backend or cloud; no multi-device sync.
- No iOS implementation.
- No reliance on legacy BLE as authority; only S04 canon contract and delivered characteristics.

---

## 5. BLE capabilities available to mobile now

| Capability | Use in app |
|------------|------------|
| Discovery / advertising | Scan, filter by Naviga, show identity + version + RSSI |
| Version gate | Connect only on exact version match; show incompatible otherwise |
| Device info / status | Post-connect device info and status read (optional for first UI) |
| NodeTable baseline | Paged snapshot after connect; full product-facing fields per contract |
| NodeTable subscription | After baseline; 2 s coalesced batches; full record per update |
| Self node_name | Read/write; persist on node; display identity in ad when set |
| Profiles list | List radio and user profile IDs |
| Profile read | Read one profile by (type, id); display only |

---

### 5.1 Post-connect lifecycle policy (canon-aligned)

Per BLE contract §7–§8, the app **must** follow this sequence; there is no product option for “manual baseline only” as the S05 default.

1. **After successful connect and version match:** The app performs **automatic baseline reads** (paged NodeTable snapshot and any other explicit reads required for initial state). No user trigger is required.
2. **After baseline is complete:** The app enables **subscription** for batched NodeTable updates (2 s coalesced, full record per update).
3. **Reconnect:** A new connection is a **fresh session**. The app repeats baseline reads, then re-enables subscription. No missed-update replay or delta recovery in S05.

Optional future iterations may offer an explicit “refresh” or “re-baseline” control; in S05 the default is automatic baseline on connect, then subscribe.

---

## 6. Core user flows

1. **Scan → Connect**  
   User opens app → scan → list of Naviga nodes (identity, RSSI, version) → tap one → version check → connect or show “incompatible”.

2. **Post-connect**  
   Connect → (optional: device info/status) → **automatic baseline** (paged NodeTable load per §5.1) → **subscribe** to NodeTable updates. App state now has full snapshot + live updates.

3. **Nodes**  
   User sees list of nodes (self + remotes); can open detail for a node. Data from NodeTable only; no remote alias editing.

4. **My node / self name**  
   User can view and edit the connected node’s name (self node_name read/write over BLE). Changes persist on node and appear in discovery.

5. **Profiles**  
   User can open profiles: list of radio and user profiles; tap one to read and display content. No edit/select/delete.

---

## 7. Proposed main screens

| Screen | Responsibility |
|--------|----------------|
| **Scan / device list** | Show discovered Naviga devices (identity, RSSI, version); version gate; connect or “incompatible”. |
| **Connected home / dashboard** | Post-connect entry: show connected node identity; shortcuts to Nodes, My Node, Profiles. Optionally show connection status and baseline/subscription state. |
| **Nodes list** | List all nodes from NodeTable (self + remotes); key fields visible; tap → node detail. |
| **Node detail** | Single node view: identity, position/quality, status, last seen, etc. Read-only from NodeTable. |
| **My Node** | Self node: read/write node_name; show current name; persist via BLE. |
| **Profiles** | List radio and user profile IDs; tap to read and display one profile (read-only). |
| **Map** | Minimal first-version map: show node positions from NodeTable (self + remotes); no offline tiles, tracks, or advanced map features in S05. |
| **Connection status / version gate** | Visible connection state and BLE contract version; “incompatible” when version mismatch; baseline/subscription state where useful. |
| **App / Settings / About** | Minimal only: BLE/mobile lifecycle support (e.g. disconnect, permissions), app version, about; no backend/accounts. |

Each screen is justified by the current BLE surface: discovery, NodeTable baseline+subscription, self node_name, profiles list/read, and product-facing NodeTable fields for list/detail/map.

---

## 8. Screen-by-screen responsibilities

- **Scan:** Discovery only; no connect until user selects; version check before connect; clear “incompatible” state.
- **Connected home:** Reflect post-connect lifecycle (baseline done, subscription active); navigate to Nodes, My Node, Profiles.
- **Nodes list:** Consume NodeTable state (baseline + subscription); display self + remotes; no editing of remote data.
- **Node detail:** Display one node’s product-facing fields; no remote node_name edit (contract: remote name via NodeTable only).
- **My Node:** Self node_name read/write; validation per contract (e.g. length, allowlist); show success/failure.
- **Profiles:** List + read only; display profile content; no select/write/create/delete.
- **Map:** Minimal first version: nodes from NodeTable on map; no tracks/offline tiles in S05.
- **Connection status / version gate:** Show incompatible, connecting, baseline loading, live/subscribed, disconnected, reconnect-required as needed.
- **App / Settings / About:** Minimal; support BLE lifecycle and about info only.

---

## 9. Data and state model at app level

- **Connection state:** Connected device, GATT state, version.
- **NodeTable state:** Full snapshot from baseline; incremental updates applied from subscription (upsert by node id). Single source of truth for node list and node detail.
- **Self node_name:** Cached from read; updated after successful write.
- **Profiles:** List of IDs; optionally cached read result per (type, id) for displayed profile.

No offline persistence requirement for S05; in-memory or minimal local cache is enough. Reconnect implies fresh baseline then resubscribe per contract (§5.1).

### 9.1 UI / lifecycle state (product-facing)

| State | Meaning |
|-------|--------|
| **Incompatible** | BLE contract version mismatch; connect not allowed. |
| **Connecting** | GATT connect in progress. |
| **Baseline loading** | Post-connect; paged NodeTable snapshot (and any other baseline reads) in progress. |
| **Live / subscribed** | Baseline complete; subscription active; app receiving batched updates. |
| **Disconnected** | No BLE connection. |
| **Reconnect-required** | Connection lost; user or app may retry; new session will run baseline again. |
| **Explicit refresh / targeted read** | User or app requests refresh of specific record (targeted read); optional in first UI. |

---

## 10. Known constraints / rough edges inherited from S04

- **Baseline on connect:** Policy is fixed for S05: automatic baseline after connect, then subscribe (§5.1). Reference app may have been hook-driven; S05 implementation follows canon.
- **Wire formats:** First-phase formats (NodeTable 72 B, page header, profiles, node_name) are in code/parsers; no single dedicated doc yet. Optional: add short “first-phase wire formats” reference; track in early-phase checks.
- **node_name comment:** If “max 32 bytes” comment still exists in code, fix to 24 or document; implementation and canon use 24.
- **Profile read timing:** ~350 ms delay after write before read is implementation-defined; document where relevant for mobile.
- **Baseline/targeted read:** No delay between page write and read; FW fills on next tick; retry on snapshot mismatch. Mobile should rely on retry or document short delay if needed.

---

## 11. Promotion path to canon

1. **Stabilise this WIP** — Review with product/tech; agree on screens, flows, and exclusions.
2. **Promote to canon** — Move (or copy and trim) to docs/product/areas/mobile/ (e.g. `android_app_s05_v0.md` or similar). Update status to canon; link from current_state and iteration.
3. **Master issue** — Create single S05 implementation issue referencing the canon app spec and BLE contract.
4. **Slicing** — Decompose into technical sub-issues (e.g. connect flow, baseline+subscribe, nodes UI, My Node, Profiles).
5. **Implementation** — Execute slices; no implementation before WIP stabilisation and promotion path clear.

---

## 12. Planned issue structure after WIP stabilisation

- **One master issue** — “S05: Android mobile foundation on BLE” (or similar): scope = this document + BLE contract; deliverables = app with scan, connect, baseline, subscribe, screens above.
- **Sub-issues** — Created when WIP is stable and promoted (or promotion path locked). Examples: connect + version gate; baseline + subscription wiring; Nodes list + detail; My Node (node_name); Profiles list + read. Exact slice count and titles TBD at slicing time.

No technical sub-issues are created in this pass; they follow from the iteration and master issue once the WIP is ready.
