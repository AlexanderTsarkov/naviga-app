import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:naviga_app/features/connect/connect_controller.dart';
import 'package:naviga_app/features/nodes/node_table_cache.dart';

NodeRecordV1 _record(int nodeId) {
  return NodeRecordV1(
    nodeId: nodeId,
    shortId: nodeId & 0xFFFF,
    isSelf: false,
    posValid: true,
    isGrey: false,
    shortIdCollision: false,
    lastSeenAgeS: 10,
    latE7: 123,
    lonE7: 456,
    posAgeS: 5,
    lastRxRssi: -50,
    lastSeq: 1,
  );
}

void main() {
  setUp(() {
    SharedPreferences.setMockInitialValues({});
  });

  test('cache roundtrip preserves snapshot data', () async {
    final prefs = await SharedPreferences.getInstance();
    final store = NodeTableCacheStore(prefs: prefs, nowMs: () => 1500);

    final snapshot = CachedNodeTableSnapshot(
      deviceId: 'dev-1',
      snapshotId: 7,
      savedAtMs: 1000,
      records: [_record(1), _record(2)],
    );

    await store.save(deviceId: 'dev-1', snapshot: snapshot);
    final restored = await store.restore('dev-1');

    expect(restored, isNotNull);
    expect(restored!.deviceId, 'dev-1');
    expect(restored.snapshotId, 7);
    expect(restored.records.length, 2);
    expect(restored.cacheAgeMs, 500);
  });

  test('cache expires after ttl and clears entry', () async {
    final prefs = await SharedPreferences.getInstance();
    final store = NodeTableCacheStore(
      prefs: prefs,
      ttlMs: 1000,
      nowMs: () => 5000,
    );

    final snapshot = CachedNodeTableSnapshot(
      deviceId: 'dev-2',
      snapshotId: 3,
      savedAtMs: 1000,
      records: [_record(1)],
    );

    await store.save(deviceId: 'dev-2', snapshot: snapshot);
    final restored = await store.restore('dev-2');

    expect(restored, isNull);
    final raw = prefs.getString('$kNodeTableCacheKeyPrefix.dev-2');
    expect(raw, isNull);
  });
}
