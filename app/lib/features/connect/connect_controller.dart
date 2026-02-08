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

const String kNavigaServiceUuid = '6e4f0001-1b9a-4c3a-9a3b-000000000001';
const String kNavigaDeviceInfoUuid = '6e4f0002-1b9a-4c3a-9a3b-000000000001';
const String kNavigaNodeTableSnapshotUuid =
    '6e4f0003-1b9a-4c3a-9a3b-000000000001';
const String kNavigaNamePrefix = 'Naviga';
const int? kNavigaManufacturerId =
    null; // TODO: set when manufacturer id is known
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
  int? _lastNodeTableSnapshotId;

  static const _prefsLastDeviceKey = 'naviga.last_device_id';

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
    state = state.copyWith(
      isDiscoveringServices: true,
      telemetryError: null,
      deviceInfo: null,
      deviceInfoWarning: null,
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
      if (kDebugFetchNodeTableOnConnect &&
          nodeTableDebugRefreshOnConnect != null) {
        try {
          await nodeTableDebugRefreshOnConnect!();
        } catch (error) {
          logInfo('NodeTable: debug refresh failed: $error');
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
      );
    } catch (error) {
      state = state.copyWith(telemetryError: 'DeviceInfo read failed: $error');
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
    _deviceInfoCharacteristic = null;
    _nodeTableSnapshotCharacteristic = null;
    _lastNodeTableSnapshotId = null;
    if (clearState) {
      state = state.copyWith(
        deviceInfo: null,
        deviceInfoWarning: null,
        telemetryError: null,
        isDiscoveringServices: false,
      );
    }
  }

  int? get lastNodeTableSnapshotId => _lastNodeTableSnapshotId;

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
      updated[id] = NavigaScanDevice(
        id: id,
        name: name,
        rssi: result.rssi,
        lastSeen: DateTime.now(),
      );
    }

    if (updated.isNotEmpty) {
      state = state.copyWith(devices: updated);
    }
  }

  bool _isNavigaDevice(ScanResult result) {
    final adv = result.advertisementData;
    final navigaGuid = Guid(kNavigaServiceUuid);

    if (adv.serviceUuids.contains(navigaGuid)) {
      return true;
    }

    if (kNavigaManufacturerId != null &&
        adv.manufacturerData.containsKey(kNavigaManufacturerId)) {
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
  });

  final String id;
  final String name;
  final int rssi;
  final DateTime lastSeen;
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

class NodeRecordV1 {
  const NodeRecordV1({
    required this.nodeId,
    required this.shortId,
    required this.isSelf,
    required this.posValid,
    required this.isGrey,
    required this.shortIdCollision,
    required this.lastSeenAgeS,
    required this.latE7,
    required this.lonE7,
    required this.posAgeS,
    required this.lastRxRssi,
    required this.lastSeq,
  });

  final int nodeId;
  final int shortId;
  final bool isSelf;
  final bool posValid;
  final bool isGrey;
  final bool shortIdCollision;
  final int lastSeenAgeS;
  final int latE7;
  final int lonE7;
  final int posAgeS;
  final int lastRxRssi;
  final int lastSeq;
}

class NodeTableSnapshotPage {
  const NodeTableSnapshotPage({required this.header, required this.records});

  final NodeTableSnapshotHeader header;
  final List<NodeRecordV1> records;
}

class BleNodeTableParser {
  static BleParseResult<NodeTableSnapshotPage> parsePage(List<int> data) {
    if (data.length < 10) {
      return const BleParseResult(
        data: null,
        warning: 'NodeTableSnapshot payload too short',
      );
    }
    final recordsBytes = data.length - 10;
    if (recordsBytes % 26 != 0) {
      return BleParseResult(
        data: null,
        warning: 'NodeTableSnapshot payload size invalid (${data.length})',
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
    if (recordFormatVer != 1) {
      return BleParseResult(
        data: null,
        warning: 'Unknown NodeRecord format_ver=$recordFormatVer',
      );
    }

    final recordCount = recordsBytes ~/ 26;
    final records = <NodeRecordV1>[];
    for (var i = 0; i < recordCount; i++) {
      final nodeId = reader.readU64Le();
      final shortId = reader.readU16Le();
      final flags = reader.readU8();
      final lastSeenAgeS = reader.readU16Le();
      final latRaw = reader.readU32Le();
      final lonRaw = reader.readU32Le();
      final posAgeS = reader.readU16Le();
      final lastRxRssiRaw = reader.readU8();
      final lastSeq = reader.readU16Le();
      if (reader.hasErrors ||
          nodeId == null ||
          shortId == null ||
          flags == null ||
          lastSeenAgeS == null ||
          latRaw == null ||
          lonRaw == null ||
          posAgeS == null ||
          lastRxRssiRaw == null ||
          lastSeq == null) {
        return const BleParseResult(
          data: null,
          warning: 'NodeTableSnapshot record malformed',
        );
      }

      final isSelf = (flags & 0x01) != 0;
      final posValid = (flags & 0x02) != 0;
      final isGrey = (flags & 0x04) != 0;
      final shortIdCollision = (flags & 0x08) != 0;
      final latE7 = _toSigned32(latRaw);
      final lonE7 = _toSigned32(lonRaw);
      final lastRxRssi = _toSigned8(lastRxRssiRaw);

      records.add(
        NodeRecordV1(
          nodeId: nodeId,
          shortId: shortId,
          isSelf: isSelf,
          posValid: posValid,
          isGrey: isGrey,
          shortIdCollision: shortIdCollision,
          lastSeenAgeS: lastSeenAgeS,
          latE7: latE7,
          lonE7: lonE7,
          posAgeS: posAgeS,
          lastRxRssi: lastRxRssi,
          lastSeq: lastSeq,
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
          recordFormatVer: recordFormatVer,
        ),
        records: records,
      ),
    );
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
