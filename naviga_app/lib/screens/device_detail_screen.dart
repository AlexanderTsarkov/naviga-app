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
  bool _showCodeDialog = false;
  String _gpsCoordinates = 'GPS не получен';
  bool _gpsEnabled = false;
  DateTime? _lastGpsUpdate;
  List<Map<String, dynamic>> _connectedDevices = [];

  @override
  void initState() {
    super.initState();
    _listenToConnectionState();
  }

  void _listenToConnectionState() {
    widget.device.connectionState.listen((state) {
      setState(() {
        _connectionState = state;
        _connecting = false;
        
        switch (state) {
          case BluetoothConnectionState.connected:
            _status = 'Подключено';
            break;
          case BluetoothConnectionState.disconnected:
            _status = 'Отключено';
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
        _status = 'Подключение с кодом $code - успешно!';
        _connecting = false;
        _gpsEnabled = true; // Автоматически включаем GPS при подключении
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
    // Имитация получения GPS данных от подключенного Meshtastic устройства
    // В реальности это будет через Bluetooth характеристику от T-beam
    setState(() {
      _gpsCoordinates = 'Получение GPS от T-beam...';
    });
    
    await Future.delayed(const Duration(seconds: 2));
    
    if (_gpsEnabled && _connectionState == BluetoothConnectionState.connected) {
      setState(() {
        _gpsCoordinates = '58.5218°N, 31.2750°E (от T-beam)';
        _lastGpsUpdate = DateTime.now();
      });
      
      // Подписываемся на обновления GPS
      _startGpsSubscription();
    }
  }

  void _startGpsSubscription() {
    // Имитация подписки на обновления GPS от устройства
    // В реальности это будет через Bluetooth characteristic notifications
    Timer.periodic(const Duration(seconds: 10), (timer) {
      if (!_gpsEnabled || _connectionState != BluetoothConnectionState.connected) {
        timer.cancel();
        return;
      }
      
      // Имитация получения новых координат
      setState(() {
        final lat = 58.5218 + (DateTime.now().millisecond / 10000);
        final lng = 31.2750 + (DateTime.now().millisecond / 10000);
        _gpsCoordinates = '${lat.toStringAsFixed(4)}°N, ${lng.toStringAsFixed(4)}°E (от T-beam)';
        _lastGpsUpdate = DateTime.now();
      });
    });
  }

  Future<void> _getConnectedDevicesFromDevice() async {
    // Имитация получения списка связанных устройств от T-beam
    setState(() {
      _connectedDevices = [
        {
          'id': 'T-Beam-001',
          'name': 'Охотник Иван',
          'coordinates': '58.5200°N, 31.2700°E',
          'lastSeen': DateTime.now().subtract(const Duration(minutes: 2)),
          'rssi': -45,
        },
        {
          'id': 'T-Beam-002', 
          'name': 'Собака Рекс',
          'coordinates': '58.5220°N, 31.2800°E',
          'lastSeen': DateTime.now().subtract(const Duration(minutes: 1)),
          'rssi': -38,
        },
      ];
    });
  }

  void _refreshGps() {
    if (_gpsEnabled) {
      setState(() {
        _gpsCoordinates = 'Обновление GPS...';
      });
      _simulateGpsData();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.device.remoteId.str),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Информация об устройстве',
                      style: Theme.of(context).textTheme.titleLarge,
                    ),
                    const SizedBox(height: 16),
                    _buildInfoRow('ID', widget.device.remoteId.str),
                    _buildInfoRow('Имя', 'Meshtastic Device'),
                    _buildInfoRow('Статус', _status),
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
            const SizedBox(height: 16),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Text(
                          'GPS координаты',
                          style: Theme.of(context).textTheme.titleLarge,
                        ),
                        Switch(
                          value: _gpsEnabled,
                          onChanged: _gpsEnabled ? null : (value) {
                            setState(() {
                              _gpsEnabled = value;
                              if (value) {
                                _gpsCoordinates = 'Получение GPS...';
                                _simulateGpsData();
                              } else {
                                _gpsCoordinates = 'GPS отключен';
                              }
                            });
                          },
                        ),
                      ],
                    ),
                    const SizedBox(height: 16),
                    _buildInfoRow('Координаты', _gpsCoordinates),
                    if (_lastGpsUpdate != null) ...[
                      const SizedBox(height: 4),
                      Text(
                        'Обновлено: ${_formatTimeAgo(_lastGpsUpdate!)}',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.green[600],
                        ),
                      ),
                    ],
                    const SizedBox(height: 8),
                    Text(
                      'GPS данные получаются от подключенного T-beam устройства через Bluetooth',
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        color: Colors.grey[600],
                      ),
                    ),
                    if (_gpsEnabled) ...[
                      const SizedBox(height: 8),
                      ElevatedButton.icon(
                        onPressed: _refreshGps,
                        icon: const Icon(Icons.refresh),
                        label: const Text('Обновить GPS'),
                      ),
                    ],
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),
            if (_connectedDevices.isNotEmpty) ...[
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Связанные устройства (${_connectedDevices.length})',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: 16),
                      ..._connectedDevices.map((device) => 
                        Card(
                          margin: const EdgeInsets.only(bottom: 8),
                          child: ListTile(
                            leading: const Icon(Icons.bluetooth_connected),
                            title: Text(device['name']),
                            subtitle: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text('ID: ${device['id']}'),
                                Text('Координаты: ${device['coordinates']}'),
                                Text('RSSI: ${device['rssi']} dBm'),
                                Text('Последний сигнал: ${_formatTimeAgo(device['lastSeen'])}'),
                              ],
                            ),
                          ),
                        ),
                      ).toList(),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 16),
            ],
            if (_connectionState == BluetoothConnectionState.connected) ...[
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Сервисы',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: 16),
                      FutureBuilder<List<BluetoothService>>(
                        future: widget.device.discoverServices(),
                        builder: (context, snapshot) {
                          if (snapshot.connectionState == ConnectionState.waiting) {
                            return const CircularProgressIndicator();
                          }
                          
                          if (snapshot.hasError) {
                            return Text('Ошибка: ${snapshot.error}');
                          }
                          
                          final services = snapshot.data ?? [];
                          return Column(
                            children: services.map((service) => 
                              ListTile(
                                leading: const Icon(Icons.bluetooth),
                                title: Text('Service: ${service.uuid}'),
                                subtitle: Text('Characteristics: ${service.characteristics.length}'),
                              ),
                            ).toList(),
                          );
                        },
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4.0),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 80,
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
}
