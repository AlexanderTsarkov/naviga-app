import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../services/meshtastic_bluetooth_service.dart';

class DeviceDetailScreen extends StatefulWidget {
  final BluetoothDevice device;

  const DeviceDetailScreen({super.key, required this.device});

  @override
  State<DeviceDetailScreen> createState() => _DeviceDetailScreenState();
}

class _DeviceDetailScreenState extends State<DeviceDetailScreen> {
  BluetoothConnectionState _connectionState = BluetoothConnectionState.disconnected;
  bool _connecting = false;
  String _status = 'Устройство не подключено';
  final TextEditingController _codeController = TextEditingController();
  String _gpsCoordinates = 'GPS не получен';
  bool _gpsEnabled = false;
  DateTime? _lastGpsUpdate;
  List<Map<String, dynamic>> _connectedDevices = [];
  List<Map<String, dynamic>> _allNodes = [];
  List<Map<String, dynamic>> _latestPositions = [];
  List<Map<String, dynamic>> _latestMetrics = [];
  Timer? _updateTimer;
  final MeshtasticBluetoothService _meshtasticService = MeshtasticBluetoothService();
  StreamSubscription? _gpsSubscription;
  StreamSubscription? _meshDevicesSubscription;

  @override
  void initState() {
    super.initState();
    _listenToConnectionState();
    _startUpdateTimer();
  }

  @override
  void dispose() {
    _updateTimer?.cancel();
    _gpsSubscription?.cancel();
    _meshDevicesSubscription?.cancel();
    _meshtasticService.dispose();
    super.dispose();
  }

  void _startUpdateTimer() {
    // Обновляем таймеры каждую секунду для корректного отображения времени
    _updateTimer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (mounted) {
        setState(() {
          // Принудительно обновляем UI для пересчета времени
        });
      }
    });
    
    // Загружаем данные из БД каждые 5 секунд
    Timer.periodic(const Duration(seconds: 5), (timer) {
      _loadDataFromDatabase();
    });
  }
  
  Future<void> _loadDataFromDatabase() async {
    try {
      final allNodes = await _meshtasticService.getAllNodes();
      final latestPositions = await _meshtasticService.getLatestPositions();
      final latestMetrics = await _meshtasticService.getLatestMetrics();
      
      setState(() {
        _allNodes = allNodes;
        _latestPositions = latestPositions;
        _latestMetrics = latestMetrics;
      });
      
      print('📊 Загружено из БД: ${allNodes.length} узлов, ${latestPositions.length} позиций, ${latestMetrics.length} метрик');
    } catch (e) {
      print('❌ Ошибка загрузки данных из БД: $e');
    }
  }

  void _listenToConnectionState() {
    widget.device.connectionState.listen((state) {
      setState(() {
        _connectionState = state;
        _connecting = false;
        
        switch (state) {
          case BluetoothConnectionState.connected:
            _status = 'Подключено';
            _gpsEnabled = true;
            _getGpsDataFromDevice();
            _getConnectedDevicesFromDevice();
            _loadDataFromDatabase(); // Загружаем данные сразу после подключения
            break;
          case BluetoothConnectionState.disconnected:
            _status = 'Отключено';
            _gpsEnabled = false;
            _gpsCoordinates = 'GPS не получен';
            _connectedDevices.clear();
            break;
          case BluetoothConnectionState.connecting:
            _status = 'Подключение...';
            break;
          case BluetoothConnectionState.disconnecting:
            _status = 'Отключение...';
            break;
        }
      });
    });
  }

  Future<void> _connect() async {
    if (_connectionState == BluetoothConnectionState.connected) {
      await _disconnect();
      return;
    }

    // Подключаемся напрямую без запроса кода - система сама запросит
    _attemptConnection();
  }

  void _showCodeInputDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('Код подключения'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Text('Введите код подключения для Meshtastic устройства:'),
              const SizedBox(height: 16),
              TextField(
                controller: _codeController,
                decoration: const InputDecoration(
                  labelText: 'Код',
                  hintText: 'Введите код',
                  border: OutlineInputBorder(),
                ),
                keyboardType: TextInputType.number,
                maxLength: 6,
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
                _codeController.clear();
              },
              child: const Text('Отмена'),
            ),
            ElevatedButton(
              onPressed: () {
                Navigator.of(context).pop();
                _attemptConnection();
              },
              child: const Text('Подключиться'),
            ),
          ],
        );
      },
    );
  }

  Future<void> _attemptConnection() async {
    setState(() {
      _connecting = true;
      _status = 'Подключение к Meshtastic устройству...';
    });

    try {
      // Подключаемся к Meshtastic устройству через реальный сервис
      final success = await _meshtasticService.connectToDevice(widget.device);
      
      if (success) {
        setState(() {
          _status = 'Подключено к Meshtastic';
          _connecting = false;
          _gpsEnabled = true;
        });
        
        // Подписываемся на реальные данные
        _subscribeToRealData();
      } else {
        setState(() {
          _status = 'Ошибка подключения к Meshtastic';
          _connecting = false;
        });
      }
      
    } catch (e) {
      setState(() {
        _status = 'Ошибка подключения: $e';
        _connecting = false;
      });
    }
  }

  Future<void> _disconnect() async {
    try {
      await _meshtasticService.disconnect();
      setState(() {
        _status = 'Отключено';
        _gpsEnabled = false;
        _gpsCoordinates = 'GPS не получен';
        _connectedDevices.clear();
      });
    } catch (e) {
      setState(() {
        _status = 'Ошибка отключения: $e';
      });
    }
  }

  Future<void> _getGpsDataFromDevice() async {
    setState(() {
      _gpsCoordinates = 'Получение GPS от T-beam...';
    });
    
    await Future.delayed(const Duration(seconds: 2));
    
    if (_gpsEnabled && _connectionState == BluetoothConnectionState.connected) {
      setState(() {
        _gpsCoordinates = '58.5218°N, 31.2750°E (ИМИТАЦИЯ - не от реального T-beam)';
        _lastGpsUpdate = DateTime.now();
      });
      
      // Подписываемся на обновления GPS
      _startGpsSubscription();
    }
  }

  void _startGpsSubscription() {
    Timer.periodic(const Duration(seconds: 10), (timer) {
      if (!_gpsEnabled || _connectionState != BluetoothConnectionState.connected) {
        timer.cancel();
        return;
      }
      
      setState(() {
        final lat = 58.5218 + (DateTime.now().millisecond / 10000);
        final lng = 31.2750 + (DateTime.now().millisecond / 10000);
        _gpsCoordinates = '${lat.toStringAsFixed(4)}°N, ${lng.toStringAsFixed(4)}°E (ИМИТАЦИЯ)';
        _lastGpsUpdate = DateTime.now();
      });
    });
  }

  Future<void> _getConnectedDevicesFromDevice() async {
    // TODO: Получить реальные данные от подключенного Meshtastic устройства
    // Пока что показываем сообщение о том, что это имитация
    setState(() {
      _connectedDevices = [
        {
          'id': 'ИМИТАЦИЯ',
          'name': 'Тестовые данные (не реальные)',
          'coordinates': 'Реальные данные будут получены от T-beam',
          'lastSeen': DateTime.now(),
          'rssi': 0,
          'battery': 0,
        },
      ];
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.device.remoteId.str),
        backgroundColor: _connectionState == BluetoothConnectionState.connected 
            ? Colors.green[700] 
            : null,
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Основная карточка устройства
            Card(
              elevation: _connectionState == BluetoothConnectionState.connected ? 8 : 2,
              color: _connectionState == BluetoothConnectionState.connected 
                  ? Colors.green[50] 
                  : null,
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        Icon(
                          Icons.bluetooth,
                          color: _connectionState == BluetoothConnectionState.connected 
                              ? Colors.green 
                              : Colors.grey,
                        ),
                        const SizedBox(width: 8),
                        Expanded(
                          child: Text(
                            'Meshtastic Device',
                            style: Theme.of(context).textTheme.titleLarge?.copyWith(
                              color: _connectionState == BluetoothConnectionState.connected 
                                  ? Colors.green[700] 
                                  : null,
                            ),
                          ),
                        ),
                        if (_connectionState == BluetoothConnectionState.connected)
                          Container(
                            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                            decoration: BoxDecoration(
                              color: Colors.green,
                              borderRadius: BorderRadius.circular(12),
                            ),
                            child: const Text(
                              'ПОДКЛЮЧЕНО',
                              style: TextStyle(
                                color: Colors.white,
                                fontSize: 12,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                      ],
                    ),
                    const SizedBox(height: 16),
                    _buildInfoRow('ID', widget.device.remoteId.str),
                    _buildInfoRow('Статус', _status),
                    if (_connectionState == BluetoothConnectionState.connected) ...[
                      const SizedBox(height: 16),
                      _buildInfoRow('GPS координаты', _gpsCoordinates),
                      if (_lastGpsUpdate != null) ...[
                        const SizedBox(height: 4),
                        Text(
                          'Обновлено: ${_formatTimeAgo(_lastGpsUpdate!)}',
                          style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: Colors.green[600],
                          ),
                        ),
                      ],
                    ],
                    const SizedBox(height: 16),
                    SizedBox(
                      width: double.infinity,
                      child: ElevatedButton.icon(
                        onPressed: _connecting ? null : _connect,
                        icon: _connecting 
                            ? const SizedBox(
                                width: 16,
                                height: 16,
                                child: CircularProgressIndicator(strokeWidth: 2),
                              )
                            : Icon(_connectionState == BluetoothConnectionState.connected 
                                ? Icons.bluetooth_disabled 
                                : Icons.bluetooth),
                        label: Text(_connectionState == BluetoothConnectionState.connected 
                            ? 'Отключиться' 
                            : 'Подключиться'),
                        style: ElevatedButton.styleFrom(
                          backgroundColor: _connectionState == BluetoothConnectionState.connected 
                              ? Colors.red 
                              : Colors.green,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
            
            // Связанные устройства
            // Карточка подключенного устройства
            if (_connectionState == BluetoothConnectionState.connected) ...[
              const SizedBox(height: 16),
              Text(
                'Подключенное устройство',
                style: Theme.of(context).textTheme.titleLarge,
              ),
              const SizedBox(height: 8),
              _buildConnectedDeviceCard(),
            ],
            
        // Кнопки для запроса данных
        if (_connectionState == BluetoothConnectionState.connected) ...[
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: ElevatedButton.icon(
                  onPressed: _requestAllPositions,
                  icon: const Icon(Icons.location_on),
                  label: const Text('Запросить позиции'),
                ),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: ElevatedButton.icon(
                  onPressed: _requestAllTelemetry,
                  icon: const Icon(Icons.analytics),
                  label: const Text('Запросить метрики'),
                ),
              ),
            ],
          ),
        ],
        
        // Карточки других узлов в сети
        if (_allNodes.isNotEmpty) ...[
          const SizedBox(height: 16),
          Text(
            'Узлы в сети (${_allNodes.length})',
            style: Theme.of(context).textTheme.titleLarge,
          ),
          const SizedBox(height: 8),
          ..._allNodes.map((node) => _buildNodeCard(node)),
        ],
          ],
        ),
      ),
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2.0),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 100,
            child: Text(
              '$label:',
              style: const TextStyle(fontWeight: FontWeight.bold),
            ),
          ),
          Expanded(
            child: Text(value),
          ),
        ],
      ),
    );
  }

  String _formatTimeAgo(DateTime dateTime) {
    final now = DateTime.now();
    final difference = now.difference(dateTime);
    
    if (difference.inSeconds < 60) {
      return '${difference.inSeconds} сек назад';
    } else if (difference.inMinutes < 60) {
      return '${difference.inMinutes} мин назад';
    } else if (difference.inHours < 24) {
      return '${difference.inHours} ч назад';
    } else {
      return '${difference.inDays} дн назад';
    }
  }

  void _subscribeToRealData() {
    // Подписываемся на GPS данные от реального устройства
    _gpsSubscription = _meshtasticService.gpsDataStream.listen((gpsData) {
      setState(() {
        _gpsCoordinates = '${gpsData['latitude'].toStringAsFixed(4)}°N, ${gpsData['longitude'].toStringAsFixed(4)}°E (${gpsData['source']})';
        _lastGpsUpdate = gpsData['timestamp'];
      });
    });

    // Подписываемся на данные mesh устройств
    _meshDevicesSubscription = _meshtasticService.meshDevicesStream.listen((devices) {
      setState(() {
        _connectedDevices = devices;
      });
    });
  }

  Color _getBatteryColor(int batteryLevel) {
    if (batteryLevel > 50) {
      return Colors.green;
    } else if (batteryLevel > 20) {
      return Colors.orange;
    } else {
      return Colors.red;
    }
  }

  /// Создает карточку подключенного устройства
  Widget _buildConnectedDeviceCard() {
    // Находим данные подключенного устройства
    final connectedNode = _allNodes.isNotEmpty ? _allNodes.first : null;
    final latestMetrics = _latestMetrics.isNotEmpty ? _latestMetrics.first : null;
    final latestPosition = _latestPositions.isNotEmpty ? _latestPositions.first : null;
    
    return Card(
      margin: const EdgeInsets.only(bottom: 8),
      child: Padding(
        padding: const EdgeInsets.all(12.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.bluetooth_connected, color: Colors.blue),
                const SizedBox(width: 8),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        connectedNode?['long_name'] ?? widget.device.platformName ?? 'T-beam',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      Text(
                        'ID: ${connectedNode?['node_num'] ?? 'N/A'}',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.grey[600],
                        ),
                      ),
                    ],
                  ),
                ),
                Column(
                  crossAxisAlignment: CrossAxisAlignment.end,
                  children: [
                    Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(
                          Icons.battery_std,
                          size: 16,
                          color: _getBatteryColorFromMetrics(latestMetrics ?? {}),
                        ),
                        const SizedBox(width: 4),
                        Text(
                          _getBatteryText(latestMetrics ?? {}),
                          style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: _getBatteryColorFromMetrics(latestMetrics ?? {}),
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ],
                    ),
                    Text(
                      'BT: N/A dBm',
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        color: Colors.grey[600],
                      ),
                    ),
                  ],
                ),
              ],
            ),
            const SizedBox(height: 8),
            _buildInfoRow('Статус', _status),
            _buildInfoRow('Последнее обновление', latestMetrics?['t'] != null ? _formatTimeAgo(DateTime.fromMillisecondsSinceEpoch(latestMetrics!['t'] * 1000)) : 'N/A'),
            _buildInfoRow('Записей в логе', '${_latestMetrics.length}'),
            if (latestPosition != null) ...[
              _buildInfoRow('Координаты', '${latestPosition['lat']?.toStringAsFixed(6) ?? 'N/A'}, ${latestPosition['lon']?.toStringAsFixed(6) ?? 'N/A'}'),
              _buildInfoRow('Последняя позиция', latestPosition['t'] != null ? _formatTimeAgo(DateTime.fromMillisecondsSinceEpoch(latestPosition['t'] * 1000)) : 'N/A'),
              _buildInfoRow('GPS записей', '${_latestPositions.length}'),
            ] else ...[
              _buildInfoRow('Координаты', 'GPS не получен'),
              _buildInfoRow('GPS записей', '0'),
            ],
          ],
        ),
      ),
    );
  }

  /// Создает карточку узла в сети
  Widget _buildNodeCard(Map<String, dynamic> node) {
    final nodeNum = node['node_num'];
    final latestMetrics = _latestMetrics.firstWhere(
      (m) => m['node_num'] == nodeNum,
      orElse: () => <String, dynamic>{},
    );
    final latestPosition = _latestPositions.firstWhere(
      (p) => p['node_num'] == nodeNum,
      orElse: () => <String, dynamic>{},
    );
    
    return Card(
      margin: const EdgeInsets.only(bottom: 8),
      child: Padding(
        padding: const EdgeInsets.all(12.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.radio, color: Colors.green),
                const SizedBox(width: 8),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        node['long_name'] ?? 'Узел $nodeNum',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      Text(
                        'ID: $nodeNum',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.grey[600],
                        ),
                      ),
                    ],
                  ),
                ),
                Column(
                  crossAxisAlignment: CrossAxisAlignment.end,
                  children: [
                    Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(
                          Icons.battery_std,
                          size: 16,
                          color: _getBatteryColorFromMetrics(latestMetrics ?? {}),
                        ),
                        const SizedBox(width: 4),
                        Text(
                          _getBatteryText(latestMetrics ?? {}),
                          style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: _getBatteryColorFromMetrics(latestMetrics ?? {}),
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ],
                    ),
                    Text(
                      'Радио: ${latestMetrics['voltage']?.toStringAsFixed(1) ?? 'N/A'}V',
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        color: Colors.grey[600],
                      ),
                    ),
                  ],
                ),
              ],
            ),
            const SizedBox(height: 8),
            _buildInfoRow('Последний сигнал', node['last_seen'] != null ? _formatTimeAgo(DateTime.fromMillisecondsSinceEpoch(node['last_seen'] * 1000)) : 'N/A'),
            _buildInfoRow('Записей метрик', '${_latestMetrics.where((m) => m['node_num'] == nodeNum).length}'),
            if (latestPosition.isNotEmpty) ...[
              _buildInfoRow('Координаты', '${latestPosition['lat']?.toStringAsFixed(6) ?? 'N/A'}, ${latestPosition['lon']?.toStringAsFixed(6) ?? 'N/A'}'),
              _buildInfoRow('Последняя позиция', latestPosition['t'] != null ? _formatTimeAgo(DateTime.fromMillisecondsSinceEpoch(latestPosition['t'] * 1000)) : 'N/A'),
              _buildInfoRow('GPS записей', '${_latestPositions.where((p) => p['node_num'] == nodeNum).length}'),
            ] else ...[
              _buildInfoRow('Координаты', 'GPS не получен'),
              _buildInfoRow('GPS записей', '0'),
            ],
          ],
        ),
      ),
    );
  }

  /// Получает цвет батареи из метрик
  Color _getBatteryColorFromMetrics(Map<String, dynamic> metrics) {
    final battery = metrics['battery_level']?.toDouble();
    if (battery == null) return Colors.grey;
    return _getBatteryColor(battery.round());
  }

  /// Получает текст батареи из метрик
  String _getBatteryText(Map<String, dynamic> metrics) {
    final battery = metrics['battery_level']?.toDouble();
    if (battery == null) return 'N/A';
    return '${battery.toStringAsFixed(0)}%';
  }

  /// Запрашивает позиции всех узлов
  Future<void> _requestAllPositions() async {
    try {
      await _meshtasticService.requestAllPositions();
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Запросы позиций отправлены')),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Ошибка запроса позиций: $e')),
      );
    }
  }

  /// Запрашивает телеметрию всех узлов
  Future<void> _requestAllTelemetry() async {
    try {
      final nodes = await _meshtasticService.getAllNodes();
      for (final node in nodes) {
        final nodeNum = node['node_num'] as int;
        await _meshtasticService.requestTelemetry(nodeNum);
        await Future.delayed(const Duration(milliseconds: 500));
      }
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Запросы телеметрии отправлены')),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Ошибка запроса телеметрии: $e')),
      );
    }
  }
}