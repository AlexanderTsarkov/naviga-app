import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../app/app_state.dart';
import '../../shared/app_tabs.dart';
import '../connect/connect_controller.dart';

class MyNodeScreen extends ConsumerStatefulWidget {
  const MyNodeScreen({super.key});

  @override
  ConsumerState<MyNodeScreen> createState() => _MyNodeScreenState();
}

class _MyNodeScreenState extends ConsumerState<MyNodeScreen> {
  bool _initialFetchDone = false;
  DateTime? _lastFetchedAt;

  bool _isConnected(ConnectState state) {
    return state.connectionStatus == ConnectionStatus.connected &&
        state.connectedDeviceId != null;
  }

  void _goToConnect() {
    ref.read(selectedTabProvider.notifier).state = AppTab.connect;
  }

  Future<void> _fetchAndStamp() async {
    final controller = ref.read(connectControllerProvider.notifier);
    await controller.refreshDeviceInfo();
    if (mounted) {
      setState(() => _lastFetchedAt = DateTime.now());
    }
  }

  @override
  Widget build(BuildContext context) {
    final connectState = ref.watch(connectControllerProvider);

    if (!_isConnected(connectState)) {
      _initialFetchDone = false;
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                'Not connected. Go to Connect.',
                style: Theme.of(context).textTheme.bodyLarge,
                textAlign: TextAlign.center,
              ),
              const SizedBox(height: 16),
              FilledButton.icon(
                onPressed: _goToConnect,
                icon: const Icon(Icons.bluetooth_connected),
                label: const Text('Go to Connect'),
              ),
            ],
          ),
        ),
      );
    }

    if (!_initialFetchDone) {
      _initialFetchDone = true;
      WidgetsBinding.instance.addPostFrameCallback((_) {
        _fetchAndStamp();
      });
    }

    final info = connectState.deviceInfo;
    final warning = connectState.deviceInfoWarning;
    final error = connectState.telemetryError;

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          FilledButton.icon(
            onPressed: _fetchAndStamp,
            icon: const Icon(Icons.refresh),
            label: const Text('Refresh'),
          ),
          if (_lastFetchedAt != null)
            Padding(
              padding: const EdgeInsets.only(top: 8),
              child: Text(
                'Data fetched at: ${_lastFetchedAt!.toIso8601String()}',
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ),
          if (error != null)
            Padding(
              padding: const EdgeInsets.only(top: 8),
              child: Text(
                error,
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  color: Theme.of(context).colorScheme.error,
                ),
              ),
            ),
          if (warning != null)
            Padding(
              padding: const EdgeInsets.only(top: 8),
              child: Text(
                warning,
                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: Theme.of(context).colorScheme.outline,
                ),
              ),
            ),
          if (info != null) ...[
            const SizedBox(height: 16),
            _RowLabel(label: 'Node ID', value: '${info.nodeId ?? '—'}'),
            _RowLabel(label: 'Short ID', value: '${info.shortId ?? '—'}'),
            _RowLabel(label: 'Firmware', value: info.firmwareVersion ?? '—'),
            _RowLabel(
              label: 'Radio module',
              value: info.radioModuleModel ?? '—',
            ),
          ] else if (error == null && warning == null)
            const Padding(
              padding: EdgeInsets.only(top: 16),
              child: Text('No device info yet. Tap Refresh.'),
            ),
        ],
      ),
    );
  }
}

class _RowLabel extends StatelessWidget {
  const _RowLabel({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 120,
            child: Text(label, style: Theme.of(context).textTheme.titleSmall),
          ),
          Expanded(child: Text(value)),
        ],
      ),
    );
  }
}
