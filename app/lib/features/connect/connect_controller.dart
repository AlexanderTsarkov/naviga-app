import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:android_intent_plus/android_intent.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/legacy.dart';
import 'package:geolocator/geolocator.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../shared/logging.dart';
import '../nodes/node_table_debug_hook.dart';
import 'status_parser.dart';

const String kNavigaServiceUuid = '6e4f0001-1b9a-4c3a-9a3b-000000000001';
const String kNavigaDeviceInfoUuid = '6e4f0002-1b9a-4c3a-9a3b-000000000001';
const String kNavigaNodeTableSnapshotUuid =
    '6e4f0003-1b9a-4c3a-9a3b-000000000001';
const String kNavigaStatusUuid = '6e4f0007-1b9a-4c3a-9a3b-000000000001';

/// S04 #466: Self node_name — read/write; first-phase encoding: 1-byte length + UTF-8 (max 32 bytes).
const String kNavigaNodeNameUuid = '6e4f0008-1b9a-4c3a-9a3b-000000000001';

/// S04 #464: Targeted read — write 8 bytes node_id (little-endian), read one 72-byte canon record.
const String kNavigaTargetedReadUuid = '6e4f000c-1b9a-4c3a-9a3b-000000000001';

/// S04 #465: NodeTable subscription — notify with batched full records (1 byte count + N×72 bytes).
const String kNavigaNodeTableSubscribeUuid =
    '6e4f0009-1b9a-4c3a-9a3b-000000000001';
const String kNavigaNamePrefix = 'Naviga';

/// S04 Slice 1: Naviga manufacturer ID for BLE contract version in advertising.
const int kNavigaManufacturerId = 0x6E4F;

/// S04 Slice 1: Expected BLE contract version (exact match required for normal connect).
const int kBLEContractVersionMajor = 1;
const int kBLEContractVersionMinor = 0;
const bool kDebugFetchNodeTableOnConnect = false;

final connectControllerProvider =
    StateNotifierProvider<ConnectController, ConnectState>((ref) {
      return ConnectController();
    });

class ConnectController extends StateNotifier<ConnectState> {
  ConnectController() : super(ConnectState.initial()) {
    _init();
  }

  StreamSubscription<BluetoothAdapterState>? _adapterSub;
  StreamSubscription<List<ScanResult>>? _scanSub;
  StreamSubscription<bool>? _isScanningSub;
  StreamSubscription<BluetoothConnectionState>? _connectionSub;
  Timer? _scanRestartTimer;
  SharedPreferences? _prefs;
  BluetoothDevice? _activeDevice;
  BluetoothCharacteristic? _deviceInfoCharacteristic;
  BluetoothCharacteristic? _nodeTableSnapshotCharacteristic;
  BluetoothCharacteristic? _statusCharacteristic;
  BluetoothCharacteristic? _nodeNameCharacteristic;
  BluetoothCharacteristic? _nodeTableSubscribeCharacteristic;
  StreamSubscription<List<int>>? _nodeTableSubscribeSubscription;
  int? _lastNodeTableSnapshotId;

  /// S04 #465: Called when a batched subscription update arrives; set by NodesController.
  void Function(List<NodeRecordV1> records)? _subscriptionUpdateCallback;

  static const _prefsLastDeviceKey = 'naviga.last_device_id';

  /// S04 #465: Register callback to apply incoming subscription updates (upserts). Call from NodesController.
  void setSubscriptionUpdateCallback(
    void Function(List<NodeRecordV1> records)? callback,
  ) {
    _subscriptionUpdateCallback = callback;
  }

  Future<void> _init() async {
    _prefs = await SharedPreferences.getInstance();
    final lastDeviceId = _prefs?.getString(_prefsLastDeviceKey);
    if (lastDeviceId != null) {
      state = state.copyWith(lastConnectedDeviceId: lastDeviceId);
    }

    final supported = await _isBleSupported();
    if (supported) {
      _adapterSub = FlutterBluePlus.adapterState.listen((state) {
        this.state = this.state.copyWith(adapterState: state);
      });

      _isScanningSub = FlutterBluePlus.isScanning.listen((isScanning) {
        state = state.copyWith(isScanning: isScanning);
        if (!isScanning && state.scanRequested) {
          _scheduleScanRestart();
        }
      });

      _scanSub = FlutterBluePlus.scanResults.listen(_handleScanResults);
    } else {
      state = state.copyWith(adapterState: BluetoothAdapterState.unavailable);
    }

    await refreshReadiness();
  }

  Future<bool> _isBleSupported() async {
    try {
      return await FlutterBluePlus.isSupported;
    } catch (_) {
      return false;
    }
  }

  Future<void> refreshReadiness() async {
    final locationEnabled = await Geolocator.isLocationServiceEnabled();
    final locationPermission = await Permission.location.status;
    state = state.copyWith(
      locationServiceEnabled: locationEnabled,
      locationPermission: locationPermission,
    );
  }

  Future<void> requestLocationPermission() async {
    final result = await Permission.location.request();
    state = state.copyWith(locationPermission: result);
  }

  Future<void> openAppSettingsPage() {
    return openAppSettings();
  }

  void openBluetoothSettings() {
    if (!Platform.isAndroid) {
      return;
    }
    const intent = AndroidIntent(action: 'android.settings.BLUETOOTH_SETTINGS');
    intent.launch();
  }

  Future<void> openLocationSettings() {
    return Geolocator.openLocationSettings();
  }

  bool get isReadyToScan {
    return state.adapterState == BluetoothAdapterState.on &&
        state.locationServiceEnabled &&
        state.locationPermission.isGranted;
  }

  Future<void> startScan() async {
    if (!isReadyToScan) {
      return;
    }

    state = state.copyWith(lastError: null, scanRequested: true);

    await _startScanWindow();
  }

  Future<void> stopScan() async {
    state = state.copyWith(scanRequested: false);
    _scanRestartTimer?.cancel();
    await FlutterBluePlus.stopScan();
  }

  Future<void> _startScanWindow() async {
    if (!state.scanRequested) {
      return;
    }

    try {
      await FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 12),
        androidUsesFineLocation: true,
        continuousUpdates: true,
        continuousDivisor: 1,
      );
    } catch (error) {
      state = state.copyWith(lastError: error.toString());
    }
  }

  void _scheduleScanRestart() {
    _scanRestartTimer?.cancel();
    _scanRestartTimer = Timer(const Duration(seconds: 1), () {
      _startScanWindow();
    });
  }

  Future<void> connectToDevice(NavigaScanDevice device) async {
    if (state.connectionStatus == ConnectionStatus.connecting ||
        state.connectionStatus == ConnectionStatus.disconnecting) {
      return;
    }

    if (!_isBleContractVersionCompatible(device)) {
      state = state.copyWith(
        lastError:
            'Incompatible BLE version (update app or firmware). '
            'Expected $kBLEContractVersionMajor.$kBLEContractVersionMinor.',
      );
      return;
    }

    await stopScan();

    final bluetoothDevice = BluetoothDevice.fromId(device.id);
    _activeDevice = bluetoothDevice;

    _connectionSub?.cancel();
    _connectionSub = bluetoothDevice.connectionState.listen((event) async {
      if (event == BluetoothConnectionState.connected) {
        state = state.copyWith(
          connectionStatus: ConnectionStatus.connected,
          connectedDeviceId: device.id,
        );
        await _handleConnected(bluetoothDevice);
      }
      if (event == BluetoothConnectionState.disconnected) {
        await _teardownGatt(clearState: true);
        state = state.copyWith(
          connectionStatus: ConnectionStatus.idle,
          connectedDeviceId: null,
        );
      }
    });

    state = state.copyWith(
      connectionStatus: ConnectionStatus.connecting,
      connectedDeviceId: device.id,
      lastError: null,
    );

    try {
      await bluetoothDevice.connect(
        license: License.free,
        timeout: const Duration(seconds: 12),
        mtu: null,
        autoConnect: false,
      );
      await _prefs?.setString(_prefsLastDeviceKey, device.id);
      state = state.copyWith(lastConnectedDeviceId: device.id);
    } catch (error) {
      state = state.copyWith(
        connectionStatus: ConnectionStatus.failed,
        lastError: error.toString(),
      );
    }
  }

  Future<void> disconnect() async {
    if (_activeDevice == null) {
      return;
    }

    state = state.copyWith(connectionStatus: ConnectionStatus.disconnecting);
    try {
      await _teardownGatt(clearState: true);
      await _activeDevice!.disconnect();
    } catch (error) {
      state = state.copyWith(
        connectionStatus: ConnectionStatus.failed,
        lastError: error.toString(),
      );
    }
  }

  Future<void> _handleConnected(BluetoothDevice device) async {
    logInfo('DBG kDebugFetchNodeTableOnConnect=$kDebugFetchNodeTableOnConnect');
    state = state.copyWith(
      isDiscoveringServices: true,
      telemetryError: null,
      deviceInfo: null,
      deviceInfoWarning: null,
      statusData: null,
      statusWarning: null,
      statusError: null,
      clearStatusWarning: true,
      clearStatusError: true,
    );

    try {
      final services = await device.discoverServices();
      if (kDebugMode) {
        _logServices(services);
      }

      final navigaService = services.cast<BluetoothService?>().firstWhere(
        (service) => service?.uuid == Guid(kNavigaServiceUuid),
        orElse: () => null,
      );
      if (navigaService == null) {
        state = state.copyWith(
          isDiscoveringServices: false,
          telemetryError: 'Telemetry not available (firmware too old?)',
        );
        return;
      }

      _deviceInfoCharacteristic = _findCharacteristic(
        navigaService,
        Guid(kNavigaDeviceInfoUuid),
      );
      _nodeTableSnapshotCharacteristic = _findCharacteristic(
        navigaService,
        Guid(kNavigaNodeTableSnapshotUuid),
      );
      _statusCharacteristic = _findCharacteristic(
        navigaService,
        Guid(kNavigaStatusUuid),
      );
      _nodeTableSubscribeCharacteristic = _findCharacteristic(
        navigaService,
        Guid(kNavigaNodeTableSubscribeUuid),
      );
      _nodeNameCharacteristic = _findCharacteristic(
        navigaService,
        Guid(kNavigaNodeNameUuid),
      );

      if (_deviceInfoCharacteristic == null) {
        state = state.copyWith(
          telemetryError: 'Telemetry not available (firmware too old?)',
        );
        return;
      }
      if (!_deviceInfoCharacteristic!.properties.read) {
        state = state.copyWith(telemetryError: 'DeviceInfo not readable');
        return;
      }

      await _readDeviceInfo();
      await _readStatus();
      logInfo(
        'DBG nodeTableDebugRefreshOnConnect=${nodeTableDebugRefreshOnConnect != null}',
      );
      if (nodeTableDebugRefreshOnConnect != null) {
        try {
          await nodeTableDebugRefreshOnConnect!();
          // S04 #465: Subscribe only after baseline load completes (canon: baseline first, then subscribe).
          await _startNodeTableSubscription();
        } catch (error) {
          logInfo('NodeTable: baseline refresh failed: $error');
          // Non-fatal: connection stays valid; nodes list may be empty until retry.
        }
      }
    } catch (error) {
      state = state.copyWith(telemetryError: error.toString());
    } finally {
      state = state.copyWith(isDiscoveringServices: false);
    }
  }

  BluetoothCharacteristic? _findCharacteristic(
    BluetoothService service,
    Guid uuid,
  ) {
    for (final characteristic in service.characteristics) {
      if (characteristic.uuid == uuid) {
        return characteristic;
      }
    }
    return null;
  }

  Future<void> _readDeviceInfo() async {
    try {
      final payload = await _deviceInfoCharacteristic!.read();
      final result = BleDeviceInfoParser.parse(payload);
      state = state.copyWith(
        deviceInfo: result.data,
        deviceInfoWarning: result.warning,
        telemetryError: null,
      );
    } catch (error) {
      state = state.copyWith(telemetryError: 'DeviceInfo read failed: $error');
    }
  }

  /// Re-reads DeviceInfo from the connected device (e.g. for My Node screen).
  /// No-op if not connected or characteristic missing.
  Future<void> refreshDeviceInfo() async {
    if (_deviceInfoCharacteristic == null) {
      return;
    }
    await _readDeviceInfo();
  }

  /// Re-reads Status/Health (0007) from the connected device.
  /// Missing characteristic is reported as a graceful status error.
  Future<void> refreshStatus() async {
    await _readStatus();
  }

  Future<void> _readStatus() async {
    if (_statusCharacteristic == null) {
      state = state.copyWith(
        statusData: null,
        statusWarning: null,
        statusError: 'Status not available (firmware too old?)',
        clearStatusWarning: true,
      );
      return;
    }
    if (!_statusCharacteristic!.properties.read) {
      state = state.copyWith(
        statusData: null,
        statusWarning: null,
        statusError: 'Status characteristic is not readable',
        clearStatusWarning: true,
      );
      return;
    }

    try {
      final payload = await _statusCharacteristic!.read();
      final result = BleStatusParser.parse(payload);
      state = state.copyWith(
        statusData: result.data,
        statusWarning: result.warning,
        statusError: null,
        clearStatusError: true,
        clearStatusWarning: result.warning == null,
      );
    } catch (error) {
      state = state.copyWith(
        statusData: null,
        statusWarning: null,
        statusError: 'Status read failed: $error',
        clearStatusWarning: true,
      );
    }
  }

  Future<void> _writeNodeTableRequest(int snapshotId, int pageIndex) async {
    if (_nodeTableSnapshotCharacteristic == null) {
      throw StateError('NodeTableSnapshot characteristic missing');
    }
    final payload = <int>[
      snapshotId & 0xFF,
      (snapshotId >> 8) & 0xFF,
      pageIndex & 0xFF,
      (pageIndex >> 8) & 0xFF,
    ];
    await _nodeTableSnapshotCharacteristic!.write(
      payload,
      withoutResponse: false,
    );
  }

  Future<NodeTableSnapshotPage> _readNodeTablePage(
    int snapshotId,
    int pageIndex,
  ) async {
    if (_nodeTableSnapshotCharacteristic == null) {
      throw StateError('NodeTableSnapshot characteristic missing');
    }
    await _writeNodeTableRequest(snapshotId, pageIndex);
    final payload = await _nodeTableSnapshotCharacteristic!.read();
    final result = BleNodeTableParser.parsePage(payload);
    if (result.data == null) {
      throw StateError(result.warning ?? 'NodeTableSnapshot parse failed');
    }
    return result.data!;
  }

  Future<List<NodeRecordV1>> _fetchNodeTableSnapshot() async {
    var retried = false;
    while (true) {
      final page0 = await _readNodeTablePage(0, 0);
      final header = page0.header;
      _lastNodeTableSnapshotId = header.snapshotId;
      var pageCount = header.pageCount;
      if (pageCount > 50) {
        logInfo('NodeTable: page_count capped from $pageCount to 50');
        pageCount = 50;
      }

      final records = <NodeRecordV1>[...page0.records];
      var mismatch = false;
      for (var i = 1; i < pageCount; i++) {
        final page = await _readNodeTablePage(header.snapshotId, i);
        if (page.header.snapshotId != header.snapshotId ||
            page.header.pageIndex != i) {
          mismatch = true;
          break;
        }
        records.addAll(page.records);
      }

      if (!mismatch) {
        final collisions = records
            .where((record) => record.shortIdCollision)
            .length;
        logInfo(
          'NodeTable: snapshot=${header.snapshotId} totalNodes=${header.totalNodes} '
          'pages=$pageCount records=${records.length} collisions=$collisions',
        );
        return records;
      }

      if (retried) {
        throw StateError('NodeTable snapshot changed during read');
      }
      logInfo('NodeTable: snapshot changed, retrying from page 0');
      retried = true;
    }
  }

  Future<void> _teardownGatt({required bool clearState}) async {
    await _nodeTableSubscribeSubscription?.cancel();
    _nodeTableSubscribeSubscription = null;
    _nodeTableSubscribeCharacteristic = null;
    _nodeNameCharacteristic = null;
    _deviceInfoCharacteristic = null;
    _nodeTableSnapshotCharacteristic = null;
    _statusCharacteristic = null;
    _lastNodeTableSnapshotId = null;
    if (clearState) {
      state = state.copyWith(
        deviceInfo: null,
        deviceInfoWarning: null,
        statusData: null,
        statusWarning: null,
        statusError: null,
        clearStatusWarning: true,
        clearStatusError: true,
        telemetryError: null,
        isDiscoveringServices: false,
      );
    }
  }

  int? get lastNodeTableSnapshotId => _lastNodeTableSnapshotId;

  /// S04 #466: Short display label — max 12 characters, max 24 UTF-8 bytes.
  static const int kNodeNameMaxChars = 12;
  static const int kNodeNameMaxBytes = 24;

  /// S04 #466: Read current self node_name over BLE. Encoding: 1-byte length + UTF-8. Returns empty string if not connected or characteristic missing.
  Future<String> readNodeName() async {
    final char = _nodeNameCharacteristic;
    if (char == null || !char.properties.read) {
      return '';
    }
    try {
      final payload = await char.read();
      if (payload.isEmpty) return '';
      final length = payload[0].clamp(0, kNodeNameMaxBytes);
      if (length == 0 || payload.length < 1 + length) return '';
      final bytes = payload.sublist(1, 1 + length);
      return utf8.decode(bytes);
    } catch (_) {
      return '';
    }
  }

  /// S04 #466: Write self node_name over BLE. [name] must be validated and truncated (max 12 chars, 24 UTF-8 bytes, allowlist). Use [validateAndTruncateNodeName] first.
  Future<void> writeNodeName(String name) async {
    final char = _nodeNameCharacteristic;
    if (char == null || !char.properties.write) {
      throw StateError('Node name characteristic not available for write');
    }
    final bytes = utf8.encode(name);
    final len = bytes.length.clamp(0, kNodeNameMaxBytes);
    final payload = <int>[len, ...bytes.sublist(0, len)];
    await char.write(payload, withoutResponse: false);
  }

  /// S04 #466: Truncate to max 12 code points and max 24 UTF-8 bytes without splitting multibyte characters. Returns trimmed string.
  static String truncateNodeNameToLimit(String name) {
    final runes = name.runes.toList();
    if (runes.isEmpty) return '';
    final take = runes.length > kNodeNameMaxChars
        ? kNodeNameMaxChars
        : runes.length;
    String s = String.fromCharCodes(runes.take(take));
    List<int> bytes = utf8.encode(s);
    if (bytes.length <= kNodeNameMaxBytes) return s;
    // Truncate at UTF-8 code point boundary: drop trailing bytes that are continuation bytes (0x80–0xBF).
    int end = kNodeNameMaxBytes;
    while (end > 0 && (bytes[end - 1] & 0xC0) == 0x80) {
      end--;
    }
    if (end == 0) return '';
    return utf8.decode(bytes.sublist(0, end));
  }

  /// S04 #466: Allowed: Latin, Cyrillic, digits 0–9, symbols - _ # = @ +. Reject control, emoji, other.
  static bool isNodeNameAllowed(String name) {
    for (final rune in name.runes) {
      if (rune < 0x20) return false; // control
      if (rune <= 0x7F) {
        // ASCII: 0-9, A-Z, a-z, - _ # = @ +
        if ((rune >= 0x30 && rune <= 0x39) ||
            (rune >= 0x41 && rune <= 0x5A) ||
            (rune >= 0x61 && rune <= 0x7A) ||
            rune == 0x2D ||
            rune == 0x5F ||
            rune == 0x23 ||
            rune == 0x3D ||
            rune == 0x40 ||
            rune == 0x2B) {
          continue;
        }
        return false;
      }
      // Cyrillic block U+0400..U+04FF
      if (rune >= 0x0400 && rune <= 0x04FF) continue;
      return false; // disallow emoji, other scripts, etc.
    }
    return true;
  }

  /// S04 #466: Validate and truncate for write. Returns null if invalid (disallowed chars); otherwise trimmed/truncated string.
  static String? validateAndTruncateNodeName(String name) {
    final trimmed = name.trim();
    if (trimmed.isEmpty) return '';
    final truncated = truncateNodeNameToLimit(trimmed);
    if (!isNodeNameAllowed(truncated)) return null;
    return truncated;
  }

  /// S04 #465: Enable notify on NodeTable subscribe characteristic; only call after baseline load.
  Future<void> _startNodeTableSubscription() async {
    final char = _nodeTableSubscribeCharacteristic;
    if (char == null || !char.properties.notify) {
      return;
    }
    try {
      await char.setNotifyValue(true);
      _nodeTableSubscribeSubscription?.cancel();
      _nodeTableSubscribeSubscription = char.lastValueStream.listen((
        List<int> value,
      ) {
        final records = BleNodeTableParser.parseSubscriptionBatch(value);
        if (records.isNotEmpty && _subscriptionUpdateCallback != null) {
          _subscriptionUpdateCallback!(records);
        }
      });
    } catch (e) {
      logInfo('NodeTable: subscription enable failed: $e');
    }
  }

  Future<List<NodeRecordV1>> fetchNodeTableSnapshot() async {
    if (_nodeTableSnapshotCharacteristic == null) {
      throw StateError('NodeTableSnapshot characteristic missing');
    }
    return _fetchNodeTableSnapshot();
  }

  void _logServices(List<BluetoothService> services) {
    for (final service in services) {
      logInfo('Service ${service.uuid}');
      for (final characteristic in service.characteristics) {
        final props = <String>[];
        if (characteristic.properties.read) props.add('read');
        if (characteristic.properties.notify) props.add('notify');
        if (characteristic.properties.indicate) props.add('indicate');
        if (characteristic.properties.write) props.add('write');
        if (characteristic.properties.writeWithoutResponse) {
          props.add('writeNoRsp');
        }
        logInfo('  Char ${characteristic.uuid} props=${props.join(',')}');
      }
    }
  }

  void _handleScanResults(List<ScanResult> results) {
    final Map<String, NavigaScanDevice> updated = Map.from(state.devices);

    for (final result in results) {
      if (!_isNavigaDevice(result)) {
        continue;
      }

      _logAdvertisement(result);

      final id = result.device.remoteId.toString();
      final name = _deviceName(result);
      final version = _parseBleContractVersion(result);
      updated[id] = NavigaScanDevice(
        id: id,
        name: name,
        rssi: result.rssi,
        lastSeen: DateTime.now(),
        bleContractVersionMajor: version?.$1,
        bleContractVersionMinor: version?.$2,
      );
    }

    if (updated.isNotEmpty) {
      state = state.copyWith(devices: updated);
    }
  }

  /// Parses BLE contract version from manufacturer data (S04 Slice 1).
  /// On standard BLE scan APIs, manufacturerData[key] is the payload *after* the
  /// 2-byte company ID, so the value is [major, minor] (2 bytes). Returns (major, minor)
  /// if present; else null.
  (int, int)? _parseBleContractVersion(ScanResult result) {
    final data =
        result.advertisementData.manufacturerData[kNavigaManufacturerId];
    return ConnectController.parseBleContractVersionFromManufacturerPayload(
      data,
    );
  }

  /// Parses BLE contract version from manufacturer payload bytes. Exposed for tests.
  /// Payload shape: [major, minor] (2 bytes); company ID is not included in the value.
  static (int, int)? parseBleContractVersionFromManufacturerPayload(
    List<int>? data,
  ) {
    if (data == null || data.length < 2) {
      return null;
    }
    return (data[0] & 0xFF, data[1] & 0xFF);
  }

  bool _isBleContractVersionCompatible(NavigaScanDevice device) {
    final major = device.bleContractVersionMajor;
    final minor = device.bleContractVersionMinor;
    if (major == null || minor == null) {
      return false;
    }
    return major == kBLEContractVersionMajor &&
        minor == kBLEContractVersionMinor;
  }

  bool _isNavigaDevice(ScanResult result) {
    final adv = result.advertisementData;
    final navigaGuid = Guid(kNavigaServiceUuid);

    if (adv.serviceUuids.contains(navigaGuid)) {
      return true;
    }

    if (adv.manufacturerData.containsKey(kNavigaManufacturerId)) {
      return true;
    }

    final name = _deviceName(result);
    return name.toLowerCase().startsWith(kNavigaNamePrefix.toLowerCase());
  }

  String _deviceName(ScanResult result) {
    final advName = result.advertisementData.advName.trim();
    if (advName.isNotEmpty) {
      return advName;
    }
    final platformName = result.device.platformName.trim();
    if (platformName.isNotEmpty) {
      return platformName;
    }
    return 'Unknown';
  }

  void _logAdvertisement(ScanResult result) {
    logInfo(
      'Adv ${result.device.remoteId}: '
      'name=${_deviceName(result)}, '
      'rssi=${result.rssi}, '
      'serviceUuids=${result.advertisementData.serviceUuids}, '
      'manufacturerData=${result.advertisementData.manufacturerData}',
    );
  }

  @override
  void dispose() {
    _adapterSub?.cancel();
    _scanSub?.cancel();
    _isScanningSub?.cancel();
    _connectionSub?.cancel();
    _scanRestartTimer?.cancel();
    super.dispose();
  }
}

enum ConnectionStatus { idle, connecting, connected, disconnecting, failed }

class ConnectState {
  const ConnectState({
    required this.adapterState,
    required this.locationServiceEnabled,
    required this.locationPermission,
    required this.isScanning,
    required this.scanRequested,
    required this.devices,
    required this.connectionStatus,
    required this.connectedDeviceId,
    required this.lastConnectedDeviceId,
    required this.isDiscoveringServices,
    required this.deviceInfo,
    required this.deviceInfoWarning,
    required this.statusData,
    required this.statusWarning,
    required this.statusError,
    this.lastError,
    this.telemetryError,
  });

  final BluetoothAdapterState adapterState;
  final bool locationServiceEnabled;
  final PermissionStatus locationPermission;
  final bool isScanning;
  final bool scanRequested;
  final Map<String, NavigaScanDevice> devices;
  final ConnectionStatus connectionStatus;
  final String? connectedDeviceId;
  final String? lastConnectedDeviceId;
  final bool isDiscoveringServices;
  final DeviceInfoData? deviceInfo;
  final String? deviceInfoWarning;
  final StatusData? statusData;
  final String? statusWarning;
  final String? statusError;
  final String? lastError;
  final String? telemetryError;

  factory ConnectState.initial() => ConnectState(
    adapterState: BluetoothAdapterState.unknown,
    locationServiceEnabled: false,
    locationPermission: PermissionStatus.denied,
    isScanning: false,
    scanRequested: false,
    devices: const {},
    connectionStatus: ConnectionStatus.idle,
    connectedDeviceId: null,
    lastConnectedDeviceId: null,
    isDiscoveringServices: false,
    deviceInfo: null,
    deviceInfoWarning: null,
    statusData: null,
    statusWarning: null,
    statusError: null,
  );

  ConnectState copyWith({
    BluetoothAdapterState? adapterState,
    bool? locationServiceEnabled,
    PermissionStatus? locationPermission,
    bool? isScanning,
    bool? scanRequested,
    Map<String, NavigaScanDevice>? devices,
    ConnectionStatus? connectionStatus,
    String? connectedDeviceId,
    String? lastConnectedDeviceId,
    String? lastError,
    bool? isDiscoveringServices,
    DeviceInfoData? deviceInfo,
    String? deviceInfoWarning,
    StatusData? statusData,
    String? statusWarning,
    String? statusError,
    bool clearStatusWarning = false,
    bool clearStatusError = false,
    String? telemetryError,
  }) {
    return ConnectState(
      adapterState: adapterState ?? this.adapterState,
      locationServiceEnabled:
          locationServiceEnabled ?? this.locationServiceEnabled,
      locationPermission: locationPermission ?? this.locationPermission,
      isScanning: isScanning ?? this.isScanning,
      scanRequested: scanRequested ?? this.scanRequested,
      devices: devices ?? this.devices,
      connectionStatus: connectionStatus ?? this.connectionStatus,
      connectedDeviceId: connectedDeviceId ?? this.connectedDeviceId,
      lastConnectedDeviceId:
          lastConnectedDeviceId ?? this.lastConnectedDeviceId,
      lastError: lastError ?? this.lastError,
      isDiscoveringServices:
          isDiscoveringServices ?? this.isDiscoveringServices,
      deviceInfo: deviceInfo ?? this.deviceInfo,
      deviceInfoWarning: deviceInfoWarning ?? this.deviceInfoWarning,
      statusData: statusData ?? this.statusData,
      statusWarning: clearStatusWarning
          ? null
          : (statusWarning ?? this.statusWarning),
      statusError: clearStatusError ? null : (statusError ?? this.statusError),
      telemetryError: telemetryError ?? this.telemetryError,
    );
  }

  List<NavigaScanDevice> get deviceList {
    final list = devices.values.toList()
      ..sort((a, b) => b.lastSeen.compareTo(a.lastSeen));
    return list;
  }
}

class NavigaScanDevice {
  const NavigaScanDevice({
    required this.id,
    required this.name,
    required this.rssi,
    required this.lastSeen,
    this.bleContractVersionMajor,
    this.bleContractVersionMinor,
  });

  final String id;
  final String name;
  final int rssi;
  final DateTime lastSeen;

  /// S04 Slice 1: BLE contract version from advertising (null if not present).
  final int? bleContractVersionMajor;
  final int? bleContractVersionMinor;

  bool get isBleContractVersionCompatible =>
      bleContractVersionMajor == kBLEContractVersionMajor &&
      bleContractVersionMinor == kBLEContractVersionMinor;
}

class BleParseResult<T> {
  const BleParseResult({required this.data, this.warning});

  final T? data;
  final String? warning;
}

class DeviceInfoData {
  const DeviceInfoData({
    required this.formatVer,
    required this.bleSchemaVer,
    required this.raw,
    this.radioProtoVer,
    this.nodeId,
    this.shortId,
    this.deviceType,
    this.firmwareVersion,
    this.radioModuleModel,
    this.bandId,
    this.powerMin,
    this.powerMax,
    this.channelMin,
    this.channelMax,
    this.networkMode,
    this.channelId,
    this.publicChannelId,
    this.privateChannelMin,
    this.privateChannelMax,
    this.capabilities,
  });

  final int formatVer;
  final int bleSchemaVer;
  final int? radioProtoVer;
  final int? nodeId;
  final int? shortId;
  final int? deviceType;
  final String? firmwareVersion;
  final String? radioModuleModel;
  final int? bandId;
  final int? powerMin;
  final int? powerMax;
  final int? channelMin;
  final int? channelMax;
  final int? networkMode;
  final int? channelId;
  final int? publicChannelId;
  final int? privateChannelMin;
  final int? privateChannelMax;
  final int? capabilities;
  final List<int> raw;
}

class BleDeviceInfoParser {
  static BleParseResult<DeviceInfoData> parse(List<int> data) {
    final reader = BleByteReader(data);
    final formatVer = reader.readU8();
    final bleSchemaVer = reader.readU8();
    if (formatVer == null || bleSchemaVer == null) {
      return const BleParseResult(
        data: null,
        warning: 'DeviceInfo payload too short',
      );
    }

    if (formatVer != 1) {
      return BleParseResult(
        data: DeviceInfoData(
          formatVer: formatVer,
          bleSchemaVer: bleSchemaVer,
          raw: data,
        ),
        warning: 'Unknown DeviceInfo format_ver=$formatVer',
      );
    }

    final withRadio = _parseV1(data, includeRadioProto: true);
    if (withRadio != null) {
      return BleParseResult(data: withRadio);
    }

    final withoutRadio = _parseV1(data, includeRadioProto: false);
    if (withoutRadio != null) {
      return BleParseResult(
        data: withoutRadio,
        warning: 'DeviceInfo radio_proto_ver missing',
      );
    }

    return const BleParseResult(
      data: null,
      warning: 'DeviceInfo payload malformed',
    );
  }

  static DeviceInfoData? _parseV1(
    List<int> data, {
    required bool includeRadioProto,
  }) {
    final reader = BleByteReader(data);
    final formatVer = reader.readU8();
    final bleSchemaVer = reader.readU8();
    if (formatVer == null || bleSchemaVer == null) {
      return null;
    }

    int? radioProtoVer;
    if (includeRadioProto) {
      radioProtoVer = reader.readU8();
      if (radioProtoVer == null) {
        return null;
      }
    }

    final nodeId = reader.readU64Le();
    final shortId = reader.readU16Le();
    final deviceType = reader.readU8();
    final firmwareVersion = reader.readLenPrefixedString();
    final radioModuleModel = reader.readLenPrefixedString();
    final bandId = reader.readU16Le();
    final powerMin = reader.readU16Le();
    final powerMax = reader.readU16Le();
    final channelMin = reader.readU16Le();
    final channelMax = reader.readU16Le();
    final networkMode = reader.readU8();
    final channelId = reader.readU16Le();
    final publicChannelId = reader.readU16Le();
    final privateChannelMin = reader.readU16Le();
    final privateChannelMax = reader.readU16Le();
    final capabilities = reader.readU32Le();

    if (reader.hasErrors) {
      return null;
    }

    return DeviceInfoData(
      formatVer: formatVer,
      bleSchemaVer: bleSchemaVer,
      radioProtoVer: radioProtoVer,
      nodeId: nodeId,
      shortId: shortId,
      deviceType: deviceType,
      firmwareVersion: firmwareVersion,
      radioModuleModel: radioModuleModel,
      bandId: bandId,
      powerMin: powerMin,
      powerMax: powerMax,
      channelMin: channelMin,
      channelMax: channelMax,
      networkMode: networkMode,
      channelId: channelId,
      publicChannelId: publicChannelId,
      privateChannelMin: privateChannelMin,
      privateChannelMax: privateChannelMax,
      capabilities: capabilities,
      raw: data,
    );
  }
}

class NodeTableSnapshotHeader {
  const NodeTableSnapshotHeader({
    required this.snapshotId,
    required this.totalNodes,
    required this.pageIndex,
    required this.pageSize,
    required this.pageCount,
    required this.recordFormatVer,
  });

  final int snapshotId;
  final int totalNodes;
  final int pageIndex;
  final int pageSize;
  final int pageCount;
  final int recordFormatVer;
}

/// S04 #464: Canon BLE record (format ver 2). lastSeq excluded from BLE; use 0 when from BLE.
/// Product-facing staleness is [isStale]. [isGrey] is a backward-compat getter for the same value.
class NodeRecordV1 {
  const NodeRecordV1({
    required this.nodeId,
    required this.shortId,
    required this.isSelf,
    required this.posValid,
    required this.isStale,
    required this.shortIdCollision,
    required this.lastSeenAgeS,
    required this.latE7,
    required this.lonE7,
    required this.posAgeS,
    required this.lastRxRssi,
    required this.lastSeq,
    this.nodeName = '',
    this.snrLast,
    this.hasPosFlags = false,
    this.posFlags = 0,
    this.hasSats = false,
    this.sats = 0,
    this.hasBattery = false,
    this.batteryPercent = 0xff,
    this.hasUptime = false,
    this.uptimeSec = 0xffffffff,
    this.hasMaxSilence = false,
    this.maxSilence10s = 0,
    this.hasHwProfile = false,
    this.hwProfileId = 0xffff,
    this.hasFwVersion = false,
    this.fwVersionId = 0xffff,
  });

  final int nodeId;
  final int shortId;
  final bool isSelf;
  final bool posValid;

  /// Canon product-facing staleness (BLE baseline path). Prefer this over [isGrey].
  final bool isStale;

  /// Backward compat: same as [isStale]. Do not use for new BLE-baseline code.
  bool get isGrey => isStale;
  final bool shortIdCollision;
  final int lastSeenAgeS;
  final int latE7;
  final int lonE7;
  final int posAgeS;
  final int lastRxRssi;

  /// Excluded from BLE export per canon; 0 when record is from BLE.
  final int lastSeq;
  final String nodeName;
  final int? snrLast;
  final bool hasPosFlags;
  final int posFlags;
  final bool hasSats;
  final int sats;
  final bool hasBattery;
  final int batteryPercent;
  final bool hasUptime;
  final int uptimeSec;
  final bool hasMaxSilence;
  final int maxSilence10s;
  final bool hasHwProfile;
  final int hwProfileId;
  final bool hasFwVersion;
  final int fwVersionId;
}

class NodeTableSnapshotPage {
  const NodeTableSnapshotPage({required this.header, required this.records});

  final NodeTableSnapshotHeader header;
  final List<NodeRecordV1> records;
}

/// S04 #464: Canon BLE record size (format ver 2).
const int kNodeRecordBytesBleCanon = 72;

/// Legacy format 1 record size (26 bytes); supported for backward compatibility with older firmware.
const int kNodeRecordBytesFormat1 = 26;

class BleNodeTableParser {
  static BleParseResult<NodeTableSnapshotPage> parsePage(List<int> data) {
    if (data.length < 10) {
      return const BleParseResult(
        data: null,
        warning: 'NodeTableSnapshot payload too short',
      );
    }
    final reader = BleByteReader(data);
    final snapshotId = reader.readU16Le();
    final totalNodes = reader.readU16Le();
    final pageIndex = reader.readU16Le();
    final pageSize = reader.readU8();
    final pageCount = reader.readU16Le();
    final recordFormatVer = reader.readU8();
    if (reader.hasErrors ||
        snapshotId == null ||
        totalNodes == null ||
        pageIndex == null ||
        pageSize == null ||
        pageCount == null ||
        recordFormatVer == null) {
      return const BleParseResult(
        data: null,
        warning: 'NodeTableSnapshot header malformed',
      );
    }
    final recordsBytes = data.length - 10;
    if (recordFormatVer == 1) {
      if (recordsBytes % kNodeRecordBytesFormat1 != 0) {
        return BleParseResult(
          data: null,
          warning:
              'NodeTableSnapshot format 1 payload size invalid (${data.length})',
        );
      }
      return _parsePageFormat1(
        data: data,
        offset: 10,
        snapshotId: snapshotId,
        totalNodes: totalNodes,
        pageIndex: pageIndex,
        pageSize: pageSize,
        pageCount: pageCount,
      );
    }
    if (recordFormatVer == 2) {
      if (recordsBytes % kNodeRecordBytesBleCanon != 0) {
        return BleParseResult(
          data: null,
          warning:
              'NodeTableSnapshot format 2 payload size invalid (${data.length})',
        );
      }
      return _parsePageFormat2(
        data: data,
        offset: 10,
        snapshotId: snapshotId,
        totalNodes: totalNodes,
        pageIndex: pageIndex,
        pageSize: pageSize,
        pageCount: pageCount,
      );
    }
    return BleParseResult(
      data: null,
      warning: 'Unknown NodeRecord format_ver=$recordFormatVer',
    );
  }

  /// S04 #465: Parse subscription batch (1 byte count + N×72-byte canon records). Returns empty list on error.
  static List<NodeRecordV1> parseSubscriptionBatch(List<int> data) {
    if (data.isEmpty) return [];
    final count = data[0].clamp(0, 5);
    if (1 + count * kNodeRecordBytesBleCanon > data.length) return [];
    final records = <NodeRecordV1>[];
    for (var i = 0; i < count; i++) {
      final offset = 1 + i * kNodeRecordBytesBleCanon;
      final record = _parseOneRecordFormat2(data, offset);
      if (record != null) records.add(record);
    }
    return records;
  }

  static NodeRecordV1? _parseOneRecordFormat2(List<int> data, int base) {
    if (base + kNodeRecordBytesBleCanon > data.length) return null;
    final nodeId = _readU64Le(data, base + 0);
    final shortId = _readU16Le(data, base + 8);
    final flags1 = data[base + 10];
    final lastSeenAgeS = _readU16Le(data, base + 11);
    final latE7 = _toSigned32(_readU32Le(data, base + 13));
    final lonE7 = _toSigned32(_readU32Le(data, base + 17));
    final posAgeS = _readU16Le(data, base + 21);
    final flags2 = data[base + 23];
    final posFlags = data[base + 24];
    final sats = data[base + 25];
    final flags3 = data[base + 26];
    final batteryPercent = data[base + 27];
    final uptimeSec = _readU32Le(data, base + 28);
    final maxSilence10s = data[base + 32];
    final hwProfileId = _readU16Le(data, base + 33);
    final fwVersionId = _readU16Le(data, base + 35);
    final lastRxRssi = _toSigned8(data[base + 37]);
    final snrLast = _toSigned8(data[base + 38]);
    final nameLen = data[base + 39].clamp(0, 32);
    final nodeName = nameLen > 0
        ? String.fromCharCodes(data.sublist(base + 40, base + 40 + nameLen))
        : '';

    final isSelf = (flags1 & 0x01) != 0;
    final shortIdCollision = (flags1 & 0x02) != 0;
    final posValid = (flags1 & 0x04) != 0;
    final isStale = (flags1 & 0x08) != 0;
    final hasPosFlags = (flags2 & 0x01) != 0;
    final hasSats = (flags2 & 0x02) != 0;
    final hasBattery = (flags3 & 0x01) != 0;
    final hasUptime = (flags3 & 0x02) != 0;
    final hasMaxSilence = (flags3 & 0x04) != 0;
    final hasHwProfile = (flags3 & 0x08) != 0;
    final hasFwVersion = (flags3 & 0x10) != 0;

    return NodeRecordV1(
      nodeId: nodeId,
      shortId: shortId,
      isSelf: isSelf,
      posValid: posValid,
      isStale: isStale,
      shortIdCollision: shortIdCollision,
      lastSeenAgeS: lastSeenAgeS,
      latE7: latE7,
      lonE7: lonE7,
      posAgeS: posAgeS,
      lastRxRssi: lastRxRssi,
      lastSeq: 0,
      nodeName: nodeName,
      snrLast: snrLast,
      hasPosFlags: hasPosFlags,
      posFlags: posFlags,
      hasSats: hasSats,
      sats: sats,
      hasBattery: hasBattery,
      batteryPercent: batteryPercent,
      hasUptime: hasUptime,
      uptimeSec: uptimeSec,
      hasMaxSilence: hasMaxSilence,
      maxSilence10s: maxSilence10s,
      hasHwProfile: hasHwProfile,
      hwProfileId: hwProfileId,
      hasFwVersion: hasFwVersion,
      fwVersionId: fwVersionId,
    );
  }

  static BleParseResult<NodeTableSnapshotPage> _parsePageFormat1({
    required List<int> data,
    required int offset,
    required int snapshotId,
    required int totalNodes,
    required int pageIndex,
    required int pageSize,
    required int pageCount,
  }) {
    final records = <NodeRecordV1>[];
    final recordCount = (data.length - offset) ~/ kNodeRecordBytesFormat1;
    for (var i = 0; i < recordCount; i++) {
      final base = offset + i * kNodeRecordBytesFormat1;
      if (base + kNodeRecordBytesFormat1 > data.length) break;
      final nodeId = _readU64Le(data, base + 0);
      final shortId = _readU16Le(data, base + 8);
      final flags = data[base + 10];
      final lastSeenAgeS = _readU16Le(data, base + 11);
      final latE7 = _toSigned32(_readU32Le(data, base + 13));
      final lonE7 = _toSigned32(_readU32Le(data, base + 17));
      final posAgeS = _readU16Le(data, base + 21);
      final lastRxRssi = _toSigned8(data[base + 23]);
      final snrLast = _toSigned8(data[base + 24]);

      final isSelf = (flags & 0x01) != 0;
      final posValid = (flags & 0x02) != 0;
      final isStale = (flags & 0x04) != 0;
      final shortIdCollision = (flags & 0x08) != 0;

      records.add(
        NodeRecordV1(
          nodeId: nodeId,
          shortId: shortId,
          isSelf: isSelf,
          posValid: posValid,
          isStale: isStale,
          shortIdCollision: shortIdCollision,
          lastSeenAgeS: lastSeenAgeS,
          latE7: latE7,
          lonE7: lonE7,
          posAgeS: posAgeS,
          lastRxRssi: lastRxRssi,
          lastSeq: 0,
          snrLast: snrLast,
        ),
      );
    }
    return BleParseResult(
      data: NodeTableSnapshotPage(
        header: NodeTableSnapshotHeader(
          snapshotId: snapshotId,
          totalNodes: totalNodes,
          pageIndex: pageIndex,
          pageSize: pageSize,
          pageCount: pageCount,
          recordFormatVer: 1,
        ),
        records: records,
      ),
    );
  }

  static BleParseResult<NodeTableSnapshotPage> _parsePageFormat2({
    required List<int> data,
    required int offset,
    required int snapshotId,
    required int totalNodes,
    required int pageIndex,
    required int pageSize,
    required int pageCount,
  }) {
    final records = <NodeRecordV1>[];
    final recordCount = (data.length - offset) ~/ kNodeRecordBytesBleCanon;
    for (var i = 0; i < recordCount; i++) {
      final base = offset + i * kNodeRecordBytesBleCanon;
      if (base + kNodeRecordBytesBleCanon > data.length) break;
      final nodeId = _readU64Le(data, base + 0);
      final shortId = _readU16Le(data, base + 8);
      final flags1 = data[base + 10];
      final lastSeenAgeS = _readU16Le(data, base + 11);
      final latE7 = _toSigned32(_readU32Le(data, base + 13));
      final lonE7 = _toSigned32(_readU32Le(data, base + 17));
      final posAgeS = _readU16Le(data, base + 21);
      final flags2 = data[base + 23];
      final posFlags = data[base + 24];
      final sats = data[base + 25];
      final flags3 = data[base + 26];
      final batteryPercent = data[base + 27];
      final uptimeSec = _readU32Le(data, base + 28);
      final maxSilence10s = data[base + 32];
      final hwProfileId = _readU16Le(data, base + 33);
      final fwVersionId = _readU16Le(data, base + 35);
      final lastRxRssi = _toSigned8(data[base + 37]);
      final snrLast = _toSigned8(data[base + 38]);
      final nameLen = data[base + 39].clamp(0, 32);
      final nodeName = nameLen > 0
          ? String.fromCharCodes(data.sublist(base + 40, base + 40 + nameLen))
          : '';

      final isSelf = (flags1 & 0x01) != 0;
      final shortIdCollision = (flags1 & 0x02) != 0;
      final posValid = (flags1 & 0x04) != 0;
      final isStale = (flags1 & 0x08) != 0;
      final hasPosFlags = (flags2 & 0x01) != 0;
      final hasSats = (flags2 & 0x02) != 0;
      final hasBattery = (flags3 & 0x01) != 0;
      final hasUptime = (flags3 & 0x02) != 0;
      final hasMaxSilence = (flags3 & 0x04) != 0;
      final hasHwProfile = (flags3 & 0x08) != 0;
      final hasFwVersion = (flags3 & 0x10) != 0;

      records.add(
        NodeRecordV1(
          nodeId: nodeId,
          shortId: shortId,
          isSelf: isSelf,
          posValid: posValid,
          isStale: isStale,
          shortIdCollision: shortIdCollision,
          lastSeenAgeS: lastSeenAgeS,
          latE7: latE7,
          lonE7: lonE7,
          posAgeS: posAgeS,
          lastRxRssi: lastRxRssi,
          lastSeq: 0,
          nodeName: nodeName,
          snrLast: snrLast,
          hasPosFlags: hasPosFlags,
          posFlags: posFlags,
          hasSats: hasSats,
          sats: sats,
          hasBattery: hasBattery,
          batteryPercent: batteryPercent,
          hasUptime: hasUptime,
          uptimeSec: uptimeSec,
          hasMaxSilence: hasMaxSilence,
          maxSilence10s: maxSilence10s,
          hasHwProfile: hasHwProfile,
          hwProfileId: hwProfileId,
          hasFwVersion: hasFwVersion,
          fwVersionId: fwVersionId,
        ),
      );
    }
    return BleParseResult(
      data: NodeTableSnapshotPage(
        header: NodeTableSnapshotHeader(
          snapshotId: snapshotId,
          totalNodes: totalNodes,
          pageIndex: pageIndex,
          pageSize: pageSize,
          pageCount: pageCount,
          recordFormatVer: 2,
        ),
        records: records,
      ),
    );
  }

  static int _readU16Le(List<int> d, int i) =>
      (d[i] & 0xFF) | ((d[i + 1] & 0xFF) << 8);
  static int _readU32Le(List<int> d, int i) =>
      (d[i] & 0xFF) |
      ((d[i + 1] & 0xFF) << 8) |
      ((d[i + 2] & 0xFF) << 16) |
      ((d[i + 3] & 0xFF) << 24);
  static int _readU64Le(List<int> d, int i) {
    var v = 0;
    for (var j = 0; j < 8; j++) {
      v |= (d[i + j] & 0xFF) << (j * 8);
    }
    return v;
  }

  static int _toSigned32(int value) {
    if (value & 0x80000000 != 0) {
      return value - 0x100000000;
    }
    return value;
  }

  static int _toSigned8(int value) {
    if (value & 0x80 != 0) {
      return value - 0x100;
    }
    return value;
  }
}

class BleByteReader {
  BleByteReader(this._data);

  final List<int> _data;
  int _offset = 0;
  bool hasErrors = false;

  int get remaining => _data.length - _offset;

  int? readU8() {
    if (_offset + 1 > _data.length) {
      hasErrors = true;
      return null;
    }
    return _data[_offset++];
  }

  int? readU16Le() {
    if (_offset + 2 > _data.length) {
      hasErrors = true;
      return null;
    }
    final value = _data[_offset] | (_data[_offset + 1] << 8);
    _offset += 2;
    return value;
  }

  int? readU32Le() {
    if (_offset + 4 > _data.length) {
      hasErrors = true;
      return null;
    }
    final value =
        _data[_offset] |
        (_data[_offset + 1] << 8) |
        (_data[_offset + 2] << 16) |
        (_data[_offset + 3] << 24);
    _offset += 4;
    return value;
  }

  int? readU64Le() {
    if (_offset + 8 > _data.length) {
      hasErrors = true;
      return null;
    }
    var value = 0;
    for (var i = 0; i < 8; i++) {
      value |= _data[_offset + i] << (8 * i);
    }
    _offset += 8;
    return value;
  }

  String? readLenPrefixedString() {
    final len = readU8();
    if (len == null) {
      return null;
    }
    if (_offset + len > _data.length) {
      hasErrors = true;
      return null;
    }
    final bytes = _data.sublist(_offset, _offset + len);
    _offset += len;
    return utf8.decode(bytes, allowMalformed: true);
  }
}
