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
      });
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
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),
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
      floatingActionButton: FloatingActionButton(
        onPressed: _connect,
        backgroundColor: Colors.blue,
        child: const Icon(Icons.info),
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
}
