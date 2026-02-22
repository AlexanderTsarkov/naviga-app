# S02.6 — Audit: mobile NodeTable integration vs V1-A contract/policies

**Status:** Canon (audit record).  
**Date:** 2026-02-22  
**Issue:** [#230](https://github.com/AlexanderTsarkov/naviga-app/issues/230) · **Epic:** [#224](https://github.com/AlexanderTsarkov/naviga-app/issues/224) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

**Goal:** Record integration points between the mobile app (Flutter) and NodeTable: BLE consumption, parsing, state, and UI; cross-check against V1-A NodeTable contract and policies. No code or spec semantics changes.

---

## 1) Scope & non-goals

**Scope:** Identify where the app receives NodeTable data (BLE), parses it, stores it, and displays it; compare field usage and semantics to [NodeTable hub](../../nodetable/index.md) and linked contracts/policies.

**Non-goals:** No mobile code changes; no NodeTable contract/policy semantic changes; no new fields, encoding, or protocol; no BLE functional changes; no mesh/JOIN/CAD/LBT/post-V1A.

---

## 2) Data flow map (BLE → parsing → state → UI)

| Stage | Location (path / symbol) | Role |
|-------|---------------------------|------|
| **BLE discovery** | `app/lib/features/connect/connect_controller.dart` · `kNavigaNodeTableSnapshotUuid`, `_nodeTableSnapshotCharacteristic` | GATT characteristic for NodeTableSnapshot (read after write request). |
| **Fetch** | `app/lib/features/connect/connect_controller.dart` · `_writeNodeTableRequest(snapshotId, pageIndex)`, `_readNodeTablePage()`, `_fetchNodeTableSnapshot()`, `fetchNodeTableSnapshot()` | Write snapshot_id + page_index; read payload; loop pages; parse each page. |
| **Parse** | `app/lib/features/connect/connect_controller.dart` · `BleNodeTableParser.parsePage()`, `NodeRecordV1`, `NodeTableSnapshotHeader`, `NodeTableSnapshotPage` | Header: snapshotId, totalNodes, pageIndex, pageSize, pageCount, recordFormatVer (10 bytes). Record: 26 bytes × N (nodeId, shortId, flags, lastSeenAgeS, latE7, lonE7, posAgeS, lastRxRssi, lastSeq). |
| **Repository** | `app/lib/features/nodes/node_table_repository.dart` · `NodeTableRepository`, `fetchLatest()` | Delegates to fetcher (ConnectController); returns `List<NodeRecordV1>`. |
| **Fetcher** | `app/lib/features/nodes/node_table_fetcher.dart` · `ConnectControllerNodeTableFetcher`, `fetch()` | Calls `_controller.fetchNodeTableSnapshot()`. |
| **Controller / state** | `app/lib/features/nodes/nodes_controller.dart` · `NodesController`, `refresh()`, `_sortRecords()`, `_findSelf()`; `app/lib/features/nodes/nodes_state.dart` · `NodesState` (records, recordsSorted, self, snapshotId, isFromCache, cacheAgeMs) | Fetches, sorts (self first, then by lastSeenAgeS, then nodeId), finds self; caches per device. |
| **Cache** | `app/lib/features/nodes/node_table_cache.dart` · `NodeTableCacheStore`, `CachedNodeTableSnapshot`, `save()` / `restore()` | Persists snapshot by deviceId (SharedPreferences); TTL 10 min. |
| **UI list** | `app/lib/features/nodes/nodes_screen.dart` · `NodesScreen`, list of `NodeRecordV1` (title: nodeId; subtitle: shortId, lastSeenAgeS, “(self)”); `posValid` + lat/lon → “Show on map”. | Displays records; tap → NodeDetailsScreen or MyNode tab. |
| **UI details** | `app/lib/features/nodes/node_details_screen.dart` · `NodeDetailsScreen`, `_findRecord()`; labels: Node ID, Short ID, Is self, Pos valid, Is grey, Short ID collision, Last seen age (s), Lat/Lon (e7), Pos age (s), Last RX RSSI, Last seq. | All NodeRecordV1 fields shown. |
| **UI self** | `app/lib/features/my_node/my_node_screen.dart` · self record, “Last RX RSSI”, “Last seq”, “NodeTable (self)” section. | Self from NodeTable + DeviceInfo/Status. |
| **Map** | `app/lib/features/map/map_screen.dart` · uses records with `posValid` and non-zero lat/lon; “No nodes with position yet. Connect and refresh NodeTable.” | Positions from NodeRecordV1 (latE7, lonE7). |
| **Connect list** | `app/lib/features/connect/connect_screen.dart` · scan results show “RSSI ${device.rssi} dBm • ${lastSeenSeconds}s ago” (scan-level RSSI, not NodeTable). | BLE scan only; not NodeTable. |

**Firmware source of NodeTable snapshot:** `firmware/src/domain/node_table.cpp` · `get_snapshot_page()` (26-byte records: node_id, short_id, flags, last_seen_age_s, lat_e7, lon_e7, pos_age_s, last_rx_rssi, last_seq); `firmware/protocol/ble_node_table_bridge.cpp` · page request/response. Mobile parser layout matches firmware 26-byte record and 10-byte page header.

---

## 3) Alignment (matches)

| Policy / contract | Evidence (file path + symbol) |
|-------------------|--------------------------------|
| **NodeRecord v1 layout (26 B)** | `app/lib/features/connect/connect_controller.dart` · `BleNodeTableParser.parsePage()`: recordFormatVer==1, 26 bytes per record; fields nodeId, shortId, flags (isSelf, posValid, isGrey, shortIdCollision), lastSeenAgeS, latE7, lonE7, posAgeS, lastRxRssi, lastSeq. Matches `firmware/src/domain/node_table.h` · `NodeTable::kRecordBytes` and `get_snapshot_page()` layout. |
| **lastSeenAge (receiver-derived)** | App displays `lastSeenAgeS` from firmware snapshot (computed on device from lastRxAt/snapshot time). [activity_state_v0](../../nodetable/policy/activity_state_v0.md) and [nodetable_fields_inventory_v0](../../nodetable/policy/nodetable_fields_inventory_v0.md): lastSeenAge derived from lastRxAt — firmware derives and sends; app consumes. |
| **isGrey (activity-related)** | App displays `isGrey` from snapshot. [activity_state_v0](../../nodetable/policy/activity_state_v0.md) §3: Stale/Lost; [nodetable_fields_inventory_v0](../../nodetable/policy/nodetable_fields_inventory_v0.md): activityState derived. Firmware computes grey from expected_interval_s + grace; app shows the flag. |
| **rssiLast (link metric)** | App displays `lastRxRssi` as “Last RX RSSI”. [link_metrics_v0](../../nodetable/policy/link_metrics_v0.md): rssiLast from last accepted packet; [link-telemetry-minset-v0](../../nodetable/contract/link-telemetry-minset-v0.md) Table A. Single-sample display aligns with rssiLast. |
| **seq16 (freshness)** | App displays `lastSeq` (last seq from snapshot). [nodetable index](../../nodetable/index.md) §2: seq16 single per-node; [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md). Display-only; no duplicate/OOO logic in app. |
| **Position (pos_valid, lat_e7, lon_e7, pos_age_s)** | App uses `posValid`, `latE7`, `lonE7`, `posAgeS` for list (map button), details, and map screen. Aligns with Core position and [position_quality_v0](../../nodetable/policy/position_quality_v0.md) (age; no Tail-1 posFlags/sats in BLE snapshot). |
| **Self entry** | App identifies self via `isSelf` flag; `NodesController._findSelf()`, MyNode screen. [NodeTable hub](../../nodetable/index.md): self in snapshot. |
| **ShortId / collision** | App displays `shortId` and `shortIdCollision`. [nodetable_fields_inventory_v0](../../nodetable/policy/nodetable_fields_inventory_v0.md): ShortId derived; collision from policy. |

---

## 4) Gaps

| # | What | Where in code (path + symbol) | Doc / rule | Minimal remediation idea |
|---|------|-------------------------------|------------|---------------------------|
| **G1** | **Activity state (Unknown/Active/Stale/Lost)** not derived or displayed. Policy [activity_state_v0](../../nodetable/policy/activity_state_v0.md) defines parameterized T_active, T_stale, T_lost and derived state; app only shows **isGrey** (from FW) and **lastSeenAgeS** (raw). No explicit “Active”/“Stale”/“Lost” label or threshold-based derivation in app. **In V1-A this is intentional:** UX relies on firmware-derived **isGrey** as the activity-related signal; the app does not derive or display Active/Stale/Lost. | `app/lib/features/nodes/node_details_screen.dart` · “Is grey”, “Last seen age (s)”; `nodes_screen.dart` · subtitle lastSeenAgeS; no `activityState` enum or threshold logic. | [activity_state_v0](../../nodetable/policy/activity_state_v0.md) §3 (Unknown/Active/Stale/Lost), §3.2 (T_active, T_stale, T_lost). | Document as intentional V1-A: grey + raw age only; optional later: app-side derivation with policy-supplied thresholds or explicit labels. |
| **G2** | **snrLast** and **rssiRecent/snrRecent** not present. Policy [link_metrics_v0](../../nodetable/policy/link_metrics_v0.md) defines rssiLast, snrLast, rssiRecent, snrRecent. BLE snapshot and app only have **lastRxRssi** (rssiLast). Not available in snapshot ⇒ app cannot surface it. Root cause: snapshot/data availability; V1-A minimal is rssiLast only. | `app/lib/features/connect/connect_controller.dart` · `NodeRecordV1.lastRxRssi`; `node_details_screen.dart` · “Last RX RSSI”; no snr or recent fields. | [link_metrics_v0](../../nodetable/policy/link_metrics_v0.md) §2.1 (rssiLast, snrLast, rssiRecent, snrRecent). | Remediation: future candidate snapshot extension (non-goal for V1-A); document as intentional V1-A subset. |
| **G3** | **Cadence / expected_interval_s / maxSilence** not used in app. Policy [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) and activity_state refer to expected cadence and maxSilence for thresholds. App does not receive or use interval/maxSilence for display or derivation. | `app/lib/features/nodes/nodes_controller.dart` · sort uses `lastSeenAgeS` only; no interval or grace. | [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md); [activity_state_v0](../../nodetable/policy/activity_state_v0.md) §3.2 (thresholds may be derived from cadence). | App could show “expected every Xs” or use it for Stale/Lost derivation if firmware or DeviceInfo exposed it; currently grey is precomputed on device. Document as out-of-scope for current BLE snapshot. |
| **G4** | **Duplicate / out-of-order diagnostics** not shown. Policy [link_metrics_v0](../../nodetable/policy/link_metrics_v0.md) §2.2: rxDuplicatesCount, rxOutOfOrderCount optional diagnostics. Not available in snapshot ⇒ app cannot surface it. Root cause: snapshot/data availability. | N/A (no field in `NodeRecordV1` or snapshot). | [link_metrics_v0](../../nodetable/policy/link_metrics_v0.md) §2.2. | Remediation: future candidate snapshot extension (non-goal for V1-A); add only if firmware and contract extend snapshot. |
| **G5** | **Cache TTL and restore** are app-defined (10 min, SharedPreferences). No canon for mobile cache semantics or snapshot cadence; index §5 lists “Persistence mechanism, snapshot cadence” as open. | `app/lib/features/nodes/node_table_cache.dart` · `kNodeTableCacheTtlMs = 10 * 60 * 1000`, `NodeTableCacheStore.restore()`. | [NodeTable index](../../nodetable/index.md) §5 (open: persistence, snapshot cadence). | Leave as implementation detail; when persistence/snapshot policy exists, align cache TTL and restore with it. |

---

## 5) Risks / footguns

- **Snapshot record layout / versioning:** Any snapshot record layout change **must** bump **recordFormatVer**; backward compatibility rules are TBD.
- **New snapshot fields:** If firmware adds fields (e.g. snrLast, rssiRecent, activityState enum), app parser is fixed 26-byte layout and recordFormatVer==1; new format version or longer record would require app parser and DTO changes; document contract for record layout versioning.
- **Thresholds in app:** If app later derives Active/Stale/Lost from lastSeenAgeS, thresholds (T_active, T_stale, T_lost) must be sourced from policy or config, not hardcoded; activity_state_v0 leaves them parameterized.
- **Grey vs Stale/Lost:** Today “isGrey” is computed on device; app does not recompute. If app ever derives presence state locally, it should align with same interval/grace as firmware to avoid contradicting grey.

---

## 6) Summary

- **Data flow:** BLE NodeTableSnapshot → ConnectController (fetch pages) → BleNodeTableParser → NodeRecordV1 list → NodeTableRepository → NodesController → NodesState → NodesScreen / NodeDetailsScreen / MyNodeScreen / MapScreen. Cache per device (10 min TTL).
- **Matches:** 26-byte record and header match firmware; lastSeenAgeS, isGrey, lastRxRssi, lastSeq, position, self, shortId/collision align with NodeTable contract and policies (rssiLast, lastSeenAge, activity-related grey, seq16, position).
- **Gaps:** (1) No explicit Active/Stale/Lost derivation or display (only grey + raw age). (2) No snrLast or rssiRecent/snrRecent (V1-A snapshot has rssiLast only). (3) No use of cadence/expected_interval/maxSilence in app. (4) No duplicate/OOO diagnostics. (5) Cache TTL/restore implementation-defined.
- **Next:** Document G1–G5 as known V1-A subset; no normative spec or encoding change. Follow-up work (if any): optional app-side activity state derivation with policy-supplied thresholds; or extend BLE snapshot + app when firmware exposes more link metrics / activity state.
