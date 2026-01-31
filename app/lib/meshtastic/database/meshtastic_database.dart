import 'dart:typed_data';
import 'package:path/path.dart';
import 'package:sqflite/sqflite.dart';
import 'package:synchronized/synchronized.dart';

/// Модуль для работы с базой данных Meshtastic
class MeshtasticDatabase {
  static Database? _database;
  static final _lock = Lock();

  /// Получение экземпляра базы данных
  Future<Database> get database async {
    if (_database != null) return _database!;
    await _lock.synchronized(() async {
      if (_database == null) {
        _database = await _initDB();
      }
    });
    return _database!;
  }

  /// Инициализация базы данных
  Future<Database> _initDB() async {
    final databasePath = await getDatabasesPath();
    final path = join(databasePath, 'meshtastic.db');

    return await openDatabase(
      path,
      version: 1,
      onCreate: (db, version) async {
        await _createTables(db);
      },
      onUpgrade: (db, oldVersion, newVersion) async {
        // Обработка обновлений схемы в будущем
      },
    );
  }

  /// Создание таблиц
  Future<void> _createTables(Database db) async {
    // Таблица информации о узлах
    await db.execute('''
      CREATE TABLE node_info (
        node_num INTEGER PRIMARY KEY,
        long_name TEXT,
        short_name TEXT,
        macaddr TEXT,
        hw_model TEXT,
        role TEXT,
        last_seen INTEGER
      )
    ''');

    // Таблица логов позиций
    await db.execute('''
      CREATE TABLE position_log (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        node_num INTEGER,
        timestamp INTEGER,
        latitude REAL,
        longitude REAL,
        altitude INTEGER,
        speed_ms REAL,
        track_deg REAL,
        precision_bits INTEGER,
        raw_data BLOB,
        FOREIGN KEY (node_num) REFERENCES node_info (node_num)
      )
    ''');

    // Таблица логов телеметрии
    await db.execute('''
      CREATE TABLE device_metrics_log (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        node_num INTEGER,
        timestamp INTEGER,
        battery_level REAL,
        voltage REAL,
        channel_util REAL,
        air_util_tx REAL,
        uptime_seconds INTEGER,
        raw_data BLOB,
        FOREIGN KEY (node_num) REFERENCES node_info (node_num)
      )
    ''');

    // Таблица геообъектов
    await db.execute('''
      CREATE TABLE geo_objects (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        node_num INTEGER,
        timestamp INTEGER,
        latitude REAL,
        longitude REAL,
        name TEXT,
        description TEXT,
        object_type TEXT,
        raw_data BLOB,
        FOREIGN KEY (node_num) REFERENCES node_info (node_num)
      )
    ''');
  }

  /// Сохранение информации о узле
  Future<void> saveNodeInfo({
    required int nodeNum,
    String? longName,
    String? shortName,
    String? macaddr,
    String? hwModel,
    String? role,
  }) async {
    final db = await database;
    await db.insert(
      'node_info',
      {
        'node_num': nodeNum,
        'long_name': longName,
        'short_name': shortName,
        'macaddr': macaddr,
        'hw_model': hwModel,
        'role': role,
        'last_seen': DateTime.now().millisecondsSinceEpoch ~/ 1000,
      },
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
  }

  /// Сохранение позиции
  Future<void> savePosition({
    required int nodeNum,
    required int timestamp,
    required double latitude,
    required double longitude,
    int? altitude,
    double? speedMs,
    double? trackDeg,
    int? precisionBits,
    Uint8List? rawData,
  }) async {
    final db = await database;
    await db.insert(
      'position_log',
      {
        'node_num': nodeNum,
        'timestamp': timestamp,
        'latitude': latitude,
        'longitude': longitude,
        'altitude': altitude,
        'speed_ms': speedMs,
        'track_deg': trackDeg,
        'precision_bits': precisionBits,
        'raw_data': rawData,
      },
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
    _cleanOldData();
  }

  /// Сохранение телеметрии
  Future<void> saveDeviceMetrics({
    required int nodeNum,
    required int timestamp,
    double? batteryLevel,
    double? voltage,
    double? channelUtil,
    double? airUtilTx,
    int? uptimeSeconds,
    Uint8List? rawData,
  }) async {
    final db = await database;
    await db.insert(
      'device_metrics_log',
      {
        'node_num': nodeNum,
        'timestamp': timestamp,
        'battery_level': batteryLevel,
        'voltage': voltage,
        'channel_util': channelUtil,
        'air_util_tx': airUtilTx,
        'uptime_seconds': uptimeSeconds,
        'raw_data': rawData,
      },
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
    _cleanOldData();
  }

  /// Сохранение геообъекта
  Future<void> saveGeoObject({
    required int nodeNum,
    required int timestamp,
    required double latitude,
    required double longitude,
    required String name,
    required String description,
    String? objectType,
    Uint8List? rawData,
  }) async {
    final db = await database;
    await db.insert(
      'geo_objects',
      {
        'node_num': nodeNum,
        'timestamp': timestamp,
        'latitude': latitude,
        'longitude': longitude,
        'name': name,
        'description': description,
        'object_type': objectType,
        'raw_data': rawData,
      },
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
  }

  /// Получение всех узлов
  Future<List<Map<String, dynamic>>> getAllNodes() async {
    final db = await database;
    return await db.query('node_info');
  }

  /// Получение последних позиций всех узлов
  Future<List<Map<String, dynamic>>> getLatestPositions() async {
    final db = await database;
    return await db.rawQuery('''
      SELECT pl.*
      FROM position_log pl
      INNER JOIN (
        SELECT node_num, MAX(timestamp) as max_timestamp
        FROM position_log
        GROUP BY node_num
      ) AS latest_positions
      ON pl.node_num = latest_positions.node_num AND pl.timestamp = latest_positions.max_timestamp
    ''');
  }

  /// Получение последних метрик всех узлов
  Future<List<Map<String, dynamic>>> getLatestMetrics() async {
    final db = await database;
    return await db.rawQuery('''
      SELECT dml.*
      FROM device_metrics_log dml
      INNER JOIN (
        SELECT node_num, MAX(timestamp) as max_timestamp
        FROM device_metrics_log
        GROUP BY node_num
      ) AS latest_metrics
      ON dml.node_num = latest_metrics.node_num AND dml.timestamp = latest_metrics.max_timestamp
    ''');
  }

  /// Получение всех геообъектов
  Future<List<Map<String, dynamic>>> getAllGeoObjects() async {
    final db = await database;
    return await db.query('geo_objects', orderBy: 'timestamp DESC');
  }

  /// Получение позиций конкретного узла
  Future<List<Map<String, dynamic>>> getNodePositions(int nodeNum, {int? limit}) async {
    final db = await database;
    return await db.query(
      'position_log',
      where: 'node_num = ?',
      whereArgs: [nodeNum],
      orderBy: 'timestamp DESC',
      limit: limit,
    );
  }

  /// Получение метрик конкретного узла
  Future<List<Map<String, dynamic>>> getNodeMetrics(int nodeNum, {int? limit}) async {
    final db = await database;
    return await db.query(
      'device_metrics_log',
      where: 'node_num = ?',
      whereArgs: [nodeNum],
      orderBy: 'timestamp DESC',
      limit: limit,
    );
  }

  /// Получение статистики по узлу
  Future<Map<String, dynamic>> getNodeStats(int nodeNum) async {
    final db = await database;
    
    // Количество позиций
    final positionCount = Sqflite.firstIntValue(await db.rawQuery(
      'SELECT COUNT(*) FROM position_log WHERE node_num = ?',
      [nodeNum],
    )) ?? 0;
    
    // Количество метрик
    final metricsCount = Sqflite.firstIntValue(await db.rawQuery(
      'SELECT COUNT(*) FROM device_metrics_log WHERE node_num = ?',
      [nodeNum],
    )) ?? 0;
    
    // Последняя активность
    final lastActivity = Sqflite.firstIntValue(await db.rawQuery(
      '''
      SELECT MAX(timestamp) FROM (
        SELECT timestamp FROM position_log WHERE node_num = ?
        UNION
        SELECT timestamp FROM device_metrics_log WHERE node_num = ?
      )
      ''',
      [nodeNum, nodeNum],
    )) ?? 0;
    
    return {
      'position_count': positionCount,
      'metrics_count': metricsCount,
      'last_activity': lastActivity,
    };
  }

  /// Очистка старых данных (старше 7 дней)
  Future<void> _cleanOldData() async {
    final db = await database;
    final sevenDaysAgo = DateTime.now().subtract(const Duration(days: 7)).millisecondsSinceEpoch ~/ 1000;
    
    await db.delete('position_log', where: 'timestamp < ?', whereArgs: [sevenDaysAgo]);
    await db.delete('device_metrics_log', where: 'timestamp < ?', whereArgs: [sevenDaysAgo]);
    await db.delete('geo_objects', where: 'timestamp < ?', whereArgs: [sevenDaysAgo]);
  }

  /// Очистка всех данных
  Future<void> clearAllData() async {
    final db = await database;
    await db.delete('position_log');
    await db.delete('device_metrics_log');
    await db.delete('geo_objects');
    await db.delete('node_info');
  }

  /// Закрытие базы данных
  Future<void> close() async {
    final db = await database;
    await db.close();
    _database = null;
  }
}
