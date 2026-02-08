import 'dart:convert';

import 'package:shared_preferences/shared_preferences.dart';

import '../../shared/logging.dart';
import '../connect/connect_controller.dart';

typedef NowMsProvider = int Function();

const int kNodeTableCacheTtlMs = 10 * 60 * 1000;
const String kNodeTableCacheKeyPrefix = 'naviga.node_table_cache';

class NodeTableCacheStore {
  NodeTableCacheStore({
    SharedPreferences? prefs,
    int ttlMs = kNodeTableCacheTtlMs,
    NowMsProvider? nowMs,
  }) : _prefsFuture = prefs != null
           ? Future.value(prefs)
           : SharedPreferences.getInstance(),
       _ttlMs = ttlMs,
       _nowMs = nowMs ?? _defaultNowMs;

  final Future<SharedPreferences?> _prefsFuture;
  final int _ttlMs;
  final NowMsProvider _nowMs;

  static int _defaultNowMs() => DateTime.now().millisecondsSinceEpoch;

  String _key(String deviceId) => '$kNodeTableCacheKeyPrefix.$deviceId';

  Future<void> save({
    required String deviceId,
    required CachedNodeTableSnapshot snapshot,
  }) async {
    final prefs = await _prefsFuture;
    if (prefs == null) {
      return;
    }
    final payload = jsonEncode(snapshot.toJson());
    await prefs.setString(_key(deviceId), payload);
  }

  Future<CachedNodeTableSnapshot?> restore(String deviceId) async {
    final prefs = await _prefsFuture;
    if (prefs == null) {
      return null;
    }
    final raw = prefs.getString(_key(deviceId));
    if (raw == null) {
      return null;
    }
    try {
      final decoded = jsonDecode(raw);
      if (decoded is! Map<String, dynamic>) {
        throw const FormatException('Cache payload is not a map');
      }
      final snapshot = CachedNodeTableSnapshot.fromJson(decoded);
      final ageMs = _nowMs() - snapshot.savedAtMs;
      if (ageMs > _ttlMs) {
        logInfo(
          'NodesCache: expired deviceId=$deviceId ageMs=$ageMs ttlMs=$_ttlMs',
        );
        await prefs.remove(_key(deviceId));
        return null;
      }
      return snapshot.withAge(ageMs < 0 ? 0 : ageMs);
    } catch (error) {
      logInfo('NodesCache: restore failed deviceId=$deviceId error=$error');
      await prefs.remove(_key(deviceId));
      return null;
    }
  }

  Future<void> clear(String deviceId) async {
    final prefs = await _prefsFuture;
    if (prefs == null) {
      return;
    }
    await prefs.remove(_key(deviceId));
  }
}

class CachedNodeTableSnapshot {
  const CachedNodeTableSnapshot({
    required this.deviceId,
    required this.snapshotId,
    required this.savedAtMs,
    required this.records,
    this.cacheAgeMs = 0,
  });

  final String deviceId;
  final int? snapshotId;
  final int savedAtMs;
  final List<NodeRecordV1> records;
  final int cacheAgeMs;

  CachedNodeTableSnapshot withAge(int ageMs) {
    return CachedNodeTableSnapshot(
      deviceId: deviceId,
      snapshotId: snapshotId,
      savedAtMs: savedAtMs,
      records: records,
      cacheAgeMs: ageMs,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'deviceId': deviceId,
      'snapshotId': snapshotId,
      'savedAtMs': savedAtMs,
      'records': records.map((record) => record.toJson()).toList(),
    };
  }

  static CachedNodeTableSnapshot fromJson(Map<String, dynamic> json) {
    return CachedNodeTableSnapshot(
      deviceId: json['deviceId'] as String,
      snapshotId: (json['snapshotId'] as num?)?.toInt(),
      savedAtMs: (json['savedAtMs'] as num).toInt(),
      records: (json['records'] as List<dynamic>)
          .map(
            (raw) =>
                NodeRecordV1JsonParser.fromJson(raw as Map<String, dynamic>),
          )
          .toList(),
    );
  }
}

extension NodeRecordV1JsonCodec on NodeRecordV1 {
  Map<String, dynamic> toJson() {
    return {
      'nodeId': nodeId,
      'shortId': shortId,
      'isSelf': isSelf,
      'posValid': posValid,
      'isGrey': isGrey,
      'shortIdCollision': shortIdCollision,
      'lastSeenAgeS': lastSeenAgeS,
      'latE7': latE7,
      'lonE7': lonE7,
      'posAgeS': posAgeS,
      'lastRxRssi': lastRxRssi,
      'lastSeq': lastSeq,
    };
  }
}

class NodeRecordV1JsonParser {
  static NodeRecordV1 fromJson(Map<String, dynamic> json) {
    return NodeRecordV1(
      nodeId: (json['nodeId'] as num).toInt(),
      shortId: (json['shortId'] as num).toInt(),
      isSelf: json['isSelf'] as bool,
      posValid: json['posValid'] as bool,
      isGrey: json['isGrey'] as bool,
      shortIdCollision: json['shortIdCollision'] as bool,
      lastSeenAgeS: (json['lastSeenAgeS'] as num).toInt(),
      latE7: (json['latE7'] as num).toInt(),
      lonE7: (json['lonE7'] as num).toInt(),
      posAgeS: (json['posAgeS'] as num).toInt(),
      lastRxRssi: (json['lastRxRssi'] as num).toInt(),
      lastSeq: (json['lastSeq'] as num).toInt(),
    );
  }
}
