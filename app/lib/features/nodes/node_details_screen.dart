import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../app/app_state.dart';
import '../../shared/app_tabs.dart';
import '../connect/connect_controller.dart';
import 'nodes_controller.dart';
import 'nodes_state.dart';

class NodeDetailsScreen extends ConsumerWidget {
  const NodeDetailsScreen({super.key, required this.nodeId});

  final int nodeId;

  static NodeRecordV1? _findRecord(NodesState state, int id) {
    for (final r in state.recordsSorted) {
      if (r.nodeId == id) return r;
    }
    return null;
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final connectState = ref.watch(connectControllerProvider);
    final nodesState = ref.watch(nodesControllerProvider);
    final nodesController = ref.read(nodesControllerProvider.notifier);
    final record = _findRecord(nodesState, nodeId);

    final connected =
        connectState.connectionStatus == ConnectionStatus.connected &&
        connectState.connectedDeviceId != null;

    return Scaffold(
      appBar: AppBar(title: const Text('Node Details')),
      body: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          if (!connected)
            Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'Not connected. NodeTable may be from cache.',
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: Theme.of(context).colorScheme.outline,
                    ),
                  ),
                  const SizedBox(height: 8),
                  FilledButton.icon(
                    onPressed: () {
                      Navigator.of(context).pop();
                      WidgetsBinding.instance.addPostFrameCallback((_) {
                        ref.read(selectedTabProvider.notifier).state =
                            AppTab.connect;
                      });
                    },
                    icon: const Icon(Icons.bluetooth_connected),
                    label: const Text('Go to Connect'),
                  ),
                ],
              ),
            ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            child: Row(
              children: [
                FilledButton.icon(
                  onPressed: nodesState.isLoading
                      ? null
                      : () => nodesController.refresh(),
                  icon: nodesState.isLoading
                      ? const SizedBox(
                          width: 18,
                          height: 18,
                          child: CircularProgressIndicator(strokeWidth: 2),
                        )
                      : const Icon(Icons.refresh),
                  label: const Text('Refresh'),
                ),
                if (record != null && record.posValid) ...[
                  const SizedBox(width: 8),
                  FilledButton.tonalIcon(
                    onPressed: () {
                      ref.read(mapFocusNodeIdProvider.notifier).state =
                          record.nodeId;
                      ref.read(selectedTabProvider.notifier).state = AppTab.map;
                      Navigator.of(context).pop();
                    },
                    icon: const Icon(Icons.map),
                    label: const Text('Show on map'),
                  ),
                ],
              ],
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
          Expanded(
            child: record == null
                ? Center(
                    child: Padding(
                      padding: const EdgeInsets.all(24),
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const Text(
                            'Node not found (table updated).',
                            textAlign: TextAlign.center,
                          ),
                          const SizedBox(height: 16),
                          FilledButton(
                            onPressed: () => Navigator.of(context).pop(),
                            child: const Text('Back to Nodes'),
                          ),
                        ],
                      ),
                    ),
                  )
                : ListView(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 16,
                      vertical: 8,
                    ),
                    children: [
                      _RowLabel(label: 'Node ID', value: '${record.nodeId}'),
                      _RowLabel(label: 'Short ID', value: '${record.shortId}'),
                      _RowLabel(label: 'Is self', value: '${record.isSelf}'),
                      _RowLabel(
                        label: 'Pos valid',
                        value: '${record.posValid}',
                      ),
                      _RowLabel(label: 'Is grey', value: '${record.isGrey}'),
                      _RowLabel(
                        label: 'Short ID collision',
                        value: '${record.shortIdCollision}',
                      ),
                      _RowLabel(
                        label: 'Last seen age (s)',
                        value: '${record.lastSeenAgeS}',
                      ),
                      _RowLabel(label: 'Lat (e7)', value: '${record.latE7}'),
                      _RowLabel(label: 'Lon (e7)', value: '${record.lonE7}'),
                      _RowLabel(
                        label: 'Pos age (s)',
                        value: '${record.posAgeS}',
                      ),
                      _RowLabel(
                        label: 'Last RX RSSI',
                        value: '${record.lastRxRssi}',
                      ),
                      _RowLabel(label: 'Last seq', value: '${record.lastSeq}'),
                    ],
                  ),
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
      padding: const EdgeInsets.symmetric(vertical: 4),
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
