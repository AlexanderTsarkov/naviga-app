import '../connect/connect_controller.dart';
import 'node_table_fetcher.dart';

class NodeTableRepository {
  NodeTableRepository(this._fetcher);

  final NodeTableFetcher _fetcher;

  Future<List<NodeRecordV1>> fetchLatest() {
    return _fetcher.fetch();
  }

  int? get lastSnapshotId {
    if (_fetcher is NodeTableSnapshotIdSource) {
      return (_fetcher as NodeTableSnapshotIdSource).lastSnapshotId;
    }
    return null;
  }
}
