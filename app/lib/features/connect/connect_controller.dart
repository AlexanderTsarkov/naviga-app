import 'dart:async';
import 'dart:io';

import 'package:android_intent_plus/android_intent.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_riverpod/legacy.dart';
import 'package:geolocator/geolocator.dart';
import 'package:permission_handler/permission_handler.dart';

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

  Future<void> _init() async {
    final supported = await _isBleSupported();
    if (supported) {
      _adapterSub = FlutterBluePlus.adapterState.listen((state) {
        this.state = this.state.copyWith(adapterState: state);
      });

      _isScanningSub = FlutterBluePlus.isScanning.listen((isScanning) {
        state = state.copyWith(isScanning: isScanning);
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

    state = state.copyWith(lastError: null);

    try {
      await FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 15),
        androidUsesFineLocation: true,
      );
    } catch (error) {
      state = state.copyWith(lastError: error.toString());
    }
  }

  Future<void> stopScan() async {
    await FlutterBluePlus.stopScan();
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
    super.dispose();
  }
}

class ConnectState {
  const ConnectState({
    required this.adapterState,
    required this.locationServiceEnabled,
    required this.locationPermission,
    required this.isScanning,
    required this.devices,
    this.lastError,
  });

  final BluetoothAdapterState adapterState;
  final bool locationServiceEnabled;
  final PermissionStatus locationPermission;
  final bool isScanning;
  final Map<String, NavigaScanDevice> devices;
  final String? lastError;

  factory ConnectState.initial() => ConnectState(
    adapterState: BluetoothAdapterState.unknown,
    locationServiceEnabled: false,
    locationPermission: PermissionStatus.denied,
    isScanning: false,
    devices: const {},
  );

  ConnectState copyWith({
    BluetoothAdapterState? adapterState,
    bool? locationServiceEnabled,
    PermissionStatus? locationPermission,
    bool? isScanning,
    Map<String, NavigaScanDevice>? devices,
    String? lastError,
  }) {
    return ConnectState(
      adapterState: adapterState ?? this.adapterState,
      locationServiceEnabled:
          locationServiceEnabled ?? this.locationServiceEnabled,
      locationPermission: locationPermission ?? this.locationPermission,
      isScanning: isScanning ?? this.isScanning,
      devices: devices ?? this.devices,
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
