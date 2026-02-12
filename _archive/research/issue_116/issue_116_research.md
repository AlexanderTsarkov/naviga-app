# Issue #116 Research: Nodes screen + BLE NodeTable

## 1) Source of truth for "is connected / active device"

- **File:** `app/lib/features/connect/connect_controller.dart`
- **State:** `ConnectState`
  - `connectionStatus` (enum `ConnectionStatus`: idle, connecting, connected, disconnecting, failed)
  - `connectedDeviceId` (String? — set when BLE connection is established)
- **Provider:** `connectControllerProvider` (Riverpod)
- **Connected when:** `connectionStatus == ConnectionStatus.connected` and `connectedDeviceId != null`

## 2) Existing BLE GATT / NodeTable path

- **File:** `app/lib/features/connect/connect_controller.dart`
- **After connect:** `_handleConnected(BluetoothDevice device)`:
  - `device.discoverServices()`
  - Find service `kNavigaServiceUuid`, characteristic `kNavigaNodeTableSnapshotUuid` → `_nodeTableSnapshotCharacteristic`
- **Read flow:** `_writeNodeTableRequest(snapshotId, pageIndex)` then `_nodeTableSnapshotCharacteristic!.read()` → `BleNodeTableParser.parsePage(payload)`
- **Public API:** `fetchNodeTableSnapshot()` → `Future<List<NodeRecordV1>>` (throws if characteristic missing)

## 3) Existing NodeTable fetch used by Nodes

- **Fetcher:** `app/lib/features/nodes/node_table_fetcher.dart` — `ConnectControllerNodeTableFetcher` calls `_controller.fetchNodeTableSnapshot()`
- **Repository:** `app/lib/features/nodes/node_table_repository.dart` — `fetchLatest()` → `_fetcher.fetch()`
- **Controller:** `app/lib/features/nodes/nodes_controller.dart` — `refresh()` calls `_repository.fetchLatest()`, sorts, sets state (records, error on catch). Listens to `connectControllerProvider` for `handleConnectionChange` and sets `_connectedDeviceId`.

## 4) NodeRecordV1 (list display)

- **File:** `app/lib/features/connect/connect_controller.dart` (class `NodeRecordV1`)
- **Fields:** nodeId, shortId, isSelf, posValid, isGrey, shortIdCollision, lastSeenAgeS, latE7, lonE7, posAgeS, lastRxRssi, lastSeq
- **Display:** id (nodeId or shortId) + 1–2 fields e.g. shortId, lastSeenAgeS

## 5) Navigation to Connect

- **File:** `app/lib/app/app_shell.dart` — tabs via `IndexedStack` and `selectedTabProvider`
- **File:** `app/lib/app/app_state.dart` — `selectedTabProvider = StateProvider<AppTab>(...)`
- **Go to Connect:** `ref.read(selectedTabProvider.notifier).state = AppTab.connect`
