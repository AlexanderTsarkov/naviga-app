import 'dart:async';
import 'dart:typed_data';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../protocol/meshtastic_protocol.dart';
import '../database/meshtastic_database.dart';

/// Основной сервис для работы с Meshtastic устройствами
class MeshtasticService {
  // BLE UUIDs для Meshtastic
  static const String meshServiceUuid = '6ba1b218-15a8-461f-9fa8-5dcae273eafd';
  static const String fromRadioUuid = '2c55e69e-4993-11ed-b878-0242ac120002';
  static const String toRadioUuid = 'f75c76d2-129e-4dad-a1dd-7866124401e7';
  static const String fromNumUuid = 'ed9da18c-a800-4f66-a670-aa7547e34453';

  // BLE компоненты
  BluetoothDevice? _device;
  BluetoothCharacteristic? _fromRadio;
  BluetoothCharacteristic? _toRadio;
  BluetoothCharacteristic? _fromNum;
  StreamSubscription? _fromRadioSubscription;
  StreamSubscription? _fromNumSubscription;

  // Сервисы
  final MeshtasticProtocol _protocol = MeshtasticProtocol();
  final MeshtasticDatabase _database = MeshtasticDatabase();

  // Потоки данных
  final StreamController<Map<String, dynamic>> _gpsDataController =
      StreamController<Map<String, dynamic>>.broadcast();
  final StreamController<List<Map<String, dynamic>>> _meshDevicesController =
      StreamController<List<Map<String, dynamic>>>.broadcast();

  // Состояние подключения
  bool _isConnected = false;
  Timer? _periodicTimer;

  // Публичные потоки
  Stream<Map<String, dynamic>> get gpsDataStream => _gpsDataController.stream;
  Stream<List<Map<String, dynamic>>> get meshDevicesStream => _meshDevicesController.stream;
  bool get isConnected => _isConnected;

  /// Подключение к Meshtastic устройству
  Future<bool> connectToDevice(BluetoothDevice device) async {
    try {
      _device = device;
      
      // Подключение к устройству
      await device.connect();
      
      // Запрос увеличения MTU
      await device.requestMtu(512);
      
      // Поиск сервисов
      final services = await device.discoverServices();
      final meshService = services.firstWhere(
        (service) => service.uuid.toString().toLowerCase() == meshServiceUuid.toLowerCase(),
        orElse: () => throw Exception('Meshtastic service not found'),
      );
      
      // Поиск характеристик
      _fromRadio = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == fromRadioUuid.toLowerCase(),
      );
      _toRadio = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == toRadioUuid.toLowerCase(),
      );
      _fromNum = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == fromNumUuid.toLowerCase(),
      );
      
      // Подписка на уведомления
      await _fromNum!.setNotifyValue(true);
      
      // Настройка потоков данных
      _setupDataStreams();
      
      // Выполнение handshake
      await _performHandshake();
      
      _isConnected = true;
      return true;
      
    } catch (e) {
      _isConnected = false;
      return false;
    }
  }

  /// Отключение от устройства
  Future<void> disconnect() async {
    _isConnected = false;
    
    await _fromRadioSubscription?.cancel();
    await _fromNumSubscription?.cancel();
    _periodicTimer?.cancel();
    
    await _device?.disconnect();
    
    _device = null;
    _fromRadio = null;
    _toRadio = null;
    _fromNum = null;
  }

  /// Запрос позиции конкретного узла
  Future<void> requestPosition(int nodeNum) async {
    if (!_isConnected || _toRadio == null) return;
    
    final packet = _protocol.createPositionRequest(nodeNum);
    final toRadio = _protocol.createToRadioMessage(packet);
    
    await _toRadio!.write(toRadio.writeToBuffer());
  }

  /// Запрос телеметрии конкретного узла
  Future<void> requestTelemetry(int nodeNum) async {
    if (!_isConnected || _toRadio == null) return;
    
    final packet = _protocol.createTelemetryRequest(nodeNum);
    final toRadio = _protocol.createToRadioMessage(packet);
    
    await _toRadio!.write(toRadio.writeToBuffer());
  }

  /// Запрос информации о узле
  Future<void> requestNodeInfo(int nodeNum) async {
    if (!_isConnected || _toRadio == null) return;
    
    final packet = _protocol.createNodeInfoRequest(nodeNum);
    final toRadio = _protocol.createToRadioMessage(packet);
    
    await _toRadio!.write(toRadio.writeToBuffer());
  }

  /// Получение всех узлов из базы данных
  Future<List<Map<String, dynamic>>> getAllNodes() async {
    return await _database.getAllNodes();
  }

  /// Получение последних позиций
  Future<List<Map<String, dynamic>>> getLatestPositions() async {
    return await _database.getLatestPositions();
  }

  /// Получение последних метрик
  Future<List<Map<String, dynamic>>> getLatestMetrics() async {
    return await _database.getLatestMetrics();
  }

  /// Настройка потоков данных
  void _setupDataStreams() {
    // Подписка на FromNum уведомления
    _fromNumSubscription = _fromNum!.lastValueStream.listen(_handleFromNumNotification);
    
    // Периодическое чтение FromRadio
    _periodicTimer = Timer.periodic(const Duration(seconds: 2), (_) {
      if (_isConnected) _drainFromRadio();
    });
  }

  /// Обработка уведомлений FromNum
  void _handleFromNumNotification(Uint8List data) {
    _drainFromRadioUntilEmpty();
  }

  /// Чтение всех данных из FromRadio
  Future<void> _drainFromRadio() async {
    if (_fromRadio == null) return;
    
    try {
      final data = await _fromRadio!.read();
      if (data.isNotEmpty) {
        _processFromRadioData(data);
      }
    } catch (e) {
      // Игнорируем ошибки чтения
    }
  }

  /// Чтение всех данных из FromRadio до пустого ответа
  Future<void> _drainFromRadioUntilEmpty() async {
    if (_fromRadio == null) return;
    
    while (true) {
      try {
        final data = await _fromRadio!.read();
        if (data.isEmpty) break;
        _processFromRadioData(data);
      } catch (e) {
        break;
      }
    }
  }

  /// Обработка данных от FromRadio
  void _processFromRadioData(Uint8List data) {
    try {
      final fromRadio = _protocol.parseFromRadio(data);
      _handleFromRadio(fromRadio);
    } catch (e) {
      // Игнорируем ошибки парсинга
    }
  }

  /// Обработка FromRadio сообщений
  void _handleFromRadio(dynamic fromRadio) {
    if (fromRadio.hasMyInfo()) {
      _handleMyInfo(fromRadio.myInfo);
    }
    
    if (fromRadio.hasNodeInfo()) {
      _handleNodeInfo(fromRadio.nodeInfo);
    }
    
    if (fromRadio.hasPacket()) {
      _handleMeshPacket(fromRadio.packet);
    }
  }

  /// Обработка информации о подключенном устройстве
  void _handleMyInfo(dynamic myInfo) {
    // Сохраняем информацию о подключенном устройстве
    _database.saveNodeInfo(
      nodeNum: myInfo.myNodeNum,
      longName: 'Connected Device',
      shortName: 'DEV',
      hwModel: 'T-Beam',
      role: 'Client',
    );
  }

  /// Обработка информации о узле
  void _handleNodeInfo(dynamic nodeInfo) {
    _database.saveNodeInfo(
      nodeNum: nodeInfo.num,
      longName: nodeInfo.user?.longName,
      shortName: nodeInfo.user?.shortName,
      macaddr: nodeInfo.user?.macaddr,
      hwModel: nodeInfo.user?.hwModel,
      role: nodeInfo.user?.role,
    );
    
    // Если есть позиция в NodeInfo, сохраняем её
    if (nodeInfo.hasPosition()) {
      final position = nodeInfo.position;
      _database.savePosition(
        nodeNum: nodeInfo.num,
        timestamp: position.hasTime() ? position.time : DateTime.now().millisecondsSinceEpoch ~/ 1000,
        latitude: position.latitudeI / 10000000.0,
        longitude: position.longitudeI / 10000000.0,
        altitude: position.hasAltitude() ? position.altitude.toInt() : null,
        speedMs: position.hasGroundSpeed() ? position.groundSpeed.toDouble() : null,
        trackDeg: position.hasGroundTrack() ? position.groundTrack.toDouble() : null,
        precisionBits: position.hasPrecisionBits() ? position.precisionBits : null,
      );
    }
  }

  /// Обработка MeshPacket
  void _handleMeshPacket(dynamic packet) {
    if (!packet.hasDecoded()) return;
    
    final data = packet.decoded;
    final portnum = data.portnum;
    
    switch (portnum) {
      case 3: // POSITION_APP
        _handlePositionData(data.payload, packet.from);
        break;
      case 67: // TELEMETRY_APP
        _handleTelemetryData(data.payload, packet.from);
        break;
      case 6: // NODEINFO_APP
        _handleNodeInfoData(data.payload, packet.from);
        break;
    }
  }

  /// Обработка данных позиции
  void _handlePositionData(Uint8List payload, int nodeNum) {
    try {
      final position = _protocol.parsePosition(payload);
      if (position.hasLatitudeI() && position.hasLongitudeI()) {
        _database.savePosition(
          nodeNum: nodeNum,
          timestamp: position.hasTime() ? position.time : DateTime.now().millisecondsSinceEpoch ~/ 1000,
          latitude: position.latitudeI / 10000000.0,
          longitude: position.longitudeI / 10000000.0,
          altitude: position.hasAltitude() ? position.altitude.toInt() : null,
          speedMs: position.hasGroundSpeed() ? position.groundSpeed.toDouble() : null,
          trackDeg: position.hasGroundTrack() ? position.groundTrack.toDouble() : null,
          precisionBits: position.hasPrecisionBits() ? position.precisionBits : null,
        );
        
        // Отправляем в поток
        _gpsDataController.add({
          'nodeNum': nodeNum,
          'latitude': position.latitudeI / 10000000.0,
          'longitude': position.longitudeI / 10000000.0,
          'timestamp': position.hasTime() ? position.time : DateTime.now().millisecondsSinceEpoch ~/ 1000,
        });
      }
    } catch (e) {
      // Игнорируем ошибки парсинга
    }
  }

  /// Обработка телеметрии
  void _handleTelemetryData(Uint8List payload, int nodeNum) {
    try {
      final telemetry = _protocol.parseTelemetry(payload);
      if (telemetry.hasDeviceMetrics()) {
        final metrics = telemetry.deviceMetrics;
        _database.saveDeviceMetrics(
          nodeNum: nodeNum,
          timestamp: DateTime.now().millisecondsSinceEpoch ~/ 1000,
          batteryLevel: metrics.hasBatteryLevel() ? metrics.batteryLevel : null,
          voltage: metrics.hasVoltage() ? metrics.voltage : null,
          channelUtil: metrics.hasChannelUtilization() ? metrics.channelUtilization : null,
          airUtilTx: metrics.hasAirUtilTx() ? metrics.airUtilTx : null,
          uptimeSeconds: metrics.hasUptimeSeconds() ? metrics.uptimeSeconds : null,
        );
      }
    } catch (e) {
      // Игнорируем ошибки парсинга
    }
  }

  /// Обработка информации о узле
  void _handleNodeInfoData(Uint8List payload, int nodeNum) {
    try {
      final user = _protocol.parseUser(payload);
      _database.saveNodeInfo(
        nodeNum: nodeNum,
        longName: user.longName,
        shortName: user.shortName,
        macaddr: user.macaddr,
        hwModel: user.hwModel,
        role: user.role,
      );
    } catch (e) {
      // Игнорируем ошибки парсинга
    }
  }

  /// Выполнение handshake с устройством
  Future<void> _performHandshake() async {
    if (_toRadio == null) return;
    
    // Отправляем want_config_id
    final toRadio = _protocol.createWantConfigIdRequest();
    await _toRadio!.write(toRadio.writeToBuffer());
    
    // Ждем и читаем все данные
    await Future.delayed(const Duration(milliseconds: 500));
    await _drainFromRadioUntilEmpty();
    
    // Запрашиваем позиции всех известных узлов
    await Future.delayed(const Duration(seconds: 1));
    await _requestAllKnownPositions();
  }

  /// Запрос позиций всех известных узлов
  Future<void> _requestAllKnownPositions() async {
    final nodes = await _database.getAllNodes();
    for (final node in nodes) {
      final nodeNum = node['node_num'] as int;
      await requestPosition(nodeNum);
      await Future.delayed(const Duration(milliseconds: 200));
    }
  }

  /// Освобождение ресурсов
  void dispose() {
    disconnect();
    _gpsDataController.close();
    _meshDevicesController.close();
    _database.close();
  }
}
