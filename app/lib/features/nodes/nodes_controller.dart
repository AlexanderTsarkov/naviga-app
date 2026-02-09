import 'dart:async';

import 'package:flutter_riverpod/legacy.dart';

import '../../shared/logging.dart';
import '../connect/connect_controller.dart';
import 'node_table_cache.dart';
import 'node_table_debug_hook.dart';
import 'node_table_fetcher.dart';
import 'node_table_repository.dart';
import 'nodes_state.dart';

final nodeTableRepositoryProvider = StateProvider<NodeTableRepository>((ref) {
  final controller = ref.read(connectControllerProvider.notifier);
  final fetcher = ConnectControllerNodeTableFetcher(controller);
  return NodeTableRepository(fetcher);
});

final nodesControllerProvider =
    StateNotifierProvider<NodesController, NodesState>((ref) {
      final repository = ref.read(nodeTableRepositoryProvider);
      final controller = NodesController(repository);
      ref.listen<ConnectState>(connectControllerProvider, (previous, next) {
        unawaited(controller.handleConnectionChange(previous, next));
      });
      final lastDeviceId = ref
          .read(connectControllerProvider)
          .lastConnectedDeviceId;
      if (lastDeviceId != null) {
        unawaited(controller.restoreFromCache(lastDeviceId));
      }
      return controller;
    });

class NodesController extends StateNotifier<NodesState> {
  NodesController(this._repository, {NodeTableCacheStore? cacheStore})
    : _cacheStore = cacheStore ?? NodeTableCacheStore(),
      super(NodesState.initial()) {
    if (kDebugFetchNodeTableOnConnect) {
      nodeTableDebugRefreshOnConnect = refresh;
    }
  }

  final NodeTableRepository _repository;
  final NodeTableCacheStore _cacheStore;
  String? _connectedDeviceId;
  String? _lastRestoredDeviceId;

  Future<void> refresh() async {
    logInfo('Nodes: refresh start');
    state = state.copyWith(isLoading: true, clearError: true);
    try {
      final records = await _repository.fetchLatest();
      final sorted = _sortRecords(records);
      final self = _findSelf(sorted);
      final snapshotId = _repository.lastSnapshotId;
      final savedAtMs = DateTime.now().millisecondsSinceEpoch;
      if (_connectedDeviceId != null) {
        final snapshot = CachedNodeTableSnapshot(
          deviceId: _connectedDeviceId!,
          snapshotId: snapshotId,
          savedAtMs: savedAtMs,
          records: records,
        );
        await _cacheStore.save(
          deviceId: _connectedDeviceId!,
          snapshot: snapshot,
        );
        logInfo(
          'NodesCache: save deviceId=$_connectedDeviceId '
          'snapshot=${snapshotId ?? 'n/a'} records=${records.length}',
        );
      }
      state = state.copyWith(
        records: records,
        recordsSorted: sorted,
        self: self,
        isLoading: false,
        snapshotId: snapshotId,
        isFromCache: false,
        cacheSavedAtMs: savedAtMs,
        cacheAgeMs: 0,
        clearError: true,
      );
      logInfo(
        'Nodes: refresh done records=${records.length} '
        'self=${self != null} snapshot=${snapshotId ?? 'n/a'}',
      );
    } catch (error) {
      state = state.copyWith(isLoading: false, error: error.toString());
      logInfo('Nodes: refresh failed error=$error');
    }
  }

  /// Calls [refresh]; if it fails with "characteristic missing", waits ~600ms
  /// and retries exactly once. Used for initial auto-fetch race with GATT discovery.
  Future<void> refreshWithCharacteristicRetryOnce() async {
    await refresh();
    final err = state.error;
    if (err != null &&
        (err.contains('NodeTableSnapshot characteristic missing') ||
            err.contains('characteristic missing'))) {
      await Future.delayed(const Duration(milliseconds: 600));
      await refresh();
    }
  }

  Future<void> restoreFromCache(String deviceId) async {
    if (_lastRestoredDeviceId == deviceId) {
      return;
    }
    _lastRestoredDeviceId = deviceId;
    final snapshot = await _cacheStore.restore(deviceId);
    if (snapshot == null) {
      return;
    }
    final sorted = _sortRecords(snapshot.records);
    final self = _findSelf(sorted);
    state = state.copyWith(
      records: snapshot.records,
      recordsSorted: sorted,
      self: self,
      snapshotId: snapshot.snapshotId,
      isFromCache: true,
      cacheSavedAtMs: snapshot.savedAtMs,
      cacheAgeMs: snapshot.cacheAgeMs,
      clearError: true,
    );
    logInfo(
      'NodesCache: restore deviceId=$deviceId '
      'snapshot=${snapshot.snapshotId ?? 'n/a'} '
      'records=${snapshot.records.length} ageMs=${snapshot.cacheAgeMs}',
    );
  }

  Future<void> clearCacheForDevice(String deviceId) async {
    await _cacheStore.clear(deviceId);
    logInfo('NodesCache: clear deviceId=$deviceId');
  }

  Future<void> handleConnectionChange(
    ConnectState? previous,
    ConnectState next,
  ) async {
    final prevId = previous?.connectedDeviceId;
    final nextId = next.connectedDeviceId;
    if (prevId != null && nextId == null) {
      await clearCacheForDevice(prevId);
      _connectedDeviceId = null;
      return;
    }
    if (nextId != null) {
      _connectedDeviceId = nextId;
      await restoreFromCache(nextId);
      return;
    }
    if (_lastRestoredDeviceId == null && next.lastConnectedDeviceId != null) {
      await restoreFromCache(next.lastConnectedDeviceId!);
    }
  }

  List<NodeRecordV1> _sortRecords(List<NodeRecordV1> records) {
    final sorted = List<NodeRecordV1>.from(records);
    sorted.sort((a, b) {
      if (a.isSelf != b.isSelf) {
        return a.isSelf ? -1 : 1;
      }
      final ageCompare = a.lastSeenAgeS.compareTo(b.lastSeenAgeS);
      if (ageCompare != 0) {
        return ageCompare;
      }
      return a.nodeId.compareTo(b.nodeId);
    });
    return sorted;
  }

  NodeRecordV1? _findSelf(List<NodeRecordV1> records) {
    for (final record in records) {
      if (record.isSelf) {
        return record;
      }
    }
    return null;
  }

  @override
  void dispose() {
    if (nodeTableDebugRefreshOnConnect == refresh) {
      nodeTableDebugRefreshOnConnect = null;
    }
    super.dispose();
  }
}
