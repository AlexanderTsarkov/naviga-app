import 'package:flutter_test/flutter_test.dart';

import 'package:naviga_app/features/connect/connect_controller.dart';

/// S04 Slice 1: BLE contract version compatibility (exact match required).
void main() {
  group('NavigaScanDevice BLE contract version', () {
    test('compatible when version is 1.0', () {
      final device = NavigaScanDevice(
        id: 'id',
        name: 'Naviga',
        rssi: -60,
        lastSeen: DateTime(2026),
        bleContractVersionMajor: 1,
        bleContractVersionMinor: 0,
      );
      expect(device.isBleContractVersionCompatible, isTrue);
    });

    test('incompatible when major differs', () {
      final device = NavigaScanDevice(
        id: 'id',
        name: 'Naviga',
        rssi: -60,
        lastSeen: DateTime(2026),
        bleContractVersionMajor: 2,
        bleContractVersionMinor: 0,
      );
      expect(device.isBleContractVersionCompatible, isFalse);
    });

    test('incompatible when minor differs', () {
      final device = NavigaScanDevice(
        id: 'id',
        name: 'Naviga',
        rssi: -60,
        lastSeen: DateTime(2026),
        bleContractVersionMajor: 1,
        bleContractVersionMinor: 1,
      );
      expect(device.isBleContractVersionCompatible, isFalse);
    });

    test('incompatible when version is null (legacy or missing)', () {
      final device = NavigaScanDevice(
        id: 'id',
        name: 'Naviga',
        rssi: -60,
        lastSeen: DateTime(2026),
      );
      expect(device.isBleContractVersionCompatible, isFalse);
    });

    test('expected version constants match 1.0', () {
      expect(kBLEContractVersionMajor, 1);
      expect(kBLEContractVersionMinor, 0);
    });
  });
}
