# Issue #117 Research: My Node screen + BLE DeviceInfo

## 1) Source of truth for "is connected"

- Same as #116: `ConnectState` in `app/lib/features/connect/connect_controller.dart`
- `connectionStatus == ConnectionStatus.connected` and `connectedDeviceId != null`
- Provider: `connectControllerProvider`

## 2) Existing BLE DeviceInfo path

- **File:** `app/lib/features/connect/connect_controller.dart`
- **On connect:** `_handleConnected()` → `discoverServices()` → `_deviceInfoCharacteristic` (UUID `kNavigaDeviceInfoUuid`) → `_readDeviceInfo()` (private)
- **Read:** `_deviceInfoCharacteristic!.read()` → `BleDeviceInfoParser.parse(payload)` → `state.deviceInfo` / `state.deviceInfoWarning`
- **State:** `ConnectState.deviceInfo` (DeviceInfoData?), `deviceInfoWarning` (String?), `telemetryError` (String? on read failure)
- No public "re-read" today; My Node needs one explicit request on enter + Refresh → add `refreshDeviceInfo()` that calls `_readDeviceInfo()`.

## 3) DeviceInfoData fields (what we can show)

- **File:** same, class `DeviceInfoData`
- nodeId, shortId, deviceType, firmwareVersion, radioModuleModel
- bandId, powerMin/Max, channelMin/Max, networkMode, channelId, publicChannelId, privateChannelMin/Max, capabilities
- formatVer, bleSchemaVer, radioProtoVer, raw
- First pass: nodeId, shortId, firmwareVersion, radioModuleModel (+ deviceInfoWarning, telemetryError, "data fetched at").

## 4) My Node screen today

- **File:** `app/lib/features/my_node/my_node_screen.dart` — placeholder only (`PlaceholderScreen`).
- No controller; will use `connectControllerProvider` only (read state + call `refreshDeviceInfo()`).

## 5) Navigation to Connect

- Same as #116: `ref.read(selectedTabProvider.notifier).state = AppTab.connect` (app_state.dart, app_tabs.dart).

## 6) Implementation summary

- Add `ConnectController.refreshDeviceInfo()` (no Connect UX change).
- My Node screen: ConsumerStatefulWidget; not connected → message + "Go to Connect"; connected → post-frame one `refreshDeviceInfo()`, Refresh button, show 2–4 DeviceInfo fields + warning/error + last fetched time (screen-local).
