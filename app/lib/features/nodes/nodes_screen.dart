import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../app/app_state.dart';
import '../../shared/app_tabs.dart';
import '../connect/connect_controller.dart';
import 'node_details_screen.dart';
import 'nodes_controller.dart';

class NodesScreen extends ConsumerStatefulWidget {
  const NodesScreen({super.key});

  @override
  ConsumerState<NodesScreen> createState() => _NodesScreenState();
}

class _NodesScreenState extends ConsumerState<NodesScreen> {
  bool _initialFetchDone = false;

  bool _isConnected(ConnectState state) {
    return state.connectionStatus == ConnectionStatus.connected &&
        state.connectedDeviceId != null;
  }

  static bool _hasValidPos(NodeRecordV1 r) => r.latE7 != 0 || r.lonE7 != 0;

  void _goToConnect() {
    ref.read(selectedTabProvider.notifier).state = AppTab.connect;
  }

  @override
  Widget build(BuildContext context) {
    final connectState = ref.watch(connectControllerProvider);
    final nodesState = ref.watch(nodesControllerProvider);
    final nodesController = ref.read(nodesControllerProvider.notifier);

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

    // Connected: fetch once on enter (with one retry if characteristic not ready)
    if (!_initialFetchDone) {
      _initialFetchDone = true;
      WidgetsBinding.instance.addPostFrameCallback((_) {
        nodesController.refreshWithCharacteristicRetryOnce();
      });
    }

    return RefreshIndicator(
      onRefresh: () => nodesController.refresh(),
      child: CustomScrollView(
        physics: const AlwaysScrollableScrollPhysics(),
        slivers: [
          SliverToBoxAdapter(
            child: Padding(
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
                ],
              ),
            ),
          ),
          if (nodesState.error != null)
            SliverToBoxAdapter(
              child: Padding(
                padding: const EdgeInsets.symmetric(
                  horizontal: 16,
                  vertical: 8,
                ),
                child: Text(
                  nodesState.error!,
                  style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                    color: Theme.of(context).colorScheme.error,
                  ),
                ),
              ),
            ),
          if (nodesState.recordsSorted.isEmpty && !nodesState.isLoading)
            const SliverFillRemaining(
              hasScrollBody: false,
              child: Center(child: Text('No nodes')),
            )
          else
            SliverList(
              delegate: SliverChildBuilderDelegate((context, index) {
                final r = nodesState.recordsSorted[index];
                return ListTile(
                  title: Text(
                    '${r.nodeId}',
                    style: Theme.of(context).textTheme.titleSmall,
                  ),
                  subtitle: Text(
                    'shortId: ${r.shortId}  lastSeen: ${r.lastSeenAgeS}s'
                    '${r.isSelf ? '  (self)' : ''}',
                  ),
                  trailing: r.posValid && _hasValidPos(r)
                      ? IconButton(
                          icon: const Icon(Icons.map),
                          tooltip: 'Show on map',
                          onPressed: () {
                            ref.read(mapFocusNodeIdProvider.notifier).state =
                                r.nodeId;
                            ref.read(selectedTabProvider.notifier).state =
                                AppTab.map;
                          },
                        )
                      : null,
                  onTap: () {
                    if (r.isSelf) {
                      ref.read(selectedTabProvider.notifier).state =
                          AppTab.myNode;
                    } else {
                      Navigator.of(context).push(
                        MaterialPageRoute<void>(
                          builder: (_) => NodeDetailsScreen(nodeId: r.nodeId),
                        ),
                      );
                    }
                  },
                );
              }, childCount: nodesState.recordsSorted.length),
            ),
        ],
      ),
    );
  }
}
