import 'dart:async';
import 'dart:io';

import 'package:android_intent_plus/android_intent.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_riverpod/legacy.dart';
import 'package:geolocator/geolocator.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../shared/logging.dart';

const String kNavigaServiceUuid = '6e4f0001-1b9a-4c3a-9a3b-000000000001';
const String kNavigaNamePrefix = 'Naviga';
const int? kNavigaManufacturerId =
    null; // TODO: set when manufacturer id is known

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
    _connectionSub = bluetoothDevice.connectionState.listen((event) {
      if (event == BluetoothConnectionState.connected) {
        state = state.copyWith(
          connectionStatus: ConnectionStatus.connected,
          connectedDeviceId: device.id,
        );
      }
      if (event == BluetoothConnectionState.disconnected) {
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
      await _activeDevice!.disconnect();
    } catch (error) {
      state = state.copyWith(
        connectionStatus: ConnectionStatus.failed,
        lastError: error.toString(),
      );
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
    this.lastError,
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
  final String? lastError;

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
