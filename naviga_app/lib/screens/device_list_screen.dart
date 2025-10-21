import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

import '../services/meshtastic_service.dart';
import 'device_detail_screen.dart';

class DeviceListScreen extends StatefulWidget {
  const DeviceListScreen({super.key});

  @override
  State<DeviceListScreen> createState() => _DeviceListScreenState();
}

class _DeviceListScreenState extends State<DeviceListScreen> {
  final MeshtasticService _service = MeshtasticService();
  List<ScanResult> _results = const [];
  bool _scanning = false;
  String _status = 'Нажмите Refresh для поиска устройств';

  @override
  void initState() {
    super.initState();
    _service.scanResultsStream.listen((r) => setState(() => _results = r));
    _checkPermissions();
  }

  Future<void> _checkPermissions() async {
    // Проверяем разрешения
    final bluetooth = await Permission.bluetoothConnect.status;
    final location = await Permission.location.status;
    
    if (!bluetooth.isGranted || !location.isGranted) {
      setState(() => _status = 'Запрашиваем разрешения...');
      await _requestPermissions();
    } else {
      setState(() => _status = 'Разрешения получены. Нажмите Refresh.');
    }
  }

  Future<void> _requestPermissions() async {
    final permissions = [
      Permission.bluetoothConnect,
      Permission.bluetoothScan,
      Permission.location,
    ];
    
    final statuses = await permissions.request();
    
    if (statuses.values.every((status) => status.isGranted)) {
      setState(() => _status = 'Разрешения получены. Нажмите Refresh.');
    } else {
      setState(() => _status = 'Нужны разрешения для работы с Bluetooth');
    }
  }

  Future<void> _start() async {
    setState(() {
      _scanning = true;
      _status = 'Поиск устройств...';
    });
    
    try {
      await _service.startScan();
      setState(() => _status = 'Поиск завершен');
    } catch (e) {
      setState(() => _status = 'Ошибка: $e');
    } finally {
      setState(() => _scanning = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Meshtastic Devices')),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(16.0),
            child: Text(_status, textAlign: TextAlign.center),
          ),
          Expanded(
            child: _results.isEmpty
                ? Center(
                    child: _scanning
                        ? const CircularProgressIndicator()
                        : const Text('Устройства не найдены'),
                  )
                : ListView.separated(
                    itemCount: _results.length,
                    separatorBuilder: (_, __) => const Divider(height: 1),
                    itemBuilder: (context, i) {
                      final r = _results[i];
                      return ListTile(
                        leading: const Icon(Icons.bluetooth),
                        title: Text(r.advertisementData.advName.isNotEmpty
                            ? r.advertisementData.advName
                            : r.device.remoteId.str),
                        subtitle: Text('RSSI: ${r.rssi}'),
                        onTap: () {
                          Navigator.push(
                            context,
                            MaterialPageRoute(
                              builder: (context) => DeviceDetailScreen(device: r.device),
                            ),
                          );
                        },
                      );
                    },
                  ),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _scanning ? null : _start,
        child: _scanning 
            ? const CircularProgressIndicator(color: Colors.white)
            : const Icon(Icons.refresh),
      ),
    );
  }
}


