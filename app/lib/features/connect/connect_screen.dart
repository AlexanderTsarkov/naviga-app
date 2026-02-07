import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:permission_handler/permission_handler.dart';

import 'connect_controller.dart';

class ConnectScreen extends ConsumerWidget {
  const ConnectScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(connectControllerProvider);
    final controller = ref.read(connectControllerProvider.notifier);
    final isReady = controller.isReadyToScan;
    final scanActive = state.scanRequested;
    final canStartScan =
        isReady && state.connectionStatus == ConnectionStatus.idle;

    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: _ReadinessCard(
            state: state,
            onRefresh: controller.refreshReadiness,
            onOpenBluetooth: controller.openBluetoothSettings,
            onOpenLocation: controller.openLocationSettings,
            onRequestPermission: controller.requestLocationPermission,
            onOpenAppSettings: controller.openAppSettingsPage,
          ),
        ),
        const SizedBox(height: 8),
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16),
          child: Row(
            children: [
              Expanded(
                child: ElevatedButton.icon(
                  onPressed: state.isScanning
                      ? controller.stopScan
                      : (canStartScan ? controller.startScan : null),
                  icon: Icon(scanActive ? Icons.stop : Icons.search),
                  label: Text(scanActive ? 'Stop scan' : 'Start scan'),
                  style: ElevatedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(vertical: 10),
                  ),
                ),
              ),
              const SizedBox(width: 8),
              _ScanStatusIndicator(isScanning: state.isScanning),
            ],
          ),
        ),
        if (state.lastError != null)
          Padding(
            padding: const EdgeInsets.all(16),
            child: Text(
              'Scan error: ${state.lastError}',
              style: Theme.of(
                context,
              ).textTheme.bodySmall?.copyWith(color: Colors.redAccent),
            ),
          ),
        const SizedBox(height: 8),
        Expanded(
          child: _DeviceList(
            devices: state.deviceList,
            connectionStatus: state.connectionStatus,
            connectedDeviceId: state.connectedDeviceId,
            onConnect: controller.connectToDevice,
            onDisconnect: controller.disconnect,
          ),
        ),
      ],
    );
  }
}

class _ReadinessCard extends StatelessWidget {
  const _ReadinessCard({
    required this.state,
    required this.onRefresh,
    required this.onOpenBluetooth,
    required this.onOpenLocation,
    required this.onRequestPermission,
    required this.onOpenAppSettings,
  });

  final ConnectState state;
  final VoidCallback onRefresh;
  final VoidCallback onOpenBluetooth;
  final VoidCallback onOpenLocation;
  final VoidCallback onRequestPermission;
  final VoidCallback onOpenAppSettings;

  @override
  Widget build(BuildContext context) {
    final lpMissing = !state.locationPermission.isGranted;
    final lpAction = _permissionAction(
      state,
      onRequestPermission,
      onOpenAppSettings,
    );

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Text(
                  'Readiness',
                  style: Theme.of(context).textTheme.titleSmall,
                ),
                const Spacer(),
                TextButton(onPressed: onRefresh, child: const Text('Refresh')),
              ],
            ),
            const SizedBox(height: 4),
            Row(
              children: [
                Expanded(
                  child: Text(
                    'BT:${_boolLabel(state.adapterState == BluetoothAdapterState.on)} '
                    'LS:${_boolLabel(state.locationServiceEnabled)} '
                    'LP:${_lpLabel(state)}',
                    style: Theme.of(context).textTheme.bodySmall,
                  ),
                ),
              ],
            ),
            if (_bluetoothNeedsAction(state) ||
                !state.locationServiceEnabled ||
                (lpMissing && lpAction != null))
              Padding(
                padding: const EdgeInsets.only(top: 4),
                child: Wrap(
                  spacing: 8,
                  runSpacing: 4,
                  children: [
                    if (_bluetoothNeedsAction(state))
                      _compactActionButton(
                        context,
                        label: 'Open BT',
                        onPressed: onOpenBluetooth,
                      ),
                    if (!state.locationServiceEnabled)
                      _compactActionButton(
                        context,
                        label: 'Enable LS',
                        onPressed: onOpenLocation,
                      ),
                    if (lpMissing && lpAction != null)
                      _compactActionButton(
                        context,
                        label: 'Grant',
                        onPressed: lpAction,
                      ),
                  ],
                ),
              ),
          ],
        ),
      ),
    );
  }

  bool _bluetoothNeedsAction(ConnectState state) {
    return state.adapterState == BluetoothAdapterState.off ||
        state.adapterState == BluetoothAdapterState.unavailable ||
        state.adapterState == BluetoothAdapterState.unauthorized;
  }

  String _boolLabel(bool value) => value ? 'On' : 'Off';

  String _lpLabel(ConnectState state) {
    return state.locationPermission.isGranted ? 'En' : 'No';
  }

  VoidCallback? _permissionAction(
    ConnectState state,
    VoidCallback requestPermission,
    VoidCallback openSettings,
  ) {
    if (state.locationPermission.isGranted) {
      return null;
    }
    if (state.locationPermission.isPermanentlyDenied) {
      return openSettings;
    }
    return requestPermission;
  }

  Widget _compactActionButton(
    BuildContext context, {
    required String label,
    required VoidCallback onPressed,
  }) {
    return TextButton(
      onPressed: onPressed,
      style: TextButton.styleFrom(
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
        minimumSize: const Size(0, 0),
        visualDensity: VisualDensity.compact,
        tapTargetSize: MaterialTapTargetSize.shrinkWrap,
      ),
      child: Text(label, style: Theme.of(context).textTheme.bodySmall),
    );
  }
}

class _ScanStatusIndicator extends StatelessWidget {
  const _ScanStatusIndicator({required this.isScanning});

  final bool isScanning;

  @override
  Widget build(BuildContext context) {
    final label = isScanning ? 'Scanning' : 'Idle';
    return Chip(
      label: Text(label),
      visualDensity: VisualDensity.compact,
      materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
    );
  }
}

class _DeviceList extends StatelessWidget {
  const _DeviceList({
    required this.devices,
    required this.connectionStatus,
    required this.connectedDeviceId,
    required this.onConnect,
    required this.onDisconnect,
  });

  final List<NavigaScanDevice> devices;
  final ConnectionStatus connectionStatus;
  final String? connectedDeviceId;
  final void Function(NavigaScanDevice device) onConnect;
  final VoidCallback onDisconnect;

  @override
  Widget build(BuildContext context) {
    if (devices.isEmpty) {
      return const Center(child: Text('No Naviga devices found yet.'));
    }

    final ordered = List<NavigaScanDevice>.from(devices);
    if (connectedDeviceId != null) {
      final index = ordered.indexWhere(
        (device) => device.id == connectedDeviceId,
      );
      if (index > 0) {
        final connected = ordered.removeAt(index);
        ordered.insert(0, connected);
      }
    }

    return ListView.separated(
      padding: const EdgeInsets.all(12),
      itemCount: ordered.length,
      separatorBuilder: (context, index) => const SizedBox(height: 8),
      itemBuilder: (context, index) {
        final device = ordered[index];
        final isConnected = _isConnected(device);
        final lastSeenSeconds = DateTime.now()
            .difference(device.lastSeen)
            .inSeconds;

        return Card(
          color: isConnected
              ? Theme.of(context).colorScheme.primary.withAlpha(15)
              : null,
          child: ListTile(
            dense: true,
            contentPadding: const EdgeInsets.symmetric(
              horizontal: 12,
              vertical: 4,
            ),
            title: Text('${device.name} • ID:${_shortId(device.id)}'),
            subtitle: Text('RSSI ${device.rssi} dBm • ${lastSeenSeconds}s ago'),
            onTap: () => _handleDeviceTap(context, device),
            trailing: _trailingIcon(device),
          ),
        );
      },
    );
  }

  void _handleDeviceTap(BuildContext context, NavigaScanDevice device) {
    if (connectionStatus == ConnectionStatus.connecting ||
        connectionStatus == ConnectionStatus.disconnecting) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Connection in progress. Please wait.')),
      );
      return;
    }

    final isConnected = _isConnected(device);
    if (isConnected) {
      onDisconnect();
    } else {
      onConnect(device);
    }
  }

  Widget _trailingIcon(NavigaScanDevice device) {
    final isConnected = _isConnected(device);
    if (isConnected) {
      return TextButton(
        onPressed: onDisconnect,
        child: const Text('Disconnect'),
      );
    }
    if (connectionStatus == ConnectionStatus.connecting &&
        connectedDeviceId == device.id) {
      return const SizedBox(
        height: 18,
        width: 18,
        child: CircularProgressIndicator(strokeWidth: 2),
      );
    }
    return const Icon(Icons.chevron_right);
  }

  bool _isConnected(NavigaScanDevice device) {
    return connectedDeviceId == device.id &&
        connectionStatus == ConnectionStatus.connected;
  }

  String _shortId(String id) {
    if (id.length <= 4) {
      return id.toUpperCase();
    }
    return id.substring(id.length - 4).toUpperCase();
  }
}
