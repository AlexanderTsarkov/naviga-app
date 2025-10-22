import 'dart:async';
import 'dart:typed_data';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../generated/meshtastic/mesh.pb.dart';
import '../generated/meshtastic/admin.pb.dart';
import '../generated/meshtastic/portnums.pbenum.dart';
import '../generated/meshtastic/telemetry.pb.dart';
import 'database_service.dart';

class MeshtasticBluetoothService {
  static const String meshServiceUuid = '6ba1b218-15a8-461f-9fa8-5dcae273eafd';
  static const String fromRadioUuid = '2c55e69e-4993-11ed-b878-0242ac120002';
  static const String toRadioUuid = 'f75c76d2-129e-4dad-a1dd-7866124401e7';
  static const String fromNumUuid = 'ed9da18c-a800-4f66-a670-aa7547e34453';

  BluetoothDevice? _device;
  BluetoothCharacteristic? _fromRadio;
  BluetoothCharacteristic? _toRadio;
  BluetoothCharacteristic? _fromNum;
  StreamSubscription? _fromRadioSubscription;
  StreamSubscription? _fromNumSubscription;

  // Потоки для данных
  final StreamController<Map<String, dynamic>> _gpsDataController =
      StreamController<Map<String, dynamic>>.broadcast();
  final StreamController<List<Map<String, dynamic>>> _meshDevicesController =
      StreamController<List<Map<String, dynamic>>>.broadcast();
  
  final DatabaseService _databaseService = DatabaseService();
  
  // Автоматический режим тестирования
  bool _autoTestMode = true;
  Timer? _autoTestTimer;
  int _testStep = 0;
  final List<String> _testResults = [];

  Stream<Map<String, dynamic>> get gpsDataStream => _gpsDataController.stream;
  Stream<List<Map<String, dynamic>>> get meshDevicesStream => _meshDevicesController.stream;

  /// Получает все известные узлы из БД
  Future<List<Map<String, dynamic>>> getAllNodes() async {
    return await _databaseService.getAllNodes();
  }

  /// Получает последние позиции всех узлов
  Future<List<Map<String, dynamic>>> getLatestPositions() async {
    return await _databaseService.getLatestPositions();
  }

  /// Получает последние метрики всех узлов
  Future<List<Map<String, dynamic>>> getLatestMetrics() async {
    return await _databaseService.getLatestMetrics();
  }

  /// Запрашивает позицию конкретного узла
  Future<void> requestPosition(int nodeNum) async {
    if (_toRadio == null) {
      print('❌ ToRadio не инициализирован');
      return;
    }

    try {
      print('📍 Запрашиваем позицию узла $nodeNum...');
      
      // ИСПРАВЛЕНИЕ: Создаем пустой Position как запрос (как рекомендует ChatGPT)
      final queryPos = Position(); // без полей - это запрос позиции
      
      // Создаем Data с запросом позиции
      final data = Data()
        ..portnum = PortNum.POSITION_APP
        ..payload = queryPos.writeToBuffer() // Пустой Position как запрос
        ..wantResponse = true;
      
      // Создаем MeshPacket для отправки конкретному узлу
      final packet = MeshPacket()
        ..from = 0 // заполняется прошивкой
        ..to = nodeNum
        ..decoded = data
        ..hopLimit = 3; // или из конфигурации сети
      
      // Создаем ToRadio сообщение
      final toRadio = ToRadio()..packet = packet;
      
      // Отправляем запрос
      await _toRadio!.write(toRadio.writeToBuffer());
      print('✅ Запрос позиции отправлен узлу $nodeNum');
      
    } catch (e) {
      print('❌ Ошибка запроса позиции узла $nodeNum: $e');
    }
  }

  /// Запрашивает телеметрию конкретного узла
  Future<void> requestTelemetry(int nodeNum) async {
    if (_toRadio == null) {
      print('❌ ToRadio не инициализирован');
      return;
    }

    try {
      print('📊 Запрашиваем телеметрию узла $nodeNum...');
      
      // Создаем пустой DeviceMetrics для запроса
      final telemetryReq = Telemetry()
        ..deviceMetrics = DeviceMetrics();
      
      // Создаем Data с запросом телеметрии
      final data = Data()
        ..portnum = PortNum.TELEMETRY_APP
        ..payload = telemetryReq.writeToBuffer()
        ..wantResponse = true;
      
      // Создаем MeshPacket для отправки конкретному узлу
      final packet = MeshPacket()
        ..to = nodeNum
        ..channel = 0 // primary channel
        ..decoded = data;
      
      // Создаем ToRadio сообщение
      final toRadio = ToRadio()..packet = packet;
      
      // Отправляем запрос
      await _toRadio!.write(toRadio.writeToBuffer());
      print('✅ Запрос телеметрии отправлен узлу $nodeNum');
      
    } catch (e) {
      print('❌ Ошибка запроса телеметрии узла $nodeNum: $e');
    }
  }

  /// Запрашивает позиции всех известных узлов
  Future<void> requestAllPositions() async {
    try {
      final nodes = await getAllNodes();
      print('📍 Запрашиваем позиции ${nodes.length} узлов...');
      
      for (final node in nodes) {
        final nodeNum = node['node_num'] as int;
        await requestPosition(nodeNum);
        // Также запрашиваем NodeInfo - там тоже могут быть координаты
        await requestNodeInfo(nodeNum);
        // Пока убираем ADMIN запрос - сосредоточимся на ROUTING_APP
        // Небольшая задержка между запросами
        await Future.delayed(const Duration(milliseconds: 500));
      }
      
      print('✅ Запросы позиций отправлены всем узлам');
    } catch (e) {
      print('❌ Ошибка запроса позиций всех узлов: $e');
    }
  }


  /// Запрашивает информацию о узле (NodeInfo)
  Future<void> requestNodeInfo(int nodeNum) async {
    if (_toRadio == null) {
      print('❌ ToRadio не инициализирован');
      return;
    }

    try {
      print('👤 Запрашиваем NodeInfo узла $nodeNum...');
      
      // Создаем пустой User для запроса
      final userReq = User();
      
      // Создаем Data с запросом NodeInfo
      final data = Data()
        ..portnum = PortNum.NODEINFO_APP
        ..payload = userReq.writeToBuffer()
        ..wantResponse = true;
      
      // Создаем MeshPacket для отправки конкретному узлу
      final packet = MeshPacket()
        ..to = nodeNum
        ..channel = 0 // primary channel
        ..decoded = data;
      
      // Создаем ToRadio сообщение
      final toRadio = ToRadio()..packet = packet;
      
      // Отправляем запрос
      await _toRadio!.write(toRadio.writeToBuffer());
      print('✅ Запрос NodeInfo отправлен узлу $nodeNum');
      
    } catch (e) {
      print('❌ Ошибка запроса NodeInfo узла $nodeNum: $e');
    }
  }

  Future<bool> connectToDevice(BluetoothDevice device) async {
    try {
      print('=== НАЧАЛО ПОДКЛЮЧЕНИЯ К T-BEAM ===');
      print('Устройство: ${device.remoteId.str}');
      _device = device;
      
      // Подключаемся к устройству
      print('Шаг 1: Подключение к устройству...');
      await device.connect();
      print('✅ Подключение успешно!');
      
      // Устанавливаем MTU размер
      print('Шаг 2: Установка MTU размера...');
      await device.requestMtu(512);
      print('✅ MTU установлен!');
      
      // Получаем сервисы
      print('Шаг 3: Поиск сервисов...');
      final services = await device.discoverServices();
      print('Найдено сервисов: ${services.length}');
      
      for (var service in services) {
        print('Сервис: ${service.uuid}');
      }
      
      final meshService = services.firstWhere(
        (service) => service.uuid.toString().toLowerCase() == meshServiceUuid.toLowerCase(),
        orElse: () => throw Exception('MeshBluetoothService не найден'),
      );
      print('✅ MeshBluetoothService найден!');

      // Получаем характеристики
      print('Шаг 4: Поиск характеристик...');
      _fromRadio = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == fromRadioUuid.toLowerCase(),
      );
      print('✅ FromRadio найден: ${_fromRadio!.uuid}');
      
      _toRadio = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == toRadioUuid.toLowerCase(),
      );
      print('✅ ToRadio найден: ${_toRadio!.uuid}');
      
      _fromNum = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == fromNumUuid.toLowerCase(),
      );
      print('✅ FromNum найден: ${_fromNum!.uuid}');

      // Подписываемся на уведомления
      print('Шаг 5: Подписка на уведомления...');
      await _fromNum!.setNotifyValue(true);
      print('✅ Подписка на FromNum активна!');
      
      // Настраиваем потоки данных
      print('Шаг 6: Настройка потоков данных...');
      _setupDataStreams();
      print('✅ Потоки данных настроены!');
      
      // Отправляем startConfig
      print('Шаг 7: Отправка startConfig...');
      await _sendStartConfig();
      print('✅ startConfig отправлен!');
      
      print('=== ПОДКЛЮЧЕНИЕ ЗАВЕРШЕНО УСПЕШНО ===');
      return true;
    } catch (e) {
      print('❌ ОШИБКА ПОДКЛЮЧЕНИЯ: $e');
      return false;
    }
  }

  void _setupDataStreams() {
    print('=== НАСТРОЙКА ПОТОКОВ ДАННЫХ ===');
    
    // Подписываемся на FromRadio данные
    print('Подписка на FromRadio...');
    _fromRadioSubscription = _fromRadio!.lastValueStream.listen((data) {
      print('📡 Получены данные от FromRadio: ${data.length} байт');
                    // Парсим FromRadio сообщение используя официальные классы
                    final fromRadio = FromRadio.fromBuffer(data);
                    _handleFromRadio(fromRadio);
    });

    // Подписываемся на FromNum уведомления
    print('Подписка на FromNum...');
    _fromNumSubscription = _fromNum!.lastValueStream.listen((data) {
      print('🔔 Получено уведомление FromNum: ${data.length} байт');
      _handleFromNumNotification(Uint8List.fromList(data));
    });
    
    // Периодически читаем FromRadio для получения данных
    print('Запуск периодического чтения FromRadio...');
    Timer.periodic(const Duration(seconds: 2), (timer) {
      print('🔄 Периодическое чтение FromRadio...');
      _fromRadio!.read();
    });
    
    print('✅ Потоки данных настроены!');
  }

  /// Отправляет правильный handshake согласно официальной документации
  /// Использует ToRadio.want_config_id с nonce
  Future<void> _sendStartConfig() async {
    try {
      print('=== ОТПРАВКА ПРАВИЛЬНОГО HANDSHAKE ===');

      // Создаем ToRadio с want_config_id (nonce) используя официальные классы
      final nonce = DateTime.now().millisecondsSinceEpoch & 0x7fffffff;
      final toRadio = ToRadio(wantConfigId: nonce);

      print('Отправляем ToRadio.want_config_id = $nonce');
      print('Данные: ${toRadio.writeToBuffer().map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');

      await _toRadio!.write(toRadio.writeToBuffer());
      print('✅ Handshake отправлен!');

      // Теперь читаем FromRadio до пустого ответа
      await _drainFromRadio();
      
      // После успешного handshake запускаем автоматический тест
      if (_autoTestMode) {
        print('🚀 Запускаем автоматический тест Meshtastic...');
        _startAutoTest();
      } else {
        print('📍 Запрашиваем позиции всех узлов...');
        await Future.delayed(const Duration(milliseconds: 2000)); // Увеличиваем задержку
        await requestAllPositions();
        
        // Дополнительно запрашиваем позицию у самого устройства (node 0)
        print('📍 Запрашиваем позицию у самого устройства (node 0)...');
        await Future.delayed(const Duration(milliseconds: 1000));
        await requestPosition(0); // Запрос у самого устройства
      }

    } catch (e) {
      print('❌ Ошибка отправки handshake: $e');
    }
  }

  /// Читает FromRadio до пустого ответа согласно официальной документации
  /// Это ключевая часть handshake - устройство отправляет конфигурацию
  Future<void> _drainFromRadio() async {
    print('=== ЧТЕНИЕ FromRadio ДО ПУСТОГО ОТВЕТА ===');
    
    while (true) {
      try {
        final data = await _fromRadio!.read();
        if (data.isEmpty) {
          print('✅ FromRadio пуст - чтение завершено');
          break;
        }
        
        print('📡 Получены данные от FromRadio: ${data.length} байт');
        print('Hex: ${data.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
        
                    // Парсим FromRadio сообщение используя официальные классы
                    final fromRadio = FromRadio.fromBuffer(data);
                    _handleFromRadio(fromRadio);
        
      } catch (e) {
        print('❌ Ошибка чтения FromRadio: $e');
        break;
      }
    }
  }
  
  /// Обрабатывает FromRadio сообщения
  void _handleFromRadio(FromRadio fromRadio) {
    print('📡 === ОБРАБОТКА FromRadio ===');
    print('📡 Тип сообщения: ${fromRadio.hasPacket() ? "MeshPacket" : "Other"}');
    
    if (fromRadio.hasPacket()) {
      print('📦 Получен MeshPacket от узла ${fromRadio.packet.from}');
      _handleMeshPacket(fromRadio.packet);
    }

    if (fromRadio.hasMyInfo()) {
      print('ℹ️ Получена информация о моем узле: ${fromRadio.myInfo.myNodeNum}');
      print('Перезагрузок: ${fromRadio.myInfo.rebootCount}');
      print('Версия приложения: ${fromRadio.myInfo.minAppVersion}');
    }

    if (fromRadio.hasConfigCompleteId()) {
      print('✅ Конфигурация загружена полностью! ID: ${fromRadio.configCompleteId}');
      print('✅ Теперь можно запрашивать позиции узлов!');
    }
    
    if (fromRadio.hasNodeInfo()) {
      print('👥 Получена информация о узле: ${fromRadio.nodeInfo.num}');
      print('👥 Имя: ${fromRadio.nodeInfo.user?.longName ?? "N/A"}');
    }
    
    print('📡 === КОНЕЦ ОБРАБОТКИ FromRadio ===');
  }
  
  /// Обрабатывает MeshPacket сообщения
  void _handleMeshPacket(MeshPacket packet) {
    print('📦 Получен MeshPacket от узла ${packet.from}');
    print('📦 Portnum: ${packet.decoded?.portnum}');
    print('📦 HasDecoded: ${packet.hasDecoded()}');
    print('📦 Payload length: ${packet.decoded?.payload.length ?? 0}');
    
    if (packet.hasDecoded()) {
      final data = packet.decoded;

      // Проверяем тип сообщения по portnum
      switch (data.portnum) {
        case PortNum.POSITION_APP:
          print('📍 Обрабатываем POSITION_APP от узла ${packet.from}');
          print('📍 Payload length: ${data.payload.length} байт');
          print('📍 Payload hex: ${data.payload.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
          _handlePositionData(Uint8List.fromList(data.payload), packet.from);
          break;
        case PortNum.TEXT_MESSAGE_APP:
          print('💬 Обрабатываем TEXT_MESSAGE_APP от узла ${packet.from}');
          _handleTextMessage(Uint8List.fromList(data.payload));
          break;
        case PortNum.TELEMETRY_APP:
          print('📊 Обрабатываем TELEMETRY_APP от узла ${packet.from}');
          _handleTelemetryData(Uint8List.fromList(data.payload), packet.from);
          break;
        case PortNum.NODEINFO_APP:
          print('👤 Обрабатываем NODEINFO_APP от узла ${packet.from}');
          _handleNodeInfoData(Uint8List.fromList(data.payload), packet.from);
          break;
        case PortNum.ROUTING_APP:
          print('🛣️ Обрабатываем ROUTING_APP от узла ${packet.from}');
          _handleRoutingData(Uint8List.fromList(data.payload), packet.from);
          break;
        default:
          print('📨 Неизвестный тип сообщения: portnum=${data.portnum} от узла ${packet.from}');
      }
    } else {
      print('⚠️ MeshPacket от узла ${packet.from} не содержит decoded данных');
    }
  }
  
  /// Обрабатывает GPS данные
  void _handlePositionData(Uint8List payload, int nodeNum) {
    try {
      print('🔍 Обрабатываем Position данные от узла $nodeNum: ${payload.length} байт');
      print('🔍 Hex: ${payload.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
      
      // ИСПРАВЛЕНИЕ: Используем mergeFromBuffer вместо fromBuffer (как рекомендует ChatGPT)
      final position = Position()..mergeFromBuffer(payload);
      
      print('🔍 Position поля:');
      print('🔍   hasLatitudeI: ${position.hasLatitudeI()}');
      print('🔍   hasLongitudeI: ${position.hasLongitudeI()}');
      print('🔍   hasAltitude: ${position.hasAltitude()}');
      print('🔍   hasTime: ${position.hasTime()}');
      print('🔍   hasGroundSpeed: ${position.hasGroundSpeed()}');
      print('🔍   hasGroundTrack: ${position.hasGroundTrack()}');
      print('🔍   hasPrecisionBits: ${position.hasPrecisionBits()}');
      
      // Проверяем, не пустой ли это запрос позиции
      if (!position.hasLatitudeI() && !position.hasLongitudeI()) {
        print('📤 Получен запрос позиции от узла $nodeNum (пустой Position)');
        return;
      }
      
      if (position.hasLatitudeI() && position.hasLongitudeI()) {
        // Конвертируем из int32 в градусы (умножаем на 1e-7)
        final latitude = position.latitudeI / 10000000.0;
        final longitude = position.longitudeI / 10000000.0;
        final timestamp = position.hasTime() ? position.time : DateTime.now().millisecondsSinceEpoch ~/ 1000;
        
        print('📍 GPS координаты узла $nodeNum: $latitude, $longitude');
        print('📍 Время: ${DateTime.fromMillisecondsSinceEpoch(timestamp * 1000)}');
        
        // Сохраняем в БД
        _databaseService.savePosition(
          nodeNum: nodeNum,
          timestamp: timestamp,
          latitude: latitude,
          longitude: longitude,
          altitude: position.hasAltitude() ? position.altitude.toInt() : null,
          speedMs: position.hasGroundSpeed() ? position.groundSpeed.toDouble() : null,
          trackDeg: position.hasGroundTrack() ? position.groundTrack.toDouble() : null,
          precisionBits: position.hasPrecisionBits() ? position.precisionBits : null,
          rawData: payload,
        );
        
        // Отправляем в поток для UI
        _gpsDataController.add({
          'nodeNum': nodeNum,
          'latitude': latitude,
          'longitude': longitude,
          'altitude': position.hasAltitude() ? position.altitude.toDouble() : null,
          'timestamp': DateTime.fromMillisecondsSinceEpoch(timestamp * 1000),
          'source': 'T-beam (реальные GPS данные)',
        });
        
        print('✅ GPS координаты узла $nodeNum успешно сохранены и отправлены в UI');
      } else {
        print('⚠️ Position данные от узла $nodeNum не содержат координат!');
        print('⚠️ latitudeI: ${position.latitudeI}');
        print('⚠️ longitudeI: ${position.longitudeI}');
      }
    } catch (e) {
      print('❌ Ошибка парсинга GPS данных от узла $nodeNum: $e');
      print('❌ Payload hex: ${payload.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
    }
  }
  
  /// Обрабатывает текстовые сообщения
  void _handleTextMessage(Uint8List payload) {
    final text = String.fromCharCodes(payload);
    print('💬 Текстовое сообщение: $text');
  }

  /// Обрабатывает ROUTING_APP данные
  void _handleRoutingData(Uint8List payload, int nodeNum) {
    try {
      print('🛣️ Получены ROUTING данные от узла $nodeNum: ${payload.length} байт');
      print('🛣️ Hex: ${payload.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
      
      // Пытаемся парсить как RouteDiscovery
      if (payload.isNotEmpty) {
        // Проверяем первые байты для определения типа данных
        final firstByte = payload[0];
        print('🛣️ Первый байт: 0x${firstByte.toRadixString(16)}');
        
        // Возможно это RouteDiscovery или другие routing данные
        // Пока просто логируем содержимое
        if (payload.length > 1) {
          final dataString = String.fromCharCodes(payload.skip(1));
          print('🛣️ Данные как строка: $dataString');
        }
        
        // Проверяем, не содержат ли эти данные GPS координаты
        // Иногда GPS может передаваться через routing
        if (payload.length >= 8) {
          // Пытаемся интерпретировать как координаты
          try {
            final latBytes = payload.sublist(0, 4);
            final lonBytes = payload.sublist(4, 8);
            
            // Конвертируем из little-endian int32
            final lat = (latBytes[0] | (latBytes[1] << 8) | (latBytes[2] << 16) | (latBytes[3] << 24));
            final lon = (lonBytes[0] | (lonBytes[1] << 8) | (lonBytes[2] << 16) | (lonBytes[3] << 24));
            
            // Проверяем разумные значения координат (примерно для России)
            if (lat > 400000000 && lat < 800000000 && lon > 200000000 && lon < 2000000000) {
              final latitude = lat / 10000000.0;
              final longitude = lon / 10000000.0;
              print('🛣️ Возможные GPS координаты в ROUTING: $latitude, $longitude');
              
              // Сохраняем как позицию
              _databaseService.savePosition(
                nodeNum: nodeNum,
                timestamp: DateTime.now().millisecondsSinceEpoch ~/ 1000,
                latitude: latitude,
                longitude: longitude,
                rawData: payload,
              );
            }
          } catch (e) {
            print('🛣️ Не удалось интерпретировать как GPS: $e');
          }
        }
      }
      
    } catch (e) {
      print('❌ Ошибка обработки ROUTING данных: $e');
    }
  }

  /// Обрабатывает телеметрические данные
  void _handleTelemetryData(Uint8List payload, int nodeNum) {
    try {
      print('📊 Получены телеметрические данные от узла $nodeNum: ${payload.length} байт');
      print('📊 Hex: ${payload.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
      
      // Парсим Telemetry protobuf используя официальные классы
      final telemetry = Telemetry.fromBuffer(payload);
      final timestamp = telemetry.hasTime() ? telemetry.time : DateTime.now().millisecondsSinceEpoch ~/ 1000;
      
      if (telemetry.hasDeviceMetrics()) {
        final deviceMetrics = telemetry.deviceMetrics;
        print('🔋 Уровень батареи узла $nodeNum: ${deviceMetrics.batteryLevel}%');
        print('🔋 Напряжение: ${deviceMetrics.voltage}V');
        print('📡 Канальная утилизация: ${deviceMetrics.channelUtilization}%');
        print('📡 Воздушная утилизация TX: ${deviceMetrics.airUtilTx}%');
        print('⏱️ Время работы: ${deviceMetrics.uptimeSeconds} сек');
        
        // Сохраняем в БД
        _databaseService.saveDeviceMetrics(
          nodeNum: nodeNum,
          timestamp: timestamp,
          batteryLevel: deviceMetrics.batteryLevel?.toDouble(),
          voltage: deviceMetrics.voltage,
          channelUtil: deviceMetrics.channelUtilization,
          airUtilTx: deviceMetrics.airUtilTx,
          uptimeSeconds: deviceMetrics.uptimeSeconds,
          rawData: payload,
        );
      }
      
      if (telemetry.hasEnvironmentMetrics()) {
        final envMetrics = telemetry.environmentMetrics;
        print('🌡️ Температура узла $nodeNum: ${envMetrics.temperature}°C');
        print('💧 Влажность: ${envMetrics.relativeHumidity}%');
        print('🔋 Напряжение: ${envMetrics.voltage}V');
      }
      
    } catch (e) {
      print('❌ Ошибка парсинга телеметрии: $e');
    }
  }

  /// Обрабатывает информацию о узлах
  void _handleNodeInfoData(Uint8List payload, int nodeNum) {
    try {
      print('👤 Получена информация о узле $nodeNum: ${payload.length} байт');
      print('👤 Hex: ${payload.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
      
      // Парсим User protobuf используя официальные классы
      final user = User.fromBuffer(payload);
      
      print('👤 ID узла: ${user.id}');
      print('👤 Имя: ${user.longName}');
      print('👤 Короткое имя: ${user.shortName}');
      print('👤 MAC адрес: ${user.macaddr.map((b) => b.toRadixString(16).padLeft(2, '0')).join(':')}');
      
      if (user.hasHwModel()) {
        print('🔧 Модель: ${user.hwModel}');
      }
      
      if (user.hasRole()) {
        print('🎭 Роль: ${user.role}');
      }
      
      // NodeInfo не содержит GPS координат - они передаются отдельно через POSITION_APP
      print('ℹ️ NodeInfo узла $nodeNum получен (GPS координаты передаются отдельно)');
      
      // Сохраняем информацию о узле в БД
      _databaseService.saveNodeInfo(
        nodeNum: nodeNum,
        longName: user.longName.isNotEmpty ? user.longName : null,
        shortName: user.shortName.isNotEmpty ? user.shortName : null,
        macaddr: user.macaddr.isNotEmpty ? user.macaddr.map((b) => b.toRadixString(16).padLeft(2, '0')).join(':') : null,
        hwModel: user.hasHwModel() ? user.hwModel.toString() : null,
        role: user.hasRole() ? user.role.toString() : null,
      );
      
    } catch (e) {
      print('❌ Ошибка парсинга информации о узле: $e');
    }
  }
  

  /// Обрабатывает уведомления FromNum - вызывает чтение FromRadio
  void _handleFromNumNotification(Uint8List data) {
    print('🔔 Получено уведомление FromNum: ${data.length} байт');
    // При получении уведомления читаем FromRadio до пустого ответа
    _drainFromRadio();
  }

  Future<void> disconnect() async {
    try {
      await _fromRadioSubscription?.cancel();
      await _fromNumSubscription?.cancel();
      await _device?.disconnect();
      
      _fromRadio = null;
      _toRadio = null;
      _fromNum = null;
      _device = null;
    } catch (e) {
      print('Ошибка отключения: $e');
    }
  }

  /// Запускает автоматический тест Meshtastic
  void _startAutoTest() {
    _testStep = 0;
    _testResults.clear();
    _testResults.add('🚀 Начало автоматического теста Meshtastic');
    _testResults.add('⏰ ${DateTime.now().toIso8601String()}');
    
    print('🚀 === АВТОМАТИЧЕСКИЙ ТЕСТ MESHTASTIC ===');
    print('📋 План теста:');
    print('   1. Запрос позиций всех узлов (5 сек ожидания)');
    print('   2. Запрос телеметрии всех узлов (5 сек ожидания)');
    print('   3. Анализ результатов (2 сек)');
    print('   4. Автоматическое отключение');
    print('⏱️ Общее время теста: ~12 секунд');
    
    _executeTestStep();
  }
  
  /// Выполняет следующий шаг теста
  void _executeTestStep() {
    switch (_testStep) {
      case 0:
        _testResults.add('📍 Шаг 1: Запрос позиций всех узлов');
        print('📍 Шаг 1: Запрашиваем позиции всех узлов...');
        requestAllPositions().then((_) {
          _testResults.add('✅ Запросы позиций отправлены');
        });
        _scheduleNextStep(5000); // 5 секунд ожидания
        break;
        
      case 1:
        _testResults.add('📊 Шаг 2: Запрос телеметрии всех узлов');
        print('📊 Шаг 2: Запрашиваем телеметрию всех узлов...');
        requestAllTelemetry().then((_) {
          _testResults.add('✅ Запросы телеметрии отправлены');
        });
        _scheduleNextStep(5000); // 5 секунд ожидания
        break;
        
      case 2:
        _testResults.add('📈 Шаг 3: Анализ результатов');
        print('📈 Шаг 3: Анализируем полученные данные...');
        _analyzeTestResults();
        _scheduleNextStep(2000); // 2 секунды на анализ
        break;
        
      case 3:
        _testResults.add('🔚 Шаг 4: Завершение теста');
        print('🔚 Шаг 4: Завершаем тест и отключаемся...');
        _finishAutoTest();
        break;
    }
  }
  
  /// Планирует следующий шаг теста
  void _scheduleNextStep(int delayMs) {
    _testStep++;
    _autoTestTimer?.cancel();
    _autoTestTimer = Timer(Duration(milliseconds: delayMs), () {
      _executeTestStep();
    });
  }
  
  /// Анализирует результаты теста
  void _analyzeTestResults() async {
    try {
      final nodes = await getAllNodes();
      final positions = await getLatestPositions();
      final metrics = await getLatestMetrics();
      
      _testResults.add('📊 === РЕЗУЛЬТАТЫ ТЕСТА ===');
      _testResults.add('👥 Узлов в сети: ${nodes.length}');
      _testResults.add('📍 Позиций получено: ${positions.length}');
      _testResults.add('📊 Метрик получено: ${metrics.length}');
      
      // Детальный анализ каждого узла
      for (final node in nodes) {
        final nodeNum = node['node_num'] as int;
        final nodeName = node['long_name'] ?? node['short_name'] ?? 'Unknown';
        
        final hasPosition = positions.any((p) => p['node_num'] == nodeNum);
        final hasMetrics = metrics.any((m) => m['node_num'] == nodeNum);
        
        _testResults.add('🔍 Узел $nodeNum ($nodeName):');
        _testResults.add('   📍 GPS: ${hasPosition ? "✅" : "❌"}');
        _testResults.add('   📊 Телеметрия: ${hasMetrics ? "✅" : "❌"}');
        
        if (hasPosition) {
          final pos = positions.firstWhere((p) => p['node_num'] == nodeNum);
          _testResults.add('   📍 Координаты: ${pos['latitude']}, ${pos['longitude']}');
        }
        
        if (hasMetrics) {
          final met = metrics.firstWhere((m) => m['node_num'] == nodeNum);
          _testResults.add('   🔋 Батарея: ${met['battery_level']?.toStringAsFixed(1) ?? 'N/A'}%');
          _testResults.add('   ⚡ Напряжение: ${met['voltage']?.toStringAsFixed(2) ?? 'N/A'}V');
        }
      }
      
      _testResults.add('⏰ Тест завершен: ${DateTime.now().toIso8601String()}');
      
    } catch (e) {
      _testResults.add('❌ Ошибка анализа: $e');
    }
  }
  
  /// Завершает автоматический тест
  void _finishAutoTest() {
    print('🔚 === ЗАВЕРШЕНИЕ АВТОМАТИЧЕСКОГО ТЕСТА ===');
    
    // Выводим все результаты
    for (final result in _testResults) {
      print(result);
    }
    
    print('🔚 Автоматически отключаемся через 3 секунды...');
    
    // Автоматически отключаемся через 3 секунды
    Timer(const Duration(seconds: 3), () {
      print('🔌 Автоматическое отключение...');
      disconnect();
    });
  }
  
  /// Запрашивает телеметрию всех известных узлов
  Future<void> requestAllTelemetry() async {
    try {
      final nodes = await getAllNodes();
      print('📊 Запрашиваем телеметрию ${nodes.length} узлов...');
      
      for (final node in nodes) {
        final nodeNum = node['node_num'] as int;
        await requestTelemetry(nodeNum);
        // Небольшая задержка между запросами
        await Future.delayed(const Duration(milliseconds: 500));
      }
      
      print('✅ Запросы телеметрии отправлены всем узлам');
    } catch (e) {
      print('❌ Ошибка запроса телеметрии всех узлов: $e');
    }
  }

  void dispose() {
    _autoTestTimer?.cancel();
    _gpsDataController.close();
    _meshDevicesController.close();
    _databaseService.close();
  }
}

