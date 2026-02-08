import '../connect/connect_controller.dart';

class NodesState {
  const NodesState({
    required this.records,
    required this.recordsSorted,
    required this.self,
    required this.isLoading,
    required this.error,
    required this.snapshotId,
  });

  final List<NodeRecordV1> records;
  final List<NodeRecordV1> recordsSorted;
  final NodeRecordV1? self;
  final bool isLoading;
  final String? error;
  final int? snapshotId;

  factory NodesState.initial() => const NodesState(
    records: [],
    recordsSorted: [],
    self: null,
    isLoading: false,
    error: null,
    snapshotId: null,
  );

  NodesState copyWith({
    List<NodeRecordV1>? records,
    List<NodeRecordV1>? recordsSorted,
    NodeRecordV1? self,
    bool? isLoading,
    String? error,
    bool clearError = false,
    int? snapshotId,
  }) {
    return NodesState(
      records: records ?? this.records,
      recordsSorted: recordsSorted ?? this.recordsSorted,
      self: self ?? this.self,
      isLoading: isLoading ?? this.isLoading,
      error: clearError ? null : (error ?? this.error),
      snapshotId: snapshotId ?? this.snapshotId,
    );
  }
}
