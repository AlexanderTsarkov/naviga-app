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

  static String _str(dynamic v) => v?.toString() ?? '—';
  static String _capabilities(int? c) =>
      c == null ? '—' : '0x${c.toRadixString(16).toUpperCase()}';
  static String _rawBytes(List<int> raw) => raw.isEmpty
      ? '—'
      : raw.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ');

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

    final listChildren = <Widget>[
      Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        child: FilledButton.icon(
          onPressed: _fetchAndStamp,
          icon: const Icon(Icons.refresh),
          label: const Text('Refresh'),
        ),
      ),
      if (_lastFetchedAt != null)
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
          child: Text(
            'Data fetched at: ${_lastFetchedAt!.toIso8601String()}',
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ),
      if (error != null)
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
          child: Text(
            error,
            style: Theme.of(context).textTheme.bodyMedium?.copyWith(
              color: Theme.of(context).colorScheme.error,
            ),
          ),
        ),
      if (warning != null)
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
          child: Text(
            warning,
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
              color: Theme.of(context).colorScheme.outline,
            ),
          ),
        ),
    ];

    if (info != null) {
      listChildren.addAll([
        const SizedBox(height: 8),
        _RowLabel(label: 'Format ver', value: _str(info.formatVer)),
        _RowLabel(label: 'BLE schema ver', value: _str(info.bleSchemaVer)),
        _RowLabel(label: 'Radio proto ver', value: _str(info.radioProtoVer)),
        _RowLabel(label: 'Node ID', value: _str(info.nodeId)),
        _RowLabel(label: 'Short ID', value: _str(info.shortId)),
        _RowLabel(label: 'Device type', value: _str(info.deviceType)),
        _RowLabel(label: 'Firmware', value: info.firmwareVersion ?? '—'),
        _RowLabel(label: 'Radio module', value: info.radioModuleModel ?? '—'),
        _RowLabel(label: 'Band ID', value: _str(info.bandId)),
        _RowLabel(label: 'Power min', value: _str(info.powerMin)),
        _RowLabel(label: 'Power max', value: _str(info.powerMax)),
        _RowLabel(label: 'Channel min', value: _str(info.channelMin)),
        _RowLabel(label: 'Channel max', value: _str(info.channelMax)),
        _RowLabel(label: 'Network mode', value: _str(info.networkMode)),
        _RowLabel(label: 'Channel ID', value: _str(info.channelId)),
        _RowLabel(
          label: 'Public channel ID',
          value: _str(info.publicChannelId),
        ),
        _RowLabel(
          label: 'Private channel min',
          value: _str(info.privateChannelMin),
        ),
        _RowLabel(
          label: 'Private channel max',
          value: _str(info.privateChannelMax),
        ),
        _RowLabel(
          label: 'Capabilities',
          value: _capabilities(info.capabilities),
        ),
        ExpansionTile(
          title: const Text('Raw bytes'),
          children: [
            Padding(
              padding: const EdgeInsets.all(16),
              child: SelectableText(
                _rawBytes(info.raw),
                style: Theme.of(
                  context,
                ).textTheme.bodySmall?.copyWith(fontFamily: 'monospace'),
              ),
            ),
          ],
        ),
      ]);
    } else if (error == null && warning == null) {
      listChildren.add(
        const Padding(
          padding: EdgeInsets.all(16),
          child: Text('No device info yet. Tap Refresh.'),
        ),
      );
    }

    return ListView(
      padding: const EdgeInsets.only(bottom: 24),
      children: listChildren,
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
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 140,
            child: Text(label, style: Theme.of(context).textTheme.titleSmall),
          ),
          Expanded(child: Text(value)),
        ],
      ),
    );
  }
}
