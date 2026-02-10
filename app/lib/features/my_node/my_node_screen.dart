import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../app/app_state.dart';
import '../../shared/app_tabs.dart';
import '../connect/connect_controller.dart';
import '../nodes/nodes_controller.dart';
import '../nodes/nodes_state.dart';

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

  /// Refresh both DeviceInfo and NodeTable (so self record and DeviceInfo stay in sync).
  Future<void> _refreshBoth() async {
    final connectController = ref.read(connectControllerProvider.notifier);
    final nodesController = ref.read(nodesControllerProvider.notifier);
    await connectController.refreshDeviceInfo();
    await nodesController.refresh();
    if (mounted) {
      setState(() => _lastFetchedAt = DateTime.now());
    }
  }

  static NodeRecordV1? _findSelf(NodesState state) {
    for (final r in state.recordsSorted) {
      if (r.isSelf) return r;
    }
    return null;
  }

  /// Self has valid position for map: pos_valid, non-zero lat/lon, sanity (lat
  /// -90..90, lon -180..180). Matches map screen filter for display.
  static bool _selfHasValidPosForMap(NodeRecordV1? r) {
    if (r == null || !r.posValid) return false;
    if (r.latE7 == 0 && r.lonE7 == 0) return false;
    final lat = r.latE7 / 1e7;
    final lon = r.lonE7 / 1e7;
    return lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180;
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
    final nodesState = ref.watch(nodesControllerProvider);
    final selfRecord = _findSelf(nodesState);
    final info = connectState.deviceInfo;
    final warning = connectState.deviceInfoWarning;
    final error = connectState.telemetryError;

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
        _refreshBoth();
      });
    }

    final isLoading = nodesState.isLoading;

    final listChildren = <Widget>[
      Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        child: Row(
          children: [
            FilledButton.icon(
              onPressed: isLoading ? null : _refreshBoth,
              icon: isLoading
                  ? const SizedBox(
                      width: 18,
                      height: 18,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    )
                  : const Icon(Icons.refresh),
              label: const Text('Refresh'),
            ),
            if (_selfHasValidPosForMap(selfRecord)) ...[
              const SizedBox(width: 8),
              FilledButton.tonalIcon(
                onPressed: () {
                  ref.read(mapFocusNodeIdProvider.notifier).state =
                      selfRecord!.nodeId;
                  ref.read(selectedTabProvider.notifier).state = AppTab.map;
                },
                icon: const Icon(Icons.map),
                label: const Text('Show on map'),
              ),
            ],
          ],
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
      if (nodesState.error != null)
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
          child: Text(
            nodesState.error!,
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
      // Optional compact header
      if (selfRecord != null || info != null)
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
          child: Wrap(
            spacing: 12,
            runSpacing: 4,
            children: [
              if (selfRecord != null) ...[
                Text(
                  'nodeId: ${selfRecord.nodeId}',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
                Text(
                  'shortId: ${selfRecord.shortId}',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
                Text(
                  'lastSeen: ${selfRecord.lastSeenAgeS}s',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
                Text(
                  'RSSI: ${selfRecord.lastRxRssi}',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
              ],
              if (info != null) ...[
                Text(
                  'firmware: ${info.firmwareVersion ?? "—"}',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
                Text(
                  'radio: ${info.radioModuleModel ?? "—"}',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
              ],
            ],
          ),
        ),
      // NodeTable (self) section
      ExpansionTile(
        title: const Text('NodeTable (self)'),
        children: [
          Padding(
            padding: const EdgeInsets.all(16),
            child: selfRecord == null
                ? const Text('Self record not found in NodeTable.')
                : Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      _RowLabel(
                        label: 'Node ID',
                        value: '${selfRecord.nodeId}',
                      ),
                      _RowLabel(
                        label: 'Short ID',
                        value: '${selfRecord.shortId}',
                      ),
                      _RowLabel(
                        label: 'Is self',
                        value: '${selfRecord.isSelf}',
                      ),
                      _RowLabel(
                        label: 'Pos valid',
                        value: '${selfRecord.posValid}',
                      ),
                      _RowLabel(
                        label: 'Is grey',
                        value: '${selfRecord.isGrey}',
                      ),
                      _RowLabel(
                        label: 'Short ID collision',
                        value: '${selfRecord.shortIdCollision}',
                      ),
                      _RowLabel(
                        label: 'Last seen age (s)',
                        value: '${selfRecord.lastSeenAgeS}',
                      ),
                      _RowLabel(
                        label: 'Lat (e7)',
                        value: '${selfRecord.latE7}',
                      ),
                      _RowLabel(
                        label: 'Lon (e7)',
                        value: '${selfRecord.lonE7}',
                      ),
                      _RowLabel(
                        label: 'Pos age (s)',
                        value: '${selfRecord.posAgeS}',
                      ),
                      _RowLabel(
                        label: 'Last RX RSSI',
                        value: '${selfRecord.lastRxRssi}',
                      ),
                      _RowLabel(
                        label: 'Last seq',
                        value: '${selfRecord.lastSeq}',
                      ),
                    ],
                  ),
          ),
        ],
      ),
      // Device Info section
      ExpansionTile(
        title: const Text('Device Info'),
        children: [
          Padding(
            padding: const EdgeInsets.all(16),
            child: info == null
                ? const Text('No device info yet. Tap Refresh.')
                : Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      _RowLabel(
                        label: 'Format ver',
                        value: _str(info.formatVer),
                      ),
                      _RowLabel(
                        label: 'BLE schema ver',
                        value: _str(info.bleSchemaVer),
                      ),
                      _RowLabel(
                        label: 'Radio proto ver',
                        value: _str(info.radioProtoVer),
                      ),
                      _RowLabel(label: 'Node ID', value: _str(info.nodeId)),
                      _RowLabel(label: 'Short ID', value: _str(info.shortId)),
                      _RowLabel(
                        label: 'Device type',
                        value: _str(info.deviceType),
                      ),
                      _RowLabel(
                        label: 'Firmware',
                        value: info.firmwareVersion ?? '—',
                      ),
                      _RowLabel(
                        label: 'Radio module',
                        value: info.radioModuleModel ?? '—',
                      ),
                      _RowLabel(label: 'Band ID', value: _str(info.bandId)),
                      _RowLabel(label: 'Power min', value: _str(info.powerMin)),
                      _RowLabel(label: 'Power max', value: _str(info.powerMax)),
                      _RowLabel(
                        label: 'Channel min',
                        value: _str(info.channelMin),
                      ),
                      _RowLabel(
                        label: 'Channel max',
                        value: _str(info.channelMax),
                      ),
                      _RowLabel(
                        label: 'Network mode',
                        value: _str(info.networkMode),
                      ),
                      _RowLabel(
                        label: 'Channel ID',
                        value: _str(info.channelId),
                      ),
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
                      const SizedBox(height: 8),
                      Text(
                        'Raw bytes',
                        style: Theme.of(context).textTheme.titleSmall,
                      ),
                      const SizedBox(height: 4),
                      SelectableText(
                        _rawBytes(info.raw),
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          fontFamily: 'monospace',
                        ),
                      ),
                    ],
                  ),
          ),
        ],
      ),
    ];

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
      padding: const EdgeInsets.symmetric(horizontal: 0, vertical: 4),
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
