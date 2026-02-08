import '../connect/connect_controller.dart';

abstract class NodeTableFetcher {
  Future<List<NodeRecordV1>> fetch();
}

abstract class NodeTableSnapshotIdSource {
  int? get lastSnapshotId;
}

class ConnectControllerNodeTableFetcher
    implements NodeTableFetcher, NodeTableSnapshotIdSource {
  ConnectControllerNodeTableFetcher(this._controller);

  final ConnectController _controller;

  @override
  Future<List<NodeRecordV1>> fetch() {
    return _controller.fetchNodeTableSnapshot();
  }

  @override
  int? get lastSnapshotId => _controller.lastNodeTableSnapshotId;
}
