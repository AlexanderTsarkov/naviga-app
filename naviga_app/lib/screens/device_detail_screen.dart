import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

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
  Timer? _updateTimer;

  @override
  void initState() {
    super.initState();
    _listenToConnectionState();
    _startUpdateTimer();
  }

  @override
  void dispose() {
    _updateTimer?.cancel();
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

    // Показываем диалог для ввода кода
    _showCodeInputDialog();
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
    final code = _codeController.text.trim();
    if (code.isEmpty) {
      setState(() {
        _status = 'Введите код подключения';
      });
      return;
    }

    setState(() {
      _connecting = true;
      _status = 'Подключение с кодом: $code...';
    });

    try {
      // Здесь будет реальное подключение с кодом
      await Future.delayed(const Duration(seconds: 2)); // Имитация
      
      setState(() {
        _status = 'Подключено';
        _connecting = false;
        _gpsEnabled = true;
      });
      
      // Автоматически получаем GPS данные после подключения
      _getGpsDataFromDevice();
      _getConnectedDevicesFromDevice();
      
    } catch (e) {
      setState(() {
        _status = 'Ошибка подключения: $e';
        _connecting = false;
      });
    }
  }

  Future<void> _disconnect() async {
    try {
      await widget.device.disconnect();
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
            if (_connectedDevices.isNotEmpty) ...[
              const SizedBox(height: 16),
              Text(
                'Связанные устройства (${_connectedDevices.length})',
                style: Theme.of(context).textTheme.titleLarge,
              ),
              const SizedBox(height: 8),
              ..._connectedDevices.map((device) => 
                Card(
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
                                    device['name'],
                                    style: Theme.of(context).textTheme.titleMedium,
                                  ),
                                  Text(
                                    device['id'],
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
                                      color: _getBatteryColor(device['battery']),
                                    ),
                                    const SizedBox(width: 4),
                                    Text(
                                      '${device['battery']}%',
                                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                        color: _getBatteryColor(device['battery']),
                                        fontWeight: FontWeight.bold,
                                      ),
                                    ),
                                  ],
                                ),
                                Text(
                                  '${device['rssi']} dBm',
                                  style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                    color: Colors.grey[600],
                                  ),
                                ),
                              ],
                            ),
                          ],
                        ),
                        const SizedBox(height: 8),
                        _buildInfoRow('Координаты', device['coordinates']),
                        _buildInfoRow('Последний сигнал', _formatTimeAgo(device['lastSeen'])),
                      ],
                    ),
                  ),
                ),
              ).toList(),
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

  Color _getBatteryColor(int batteryLevel) {
    if (batteryLevel > 50) {
      return Colors.green;
    } else if (batteryLevel > 20) {
      return Colors.orange;
    } else {
      return Colors.red;
    }
  }
}