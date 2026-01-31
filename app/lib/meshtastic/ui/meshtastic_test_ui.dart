import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../core/meshtastic_service.dart';

/// Простой UI для тестирования Meshtastic сервиса
class MeshtasticTestUI extends StatefulWidget {
  const MeshtasticTestUI({super.key});

  @override
  State<MeshtasticTestUI> createState() => _MeshtasticTestUIState();
}

class _MeshtasticTestUIState extends State<MeshtasticTestUI> {
  final MeshtasticService _meshtasticService = MeshtasticService();
  List<BluetoothDevice> _devices = [];
  bool _isScanning = false;
  List<Map<String, dynamic>> _nodes = [];
  List<Map<String, dynamic>> _positions = [];
  List<Map<String, dynamic>> _metrics = [];

  @override
  void initState() {
    super.initState();
    _startScan();
    _loadData();
  }

  @override
  void dispose() {
    _meshtasticService.dispose();
    super.dispose();
  }

  /// Начало сканирования устройств
  void _startScan() async {
    setState(() {
      _isScanning = true;
      _devices.clear();
    });

    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));
      
      FlutterBluePlus.scanResults.listen((results) {
        setState(() {
          _devices = results
              .where((result) => result.device.platformName.isNotEmpty)
              .map((result) => result.device)
              .toList();
        });
      });
    } catch (e) {
      _showSnackBar('Ошибка сканирования: $e');
    }

    Future.delayed(const Duration(seconds: 10), () {
      setState(() {
        _isScanning = false;
      });
      FlutterBluePlus.stopScan();
    });
  }

  /// Загрузка данных из базы
  void _loadData() async {
    final nodes = await _meshtasticService.getAllNodes();
    final positions = await _meshtasticService.getLatestPositions();
    final metrics = await _meshtasticService.getLatestMetrics();
    
    setState(() {
      _nodes = nodes;
      _positions = positions;
      _metrics = metrics;
    });
  }

  /// Подключение к устройству
  void _connectToDevice(BluetoothDevice device) async {
    final success = await _meshtasticService.connectToDevice(device);
    if (success) {
      _showSnackBar('Подключено к ${device.platformName}');
      _loadData();
    } else {
      _showSnackBar('Ошибка подключения');
    }
  }

  /// Отключение от устройства
  void _disconnect() async {
    await _meshtasticService.disconnect();
    _showSnackBar('Отключено');
    _loadData();
  }

  /// Запрос позиций всех узлов
  void _requestAllPositions() async {
    for (final node in _nodes) {
      final nodeNum = node['node_num'] as int;
      await _meshtasticService.requestPosition(nodeNum);
    }
    _showSnackBar('Запросы позиций отправлены');
  }

  /// Запрос телеметрии всех узлов
  void _requestAllTelemetry() async {
    for (final node in _nodes) {
      final nodeNum = node['node_num'] as int;
      await _meshtasticService.requestTelemetry(nodeNum);
    }
    _showSnackBar('Запросы телеметрии отправлены');
  }

  /// Показать уведомление
  void _showSnackBar(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(message)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Meshtastic Test'),
        actions: [
          IconButton(
            icon: Icon(_isScanning ? Icons.stop : Icons.refresh),
            onPressed: _isScanning ? null : _startScan,
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Статус подключения
            _buildConnectionStatus(),
            const SizedBox(height: 16),
            
            // Список устройств
            _buildDevicesList(),
            const SizedBox(height: 16),
            
            // Узлы сети
            _buildNodesSection(),
            const SizedBox(height: 16),
            
            // Позиции
            _buildPositionsSection(),
            const SizedBox(height: 16),
            
            // Метрики
            _buildMetricsSection(),
          ],
        ),
      ),
    );
  }

  /// Статус подключения
  Widget _buildConnectionStatus() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Статус подключения',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            Row(
              children: [
                Icon(
                  _meshtasticService.isConnected ? Icons.wifi : Icons.wifi_off,
                  color: _meshtasticService.isConnected ? Colors.green : Colors.red,
                ),
                const SizedBox(width: 8),
                Text(_meshtasticService.isConnected ? 'Подключено' : 'Отключено'),
              ],
            ),
            if (_meshtasticService.isConnected) ...[
              const SizedBox(height: 8),
              Row(
                children: [
                  ElevatedButton(
                    onPressed: _requestAllPositions,
                    child: const Text('Запросить позиции'),
                  ),
                  const SizedBox(width: 8),
                  ElevatedButton(
                    onPressed: _requestAllTelemetry,
                    child: const Text('Запросить телеметрию'),
                  ),
                  const SizedBox(width: 8),
                  ElevatedButton(
                    onPressed: _disconnect,
                    child: const Text('Отключить'),
                  ),
                ],
              ),
            ],
          ],
        ),
      ),
    );
  }

  /// Список устройств
  Widget _buildDevicesList() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Найденные устройства (${_devices.length})',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            if (_devices.isEmpty)
              const Text('Устройства не найдены')
            else
              ..._devices.map((device) => ListTile(
                title: Text(device.platformName),
                subtitle: Text(device.remoteId),
                trailing: ElevatedButton(
                  onPressed: () => _connectToDevice(device),
                  child: const Text('Подключить'),
                ),
              )),
          ],
        ),
      ),
    );
  }

  /// Узлы сети
  Widget _buildNodesSection() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Узлы сети (${_nodes.length})',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            if (_nodes.isEmpty)
              const Text('Узлы не найдены')
            else
              ..._nodes.map((node) => ListTile(
                title: Text(node['long_name'] ?? node['short_name'] ?? 'Unknown'),
                subtitle: Text('Node: ${node['node_num']}'),
                trailing: Text(node['role'] ?? 'Unknown'),
              )),
          ],
        ),
      ),
    );
  }

  /// Позиции
  Widget _buildPositionsSection() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Позиции (${_positions.length})',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            if (_positions.isEmpty)
              const Text('Позиции не найдены')
            else
              ..._positions.map((position) => ListTile(
                title: Text('Node: ${position['node_num']}'),
                subtitle: Text(
                  '${position['latitude']?.toStringAsFixed(6)}, '
                  '${position['longitude']?.toStringAsFixed(6)}',
                ),
                trailing: Text(
                  DateTime.fromMillisecondsSinceEpoch(position['timestamp'] * 1000)
                      .toString()
                      .substring(11, 19),
                ),
              )),
          ],
        ),
      ),
    );
  }

  /// Метрики
  Widget _buildMetricsSection() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Метрики (${_metrics.length})',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            if (_metrics.isEmpty)
              const Text('Метрики не найдены')
            else
              ..._metrics.map((metric) => ListTile(
                title: Text('Node: ${metric['node_num']}'),
                subtitle: Text(
                  'Батарея: ${metric['battery_level']?.toStringAsFixed(1) ?? 'N/A'}%, '
                  'Напряжение: ${metric['voltage']?.toStringAsFixed(2) ?? 'N/A'}V',
                ),
                trailing: Text(
                  DateTime.fromMillisecondsSinceEpoch(metric['timestamp'] * 1000)
                      .toString()
                      .substring(11, 19),
                ),
              )),
          ],
        ),
      ),
    );
  }
}
