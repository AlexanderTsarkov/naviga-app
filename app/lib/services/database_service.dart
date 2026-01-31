import 'dart:async';
import 'dart:typed_data';
import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:path_provider/path_provider.dart';

class DatabaseService {
  static Database? _database;
  static const String _databaseName = 'naviga_mesh.db';
  static const int _databaseVersion = 1;

  // Таблицы
  static const String _positionLogTable = 'position_log';
  static const String _deviceMetricsTable = 'device_metrics_log';
  static const String _nodeInfoTable = 'node_info';

  Future<Database> get database async {
    if (_database != null) return _database!;
    _database = await _initDatabase();
    return _database!;
  }

  Future<Database> _initDatabase() async {
    final documentsDirectory = await getApplicationDocumentsDirectory();
    final path = join(documentsDirectory.path, _databaseName);
    
    return await openDatabase(
      path,
      version: _databaseVersion,
      onCreate: _onCreate,
    );
  }

  Future<void> _onCreate(Database db, int version) async {
    // Таблица позиций
    await db.execute('''
      CREATE TABLE $_positionLogTable (
        node_num INTEGER NOT NULL,
        t INTEGER NOT NULL,
        lat REAL,
        lon REAL,
        alt_m INTEGER,
        speed_ms REAL,
        track_deg REAL,
        precision_bits INTEGER,
        raw BLOB,
        PRIMARY KEY (node_num, t)
      )
    ''');

    // Таблица метрик устройств
    await db.execute('''
      CREATE TABLE $_deviceMetricsTable (
        node_num INTEGER NOT NULL,
        t INTEGER NOT NULL,
        battery_level REAL,
        voltage REAL,
        channel_util REAL,
        air_util_tx REAL,
        uptime_s INTEGER,
        raw BLOB,
        PRIMARY KEY (node_num, t)
      )
    ''');

    // Таблица информации о узлах
    await db.execute('''
      CREATE TABLE $_nodeInfoTable (
        node_num INTEGER PRIMARY KEY,
        long_name TEXT,
        short_name TEXT,
        macaddr TEXT,
        hw_model TEXT,
        role TEXT,
        last_seen INTEGER,
        created_at INTEGER
      )
    ''');
  }

  /// Сохраняет позицию узла
  Future<void> savePosition({
    required int nodeNum,
    required int timestamp,
    double? latitude,
    double? longitude,
    int? altitude,
    double? speedMs,
    double? trackDeg,
    int? precisionBits,
    Uint8List? rawData,
  }) async {
    final db = await database;
    
    try {
      await db.insert(
        _positionLogTable,
        {
          'node_num': nodeNum,
          't': timestamp,
          'lat': latitude,
          'lon': longitude,
          'alt_m': altitude,
          'speed_ms': speedMs,
          'track_deg': trackDeg,
          'precision_bits': precisionBits,
          'raw': rawData,
        },
        conflictAlgorithm: ConflictAlgorithm.replace,
      );
      
      print('💾 Сохранена позиция узла $nodeNum: $latitude, $longitude');
    } catch (e) {
      print('❌ Ошибка сохранения позиции: $e');
    }
  }

  /// Сохраняет метрики устройства
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
    
    try {
      await db.insert(
        _deviceMetricsTable,
        {
          'node_num': nodeNum,
          't': timestamp,
          'battery_level': batteryLevel,
          'voltage': voltage,
          'channel_util': channelUtil,
          'air_util_tx': airUtilTx,
          'uptime_s': uptimeSeconds,
          'raw': rawData,
        },
        conflictAlgorithm: ConflictAlgorithm.replace,
      );
      
      print('💾 Сохранены метрики узла $nodeNum: батарея ${batteryLevel?.toStringAsFixed(1)}%, напряжение ${voltage?.toStringAsFixed(2)}V');
    } catch (e) {
      print('❌ Ошибка сохранения метрик: $e');
    }
  }

  /// Сохраняет информацию о узле
  Future<void> saveNodeInfo({
    required int nodeNum,
    String? longName,
    String? shortName,
    String? macaddr,
    String? hwModel,
    String? role,
  }) async {
    final db = await database;
    final now = DateTime.now().millisecondsSinceEpoch ~/ 1000;
    
    try {
      await db.insert(
        _nodeInfoTable,
        {
          'node_num': nodeNum,
          'long_name': longName,
          'short_name': shortName,
          'macaddr': macaddr,
          'hw_model': hwModel,
          'role': role,
          'last_seen': now,
          'created_at': now,
        },
        conflictAlgorithm: ConflictAlgorithm.replace,
      );
      
      print('💾 Сохранена информация о узле $nodeNum: $longName ($shortName)');
    } catch (e) {
      print('❌ Ошибка сохранения информации о узле: $e');
    }
  }

  /// Получает все известные узлы
  Future<List<Map<String, dynamic>>> getAllNodes() async {
    final db = await database;
    
    try {
      final result = await db.query(
        _nodeInfoTable,
        orderBy: 'last_seen DESC',
      );
      
      print('📊 Загружено ${result.length} узлов из БД');
      return result;
    } catch (e) {
      print('❌ Ошибка загрузки узлов: $e');
      return [];
    }
  }

  /// Получает последние позиции всех узлов
  Future<List<Map<String, dynamic>>> getLatestPositions() async {
    final db = await database;
    
    try {
      final result = await db.rawQuery('''
        SELECT p.*, n.long_name, n.short_name
        FROM $_positionLogTable p
        INNER JOIN (
          SELECT node_num, MAX(t) as max_t
          FROM $_positionLogTable
          GROUP BY node_num
        ) latest ON p.node_num = latest.node_num AND p.t = latest.max_t
        LEFT JOIN $_nodeInfoTable n ON p.node_num = n.node_num
        ORDER BY p.t DESC
      ''');
      
      print('📍 Загружено ${result.length} последних позиций');
      return result;
    } catch (e) {
      print('❌ Ошибка загрузки позиций: $e');
      return [];
    }
  }

  /// Получает последние метрики всех узлов
  Future<List<Map<String, dynamic>>> getLatestMetrics() async {
    final db = await database;
    
    try {
      final result = await db.rawQuery('''
        SELECT m.*, n.long_name, n.short_name
        FROM $_deviceMetricsTable m
        INNER JOIN (
          SELECT node_num, MAX(t) as max_t
          FROM $_deviceMetricsTable
          GROUP BY node_num
        ) latest ON m.node_num = latest.node_num AND m.t = latest.max_t
        LEFT JOIN $_nodeInfoTable n ON m.node_num = n.node_num
        ORDER BY m.t DESC
      ''');
      
      print('📊 Загружено ${result.length} последних метрик');
      return result;
    } catch (e) {
      print('❌ Ошибка загрузки метрик: $e');
      return [];
    }
  }

  /// Очищает старые данные (старше 7 дней)
  Future<void> cleanupOldData() async {
    final db = await database;
    final weekAgo = DateTime.now().subtract(const Duration(days: 7)).millisecondsSinceEpoch ~/ 1000;
    
    try {
      final positionCount = await db.delete(
        _positionLogTable,
        where: 't < ?',
        whereArgs: [weekAgo],
      );
      
      final metricsCount = await db.delete(
        _deviceMetricsTable,
        where: 't < ?',
        whereArgs: [weekAgo],
      );
      
      print('🧹 Очищено $positionCount позиций и $metricsCount метрик старше недели');
    } catch (e) {
      print('❌ Ошибка очистки данных: $e');
    }
  }

  /// Закрывает соединение с БД
  Future<void> close() async {
    if (_database != null) {
      await _database!.close();
      _database = null;
    }
  }
}
