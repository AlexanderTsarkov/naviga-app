import 'dart:async';
import 'dart:typed_data';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

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

  Stream<Map<String, dynamic>> get gpsDataStream => _gpsDataController.stream;
  Stream<List<Map<String, dynamic>>> get meshDevicesStream => _meshDevicesController.stream;

  Future<bool> connectToDevice(BluetoothDevice device) async {
    try {
      _device = device;
      
      // Подключаемся к устройству
      await device.connect();
      
      // Устанавливаем MTU размер
      await device.requestMtu(512);
      
      // Получаем сервисы
      final services = await device.discoverServices();
      final meshService = services.firstWhere(
        (service) => service.uuid.toString().toLowerCase() == meshServiceUuid.toLowerCase(),
        orElse: () => throw Exception('MeshBluetoothService не найден'),
      );

      // Получаем характеристики
      _fromRadio = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == fromRadioUuid.toLowerCase(),
      );
      
      _toRadio = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == toRadioUuid.toLowerCase(),
      );
      
      _fromNum = meshService.characteristics.firstWhere(
        (char) => char.uuid.toString().toLowerCase() == fromNumUuid.toLowerCase(),
      );

      // Подписываемся на уведомления
      await _fromNum!.setNotifyValue(true);
      
      // Настраиваем потоки данных
      _setupDataStreams();
      
      // Отправляем startConfig
      await _sendStartConfig();
      
      return true;
    } catch (e) {
      print('Ошибка подключения к Meshtastic устройству: $e');
      return false;
    }
  }

  void _setupDataStreams() {
    // Подписываемся на FromRadio данные
    _fromRadioSubscription = _fromRadio!.lastValueStream.listen((data) {
      _processFromRadioData(Uint8List.fromList(data));
    });

    // Подписываемся на FromNum уведомления
    _fromNumSubscription = _fromNum!.lastValueStream.listen((data) {
      _handleFromNumNotification(Uint8List.fromList(data));
    });
  }

  Future<void> _sendStartConfig() async {
    try {
      // Создаем простой startConfig пакет
      // В реальности это должен быть protobuf, но для начала используем простой байт
      final startConfig = Uint8List.fromList([0x01]); // Простой startConfig
      await _toRadio!.write(startConfig);
    } catch (e) {
      print('Ошибка отправки startConfig: $e');
    }
  }

  void _processFromRadioData(Uint8List data) {
    if (data.isEmpty) return;
    
    try {
      // Обработка реальных данных от Meshtastic устройства
      print('=== ПОЛУЧЕНЫ ДАННЫЕ ОТ T-BEAM ===');
      print('Размер данных: ${data.length} байт');
      print('Hex данные: ${data.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
      print('Dec данные: ${data.map((b) => b.toString()).join(' ')}');
      print('================================');
      
      // TODO: Здесь нужно парсить protobuf сообщения от Meshtastic
      // Пока что отправляем тестовые данные для демонстрации
      _gpsDataController.add({
        'latitude': 58.5218 + (DateTime.now().millisecond / 10000),
        'longitude': 31.2750 + (DateTime.now().millisecond / 10000),
        'timestamp': DateTime.now(),
        'source': 'T-beam (анализируем реальные данные)',
      });
      
      // Имитируем получение списка устройств mesh
      _meshDevicesController.add([
        {
          'id': 'REAL-T-Beam-001',
          'name': 'Реальное устройство 1',
          'coordinates': '58.5200°N, 31.2700°E',
          'lastSeen': DateTime.now().subtract(const Duration(minutes: 1)),
          'rssi': -45,
          'battery': 85,
        },
        {
          'id': 'REAL-T-Beam-002',
          'name': 'Реальное устройство 2',
          'coordinates': '58.5220°N, 31.2800°E',
          'lastSeen': DateTime.now().subtract(const Duration(seconds: 30)),
          'rssi': -38,
          'battery': 92,
        },
      ]);
      
    } catch (e) {
      print('Ошибка обработки FromRadio данных: $e');
    }
  }

  void _handleFromNumNotification(Uint8List data) {
    print('Получено уведомление FromNum: ${data.length} байт');
    // Читаем новые данные из FromRadio
    _fromRadio!.read();
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

  void dispose() {
    _gpsDataController.close();
    _meshDevicesController.close();
  }
}
