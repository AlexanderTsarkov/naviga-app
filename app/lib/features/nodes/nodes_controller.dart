import 'package:flutter_riverpod/legacy.dart';

import '../../shared/logging.dart';
import '../connect/connect_controller.dart';
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
      return NodesController(repository);
    });

class NodesController extends StateNotifier<NodesState> {
  NodesController(this._repository) : super(NodesState.initial()) {
    if (kDebugFetchNodeTableOnConnect) {
      nodeTableDebugRefreshOnConnect = refresh;
    }
  }

  final NodeTableRepository _repository;

  Future<void> refresh() async {
    logInfo('Nodes: refresh start');
    state = state.copyWith(isLoading: true, clearError: true);
    try {
      final records = await _repository.fetchLatest();
      final sorted = _sortRecords(records);
      final self = _findSelf(sorted);
      final snapshotId = _repository.lastSnapshotId;
      state = state.copyWith(
        records: records,
        recordsSorted: sorted,
        self: self,
        isLoading: false,
        snapshotId: snapshotId,
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
