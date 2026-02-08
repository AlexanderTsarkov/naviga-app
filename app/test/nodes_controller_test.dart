import 'dart:async';

import 'package:flutter_test/flutter_test.dart';
import 'package:naviga_app/features/connect/connect_controller.dart';
import 'package:naviga_app/features/nodes/node_table_fetcher.dart';
import 'package:naviga_app/features/nodes/node_table_repository.dart';
import 'package:naviga_app/features/nodes/nodes_controller.dart';
import 'package:naviga_app/features/nodes/nodes_state.dart';

class FakeFetcher implements NodeTableFetcher, NodeTableSnapshotIdSource {
  FakeFetcher({required this.records, this.snapshotId, this.error, this.gate});

  final List<NodeRecordV1> records;
  final int? snapshotId;
  final Object? error;
  final Completer<void>? gate;

  @override
  Future<List<NodeRecordV1>> fetch() async {
    if (error != null) {
      throw error!;
    }
    if (gate != null) {
      await gate!.future;
    }
    return records;
  }

  @override
  int? get lastSnapshotId => snapshotId;
}

NodeRecordV1 _record({
  required int nodeId,
  required bool isSelf,
  required int lastSeenAgeS,
}) {
  return NodeRecordV1(
    nodeId: nodeId,
    shortId: nodeId & 0xFFFF,
    isSelf: isSelf,
    posValid: false,
    isGrey: false,
    shortIdCollision: false,
    lastSeenAgeS: lastSeenAgeS,
    latE7: 0,
    lonE7: 0,
    posAgeS: 0,
    lastRxRssi: -40,
    lastSeq: 0,
  );
}

void main() {
  test('refresh sorts records and detects self', () async {
    final fetcher = FakeFetcher(
      records: [
        _record(nodeId: 1, isSelf: false, lastSeenAgeS: 10),
        _record(nodeId: 2, isSelf: true, lastSeenAgeS: 30),
        _record(nodeId: 3, isSelf: false, lastSeenAgeS: 5),
      ],
      snapshotId: 7,
    );
    final repo = NodeTableRepository(fetcher);
    final controller = NodesController(repo);
    await controller.refresh();

    final state = controller.state;
    expect(state.self?.nodeId, 2);
    expect(state.snapshotId, 7);
    expect(state.recordsSorted.map((r) => r.nodeId).toList(), [2, 3, 1]);
  });

  test('refresh toggles loading state', () async {
    final gate = Completer<void>();
    final fetcher = FakeFetcher(
      records: [_record(nodeId: 1, isSelf: false, lastSeenAgeS: 1)],
      gate: gate,
    );
    final repo = NodeTableRepository(fetcher);
    final controller = NodesController(repo);
    final refreshFuture = controller.refresh();

    expect(controller.state.isLoading, isTrue);
    gate.complete();
    await refreshFuture;

    expect(controller.state.isLoading, isFalse);
  });

  test('refresh sets error on failure', () async {
    final fetcher = FakeFetcher(records: const [], error: StateError('boom'));
    final repo = NodeTableRepository(fetcher);
    final controller = NodesController(repo);
    await controller.refresh();

    expect(controller.state.isLoading, isFalse);
    expect(controller.state.error, contains('boom'));
  });
}
