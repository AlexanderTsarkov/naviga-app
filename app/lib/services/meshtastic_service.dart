import 'dart:async';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class MeshtasticService {
  MeshtasticService();

  final StreamController<List<ScanResult>> _scanResultsController =
      StreamController<List<ScanResult>>.broadcast();

  Stream<List<ScanResult>> get scanResultsStream => _scanResultsController.stream;

  Future<void> startScan({Duration timeout = const Duration(seconds: 8)}) async {
    await FlutterBluePlus.startScan(timeout: timeout);
    final subscription = FlutterBluePlus.scanResults.listen((results) {
      final filtered = results.where(_isLikelyMeshtastic).toList();
      _scanResultsController.add(filtered);
    });
    // Ensure subscription is cancelled after timeout
    Future.delayed(timeout + const Duration(seconds: 1), () => subscription.cancel());
  }

  Future<void> stopScan() async {
    await FlutterBluePlus.stopScan();
  }

  bool _isLikelyMeshtastic(ScanResult r) {
    final name = r.advertisementData.advName.toLowerCase();
    return name.contains('meshtastic') || name.contains('t-beam') || name.contains('ttgo');
  }
}


