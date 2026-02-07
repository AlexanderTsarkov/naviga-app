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
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16),
          child: _ConnectionCard(
            status: state.connectionStatus,
            connectedDeviceId: state.connectedDeviceId,
            lastConnectedDeviceId: state.lastConnectedDeviceId,
            onDisconnect: controller.disconnect,
          ),
        ),
        if (state.connectionStatus == ConnectionStatus.connected ||
            state.deviceInfo != null ||
            state.isDiscoveringServices ||
            state.telemetryError != null)
          Padding(
            padding: const EdgeInsets.only(left: 16, right: 16, top: 12),
            child: _TelemetryCard(state: state),
          ),
        const SizedBox(height: 12),
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
                ),
              ),
              const SizedBox(width: 12),
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
        const SizedBox(height: 12),
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
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Text(
                  'Readiness',
                  style: Theme.of(context).textTheme.titleMedium,
                ),
                const Spacer(),
                TextButton(onPressed: onRefresh, child: const Text('Refresh')),
              ],
            ),
            const SizedBox(height: 12),
            _ReadinessRow(
              title: 'Bluetooth',
              status: _bluetoothStatusLabel(state),
              isReady: state.adapterState == BluetoothAdapterState.on,
              actionLabel: 'Open',
              onAction: _bluetoothNeedsAction(state) ? onOpenBluetooth : null,
            ),
            const SizedBox(height: 8),
            _ReadinessRow(
              title: 'Location services',
              status: state.locationServiceEnabled ? 'On' : 'Off',
              isReady: state.locationServiceEnabled,
              actionLabel: 'Turn on',
              onAction: state.locationServiceEnabled ? null : onOpenLocation,
            ),
            const SizedBox(height: 8),
            _ReadinessRow(
              title: 'Location permission',
              status: _permissionStatusLabel(state),
              isReady: state.locationPermission.isGranted,
              actionLabel: _permissionActionLabel(state),
              onAction: _permissionAction(
                state,
                onRequestPermission,
                onOpenAppSettings,
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

  String _bluetoothStatusLabel(ConnectState state) {
    switch (state.adapterState) {
      case BluetoothAdapterState.on:
        return 'On';
      case BluetoothAdapterState.off:
        return 'Off';
      case BluetoothAdapterState.unavailable:
        return 'Unavailable';
      case BluetoothAdapterState.unauthorized:
        return 'Unauthorized';
      case BluetoothAdapterState.turningOn:
        return 'Turning on';
      case BluetoothAdapterState.turningOff:
        return 'Turning off';
      case BluetoothAdapterState.unknown:
        return 'Unknown';
    }
  }

  String _permissionStatusLabel(ConnectState state) {
    final permission = state.locationPermission;
    if (permission.isGranted) {
      return 'Granted';
    }
    if (permission.isPermanentlyDenied) {
      return 'Denied permanently';
    }
    if (permission.isRestricted) {
      return 'Restricted';
    }
    return 'Denied';
  }

  String _permissionActionLabel(ConnectState state) {
    if (state.locationPermission.isPermanentlyDenied) {
      return 'Open settings';
    }
    return 'Grant';
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
}

class _ReadinessRow extends StatelessWidget {
  const _ReadinessRow({
    required this.title,
    required this.status,
    required this.isReady,
    required this.actionLabel,
    required this.onAction,
  });

  final String title;
  final String status;
  final bool isReady;
  final String actionLabel;
  final VoidCallback? onAction;

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(
          isReady ? Icons.check_circle : Icons.error_outline,
          color: isReady ? Colors.green : Colors.orange,
        ),
        const SizedBox(width: 8),
        Expanded(child: Text('$title: $status')),
        if (onAction != null)
          TextButton(onPressed: onAction, child: Text(actionLabel)),
      ],
    );
  }
}

class _ScanStatusIndicator extends StatelessWidget {
  const _ScanStatusIndicator({required this.isScanning});

  final bool isScanning;

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(
          isScanning ? Icons.wifi_tethering : Icons.wifi_tethering_off,
          color: isScanning ? Colors.green : Colors.grey,
        ),
        const SizedBox(width: 6),
        Text(isScanning ? 'Scanning' : 'Idle'),
      ],
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

    return ListView.separated(
      padding: const EdgeInsets.all(16),
      itemCount: devices.length,
      separatorBuilder: (context, index) => const SizedBox(height: 12),
      itemBuilder: (context, index) {
        final device = devices[index];
        final lastSeenSeconds = DateTime.now()
            .difference(device.lastSeen)
            .inSeconds;

        return Card(
          child: ListTile(
            title: Text(device.name),
            subtitle: Text(
              'ID: ${device.id}\nRSSI: ${device.rssi} dBm · '
              'last seen ${lastSeenSeconds}s ago',
            ),
            isThreeLine: true,
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

    final isConnected =
        connectedDeviceId == device.id &&
        connectionStatus == ConnectionStatus.connected;
    if (isConnected) {
      onDisconnect();
    } else {
      onConnect(device);
    }
  }

  Widget _trailingIcon(NavigaScanDevice device) {
    final isConnected =
        connectedDeviceId == device.id &&
        connectionStatus == ConnectionStatus.connected;
    if (isConnected) {
      return const Icon(Icons.link, color: Colors.green);
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
}

class _ConnectionCard extends StatelessWidget {
  const _ConnectionCard({
    required this.status,
    required this.connectedDeviceId,
    required this.lastConnectedDeviceId,
    required this.onDisconnect,
  });

  final ConnectionStatus status;
  final String? connectedDeviceId;
  final String? lastConnectedDeviceId;
  final VoidCallback onDisconnect;

  @override
  Widget build(BuildContext context) {
    final statusLabel = switch (status) {
      ConnectionStatus.idle => 'Idle',
      ConnectionStatus.connecting => 'Connecting…',
      ConnectionStatus.connected => 'Connected',
      ConnectionStatus.disconnecting => 'Disconnecting…',
      ConnectionStatus.failed => 'Failed',
    };

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Connection', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 8),
            Text('Status: $statusLabel'),
            if (connectedDeviceId != null)
              Text('Connected: $connectedDeviceId'),
            if (connectedDeviceId == null && lastConnectedDeviceId != null)
              Text('Last device: $lastConnectedDeviceId'),
            if (status == ConnectionStatus.connected)
              Align(
                alignment: Alignment.centerRight,
                child: TextButton(
                  onPressed: onDisconnect,
                  child: const Text('Disconnect'),
                ),
              ),
          ],
        ),
      ),
    );
  }
}

class _TelemetryCard extends StatelessWidget {
  const _TelemetryCard({required this.state});

  final ConnectState state;

  @override
  Widget build(BuildContext context) {
    final deviceInfo = state.deviceInfo;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Telemetry', style: Theme.of(context).textTheme.titleMedium),
            if (state.isDiscoveringServices) ...[
              const SizedBox(height: 8),
              const Text('Discovering services…'),
            ],
            if (state.telemetryError != null) ...[
              const SizedBox(height: 8),
              Text(
                state.telemetryError!,
                style: Theme.of(
                  context,
                ).textTheme.bodySmall?.copyWith(color: Colors.redAccent),
              ),
            ],
            if (deviceInfo != null) ...[
              const SizedBox(height: 12),
              Text('DeviceInfo', style: Theme.of(context).textTheme.titleSmall),
              const SizedBox(height: 6),
              _TelemetryRow(
                label: 'Format',
                value:
                    '${deviceInfo.formatVer} (BLE ${deviceInfo.bleSchemaVer})',
              ),
              _TelemetryRow(
                label: 'Node ID',
                value: _formatHex(deviceInfo.nodeId, digits: 16),
              ),
              _TelemetryRow(
                label: 'Short ID',
                value: _formatHex(deviceInfo.shortId, digits: 4),
              ),
              _TelemetryRow(
                label: 'Firmware',
                value: deviceInfo.firmwareVersion ?? '—',
              ),
              _TelemetryRow(
                label: 'Radio module',
                value: deviceInfo.radioModuleModel ?? '—',
              ),
              _TelemetryRow(
                label: 'Band',
                value: _formatInt(deviceInfo.bandId),
              ),
              _TelemetryRow(
                label: 'Power',
                value: _formatRange(deviceInfo.powerMin, deviceInfo.powerMax),
              ),
              _TelemetryRow(
                label: 'Channel range',
                value: _formatRange(
                  deviceInfo.channelMin,
                  deviceInfo.channelMax,
                ),
              ),
              _TelemetryRow(
                label: 'Network mode',
                value: _formatInt(deviceInfo.networkMode),
              ),
              _TelemetryRow(
                label: 'Channel ID',
                value: _formatInt(deviceInfo.channelId),
              ),
              _TelemetryRow(
                label: 'Capabilities',
                value: _formatHex(deviceInfo.capabilities, digits: 8),
              ),
              if (state.deviceInfoWarning != null)
                Padding(
                  padding: const EdgeInsets.only(top: 6),
                  child: Text(
                    state.deviceInfoWarning!,
                    style: Theme.of(
                      context,
                    ).textTheme.bodySmall?.copyWith(color: Colors.orange),
                  ),
                ),
            ],
            if (deviceInfo == null &&
                !state.isDiscoveringServices &&
                state.telemetryError == null)
              const Padding(
                padding: EdgeInsets.only(top: 8),
                child: Text('No telemetry available yet.'),
              ),
          ],
        ),
      ),
    );
  }

  String _formatInt(int? value, {String? unit}) {
    if (value == null) {
      return '—';
    }
    if (unit == null) {
      return '$value';
    }
    return '$value $unit';
  }

  String _formatRange(int? min, int? max) {
    if (min == null || max == null) {
      return '—';
    }
    return '$min–$max';
  }

  String _formatHex(int? value, {required int digits}) {
    if (value == null) {
      return '—';
    }
    return value.toRadixString(16).padLeft(digits, '0').toUpperCase();
  }
}

class _TelemetryRow extends StatelessWidget {
  const _TelemetryRow({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 4),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 110,
            child: Text(label, style: Theme.of(context).textTheme.bodySmall),
          ),
          Expanded(child: Text(value)),
        ],
      ),
    );
  }
}
