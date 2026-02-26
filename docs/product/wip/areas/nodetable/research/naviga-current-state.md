# Research: Naviga NodeTable — current state

> **⚠️ Historical snapshot.** This doc describes the state at the time of the S02 audit. On-air encoding has since been updated: NodeID48 (6-byte LE, [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)) and BeaconCore packed24 v0 ([#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301)). References to `pos_valid`/`pos_age_s` as on-air fields in BeaconCore are **legacy**; they are domain/BLE-snapshot fields only. Canon: [`beacon_payload_encoding_v0.md §4.1`](../../../../areas/nodetable/contract/beacon_payload_encoding_v0.md).

Filled from repo code for Issue #147. Describes where NodeTable is defined, updated, read, and how fields flow firmware → BLE → mobile.

---

## Scope

This doc covers:

- **Firmware:** Domain NodeTable (single source of truth), BLE export, and legacy app NodeTable.
- **Wire format:** NodeTableSnapshot over BLE (26-byte records, paged).
- **Mobile:** Parse and consumption of NodeRecordV1 in list, map, and detail screens.
- **Used vs dead:** Which fields are written/read in logic or UI vs never consumed.

---

## Where NodeTable lives (paths)

### Active path (M1 runtime)

| Role | Path | Notes |
|------|------|--------|
| **Definition** | `firmware/src/domain/node_table.h` | `naviga::domain::NodeTable`, `NodeEntry` struct, 26-byte record layout. |
| **Implementation** | `firmware/src/domain/node_table.cpp` | init_self, update_self_position, touch_self, upsert_remote, get_page, get_snapshot_page, eviction, grey. |
| **Wiring** | `firmware/src/app/m1_runtime.h` / `m1_runtime.cpp` | Holds `domain::NodeTable node_table_`; GNSS → update_self_position/touch_self; RX → beacon_logic_.on_rx(..., node_table_); BLE → ble_bridge_.update_all(..., node_table_, ...). |
| **Beacon → NodeTable** | `firmware/src/domain/beacon_logic.cpp` | `on_rx()` decodes GEO_BEACON and calls `table.upsert_remote(node_id, pos_valid, lat_e7, lon_e7, pos_age_s, rssi_dbm, seq, now_ms)`. |
| **BLE export** | `firmware/protocol/ble_node_table_bridge.h` / `ble_node_table_bridge.cpp` | Reads `get_node_table_request()` from transport; calls `table.create_snapshot(now_ms)` and `table.get_snapshot_page(...)`; writes 10-byte header + records to `set_node_table_response()`. |
| **BLE transport** | `firmware/src/platform/ble_transport_core.cpp`, `ble_esp32_transport.cpp` | Single characteristic `6e4f0003-...` (NodeTableSnapshot); request (snapshot_id, page_index) written by client; response = page payload. |
| **Mobile fetch** | `app/lib/features/connect/connect_controller.dart` | `_writeNodeTableRequest(snapshotId, pageIndex)` then read characteristic → `BleNodeTableParser.parsePage(payload)` → `NodeRecordV1` list. |
| **Mobile model** | `app/lib/features/connect/connect_controller.dart` | `NodeRecordV1` (nodeId, shortId, isSelf, posValid, isGrey, shortIdCollision, lastSeenAgeS, latE7, lonE7, posAgeS, lastRxRssi, lastSeq). |
| **Mobile state** | `app/lib/features/nodes/node_table_fetcher.dart`, `node_table_cache.dart`, `nodes_controller.dart`, `nodes_state.dart` | Fetcher uses ConnectController.fetchNodeTableSnapshot(); cache/repository hold `List<NodeRecordV1>`; controller sorts by lastSeenAgeS, finds self. |

### Legacy (compiled but not wired in M1)

| Role | Path | Notes |
|------|------|--------|
| **Legacy table** | `firmware/src/app/node_table.h` / `node_table.cpp` | `naviga::NodeTable` (no domain namespace), 23-byte record, self-only. **Not used by M1Runtime.** |
| **Legacy BLE** | `firmware/src/services/ble_service.cpp` | `BleService::init(..., const NodeTable* node_table, ...)` uses `app::NodeTable` and multiple page UUIDs (6e4f0003–6e4f0006). Still compiled into build but **BleService::init is not called** from current AppServices; M1 uses BleEsp32Transport + BleNodeTableBridge instead. |

---

## Current fields/statuses

### Domain NodeEntry (firmware)

Defined in `firmware/src/domain/node_table.h`:

| Field | Type | Meaning |
|-------|------|---------|
| `node_id` | uint64_t | Unique node identifier (primary key). |
| `short_id` | uint16_t | CRC16-CCITT(node_id) for display; not a protocol key. |
| `is_self` | bool | True for the local device’s entry. |
| `pos_valid` | bool | Whether position (lat_e7, lon_e7) is valid. |
| `short_id_collision` | bool | True if another entry shares the same short_id. |
| `lat_e7` | int32_t | Latitude in 1e-7 degrees. |
| `lon_e7` | int32_t | Longitude in 1e-7 degrees. |
| `pos_age_s` | uint16_t | Age of position in seconds (from beacon or GNSS). |
| `last_rx_rssi` | int8_t | RSSI in dBm of last received packet from this node (remote only). |
| `last_seq` | uint16_t | Last beacon sequence number seen. |
| `last_seen_ms` | uint32_t | Uptime ms of last observation (TX for self, RX for remote). |
| `in_use` | bool | Slot occupied (internal; not sent over BLE). |

**Derived on export (not stored in NodeEntry):** `is_grey` = (age_s > expected_interval_s + grace_s). Computed in `get_page` / `get_snapshot_page` from `last_seen_ms` and `expected_interval_s_` / `grace_s_`; sent as flag bit 0x04.

### Wire format (BLE NodeTableSnapshot)

- **Page header (10 bytes):** snapshot_id (u16), total_nodes (u16), page_index (u16), page_size (u8), page_count (u16), record_format_ver (u8).
- **Record (26 bytes), little-endian:** node_id (u64), short_id (u16), flags (u8), last_seen_age_s (u16), lat_e7 (u32), lon_e7 (u32), pos_age_s (u16), last_rx_rssi (u8), last_seq (u16).
- **Flags:** bit0 = is_self, bit1 = pos_valid, bit2 = is_grey, bit3 = short_id_collision.

Source: `firmware/src/domain/node_table.cpp` (`get_page` / `get_snapshot_page`) and `app/lib/features/connect/connect_controller.dart` (`BleNodeTableParser.parsePage`).

### Mobile NodeRecordV1

Same semantics as wire; field names: nodeId, shortId, isSelf, posValid, isGrey, shortIdCollision, lastSeenAgeS, latE7, lonE7, posAgeS, lastRxRssi, lastSeq.

---

## Where each field is written and read

| Field | Written (firmware) | Read (firmware) | Read (mobile UI/logic) |
|-------|--------------------|-----------------|-------------------------|
| **node_id** | node_table.cpp init_self, upsert_remote | get_snapshot_page (serialize); find_entry_index | All screens: identity, list, map, details, sort; filter self. |
| **short_id** | node_table.cpp init_self, upsert_remote (compute_short_id) | get_snapshot_page (serialize) | List/detail labels; My Node display. |
| **is_self** | node_table.cpp init_self, upsert_remote (false for remote) | get_snapshot_page → flags bit0 | nodes_controller _findSelf; map self marker; My Node screen; list/details. |
| **pos_valid** | update_self_position (true); touch_self path leaves as-is; upsert_remote (from beacon) | get_snapshot_page → flags bit1 | Map: _nodesWithValidPos; list trailing; details “Position valid”; My Node. |
| **short_id_collision** | recompute_collisions() after init/upsert/evict | get_snapshot_page → flags bit3 | connect_controller (filter in fetch); My Node + node_details_screen display. |
| **lat_e7 / lon_e7** | update_self_position; upsert_remote (from beacon) | get_snapshot_page (when pos_valid) | Map markers; fitBounds; details; “has valid pos” checks. |
| **pos_age_s** | update_self_position; upsert_remote (from beacon) | get_snapshot_page | My Node, node_details_screen (display). |
| **last_rx_rssi** | upsert_remote (rssi_dbm from radio) | get_snapshot_page | My Node, node_details_screen (display). |
| **last_seq** | upsert_remote (from beacon fields.seq) | get_snapshot_page | My Node, node_details_screen (display). |
| **last_seen_ms** | init_self, update_self_position, touch_self, upsert_remote | is_grey(), get_snapshot_page (converted to last_seen_age_s) | Not sent as raw ms; mobile gets lastSeenAgeS. |
| **last_seen_age_s** (derived) | — | Computed in get_snapshot_page (now_ms - last_seen_ms)/1000 | List subtitle; sort (nodes_controller); details; map _nodesForFit (active ≤ 180s). |
| **is_grey** (derived) | — | is_grey() in get_snapshot_page → flags bit2 | My Node, node_details_screen (display). |
| **in_use** | init_self, upsert_remote, evict_oldest_grey | find_entry_index, find_free_index, build_ordered_indices, eviction | Not exported; internal only. |

---

## Used vs dead fields

**Used (evidence):**

- **node_id** — UI: nodes_screen.dart (list, tap to details), node_details_screen (nodeId), map_screen (_findNode, self marker), my_node_screen (selfRecord.nodeId). Logic: sort, self detection.
- **short_id** — nodes_screen list, node_details_screen, my_node_screen labels.
- **is_self** — nodes_controller._findSelf, map self marker, My Node screen.
- **pos_valid** — map_screen _nodesWithValidPos, nodes_screen trailing, node_details_screen and my_node_screen “Position valid”.
- **short_id_collision** — connect_controller fetch filter (record.shortIdCollision); my_node_screen, node_details_screen display.
- **lat_e7 / lon_e7** — map_screen markers, fitBounds, detail screens.
- **pos_age_s** — My Node and node_details_screen “Position age” / “posAgeS”.
- **last_rx_rssi** — My Node and node_details_screen “RSSI”.
- **last_seq** — node_details_screen “Last seq”; My Node display.
- **last_seen_age_s** — nodes_screen “lastSeen: Xs”; nodes_controller sort by lastSeenAgeS; map_screen _nodesForFit (active ≤ 180s); My Node and node_details_screen.
- **is_grey** — my_node_screen, node_details_screen “Is grey”.

**Dead (no evidence of read in logic or UI):**

- **in_use** — Internal slot flag only; not in BLE or mobile.

All other fields listed above are both written in firmware and read in mobile UI or logic.

---

## Call sites (logic/UX)

### Firmware — writes

- **m1_runtime.cpp:** `node_table_.set_expected_interval_s(10)`, `init_self(self_id, now_ms)`, `update_self_position(...)` / `touch_self(now_ms)` from set_self_position, `beacon_logic_.on_rx(..., node_table_)` in handle_rx.
- **beacon_logic.cpp:** `on_rx()` → `table.upsert_remote(node_id, pos_valid, lat_e7, lon_e7, pos_age_s, rssi_dbm, seq, now_ms)`.
- **node_table.cpp:** init_self, update_self_position, touch_self, upsert_remote, evict_oldest_grey, recompute_collisions.

### Firmware — reads

- **m1_runtime.cpp:** `node_table_.size()` for node_count().
- **ble_node_table_bridge.cpp:** `table.create_snapshot(now_ms)`, `table.size()`, `table.get_snapshot_page(snapshot_id, page_index, page_size, buffer)`.

### Mobile — read path

- **connect_controller.dart:** `fetchNodeTableSnapshot()` → _writeNodeTableRequest + read characteristic → `BleNodeTableParser.parsePage()` → `List<NodeRecordV1>`.
- **node_table_fetcher.dart:** Calls controller.fetchNodeTableSnapshot().
- **nodes_controller.dart:** Fetcher result → state (records, recordsSorted, self); sort by lastSeenAgeS; _findSelf by isSelf.
- **nodes_screen.dart:** List of nodes (shortId, lastSeenAgeS, posValid, tap nodeId → details).
- **node_details_screen.dart:** By nodeId from state; shows nodeId, shortId, posValid, shortIdCollision, isGrey, lastSeenAgeS, posAgeS, lastRxRssi, lastSeq, lat/lon.
- **my_node_screen.dart:** Self record; nodeId, shortId, posValid, shortIdCollision, isGrey, lastSeenAgeS, posAgeS, lastRxRssi, lastSeq.
- **map_screen.dart:** _nodesWithValidPos (posValid), _nodesForFit (lastSeenAgeS ≤ 180), markers by nodeId, self marker (nodeId == selfNodeId).

---

## Update flow and data sources (short summary)

1. **Self entry**
   - **Source:** GNSS (or stub) in AppServices → M1Runtime.set_self_position(pos_valid, lat_e7, lon_e7, pos_age_s, ...) → node_table_.update_self_position() or node_table_.touch_self().
   - **Beacon TX:** BeaconLogic.build_tx reads self_fields_ (fed from set_self_position); after send, touch_self(now_ms) is not called in current code (last_seen_ms for self is updated only in set_self_position / update_self_position).

2. **Remote entries**
   - **Source:** Radio RX → M1Runtime.handle_rx() → radio_->recv() → BeaconLogic.on_rx(payload, rssi_dbm, node_table_) → decode GEO_BEACON → node_table_.upsert_remote(node_id, pos_valid, lat_e7, lon_e7, pos_age_s, rssi_dbm, seq, now_ms).

3. **BLE → mobile**
   - App requests page: write (snapshot_id, page_index) to NodeTableSnapshot characteristic; firmware on read: BleNodeTableBridge.update_node_table() → create_snapshot (if snapshot_id 0) → get_snapshot_page() → set_node_table_response(header + 26-byte records). Mobile reads characteristic, parsePage() → NodeRecordV1 list.

4. **Eviction / grey**
   - expected_interval_s (e.g. 10) + grace_s set on NodeTable; is_grey(entry, now_ms) = (age_s > expected_interval_s + grace_s). Eviction: when full, evict_oldest_grey() frees slot for new remote or for self. Grey entries still exported; mobile can show “is_grey” in UI.

---

## Notes / gaps

- **Self last_seen_ms on TX:** Currently touch_self(now_ms) is not called after a successful beacon TX; self last_seen is only updated in set_self_position. So “last seen” for self reflects last GNSS/position update, not last TX. Confirm if intentional.
- **Legacy app::NodeTable and BleService:** Still compiled, not used by M1; can be removed or guarded when cleaning legacy BLE.
- **connect_controller filter:** Records with shortIdCollision are filtered out in _fetchNodeTableSnapshot (`.where((record) => !record.shortIdCollision)`); collision is still displayed for records that are shown (e.g. self), so the filter applies to “which remote nodes to include” (e.g. dedup by short_id). Intent and edge cases could be clarified in product.
- **Map “active” threshold:** _kActiveMaxAgeS = 180s for fitBounds; other UX (list, details) does not hard-code this.
- **Record format version:** BLE page header has record_format_ver = 1; mobile parser rejects ≠ 1. Single version in use.
