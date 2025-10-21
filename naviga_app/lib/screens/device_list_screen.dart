import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../services/meshtastic_service.dart';

class DeviceListScreen extends StatefulWidget {
  const DeviceListScreen({super.key});

  @override
  State<DeviceListScreen> createState() => _DeviceListScreenState();
}

class _DeviceListScreenState extends State<DeviceListScreen> {
  final MeshtasticService _service = MeshtasticService();
  List<ScanResult> _results = const [];
  bool _scanning = false;

  @override
  void initState() {
    super.initState();
    _service.scanResultsStream.listen((r) => setState(() => _results = r));
    _start();
  }

  Future<void> _start() async {
    setState(() => _scanning = true);
    await _service.startScan();
    setState(() => _scanning = false);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Meshtastic Devices')),
      body: _results.isEmpty
          ? Center(
              child: _scanning
                  ? const CircularProgressIndicator()
                  : const Text('No devices found. Tap refresh.'),
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
                    // TODO: navigate to details/connect
                  },
                );
              },
            ),
      floatingActionButton: FloatingActionButton(
        onPressed: _start,
        child: const Icon(Icons.refresh),
      ),
    );
  }
}


