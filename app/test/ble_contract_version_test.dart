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

  group(
    'parseBleContractVersionFromManufacturerPayload (payload = [major, minor])',
    () {
      test('returns (1, 0) for 2-byte payload [1, 0]', () {
        expect(
          ConnectController.parseBleContractVersionFromManufacturerPayload([
            1,
            0,
          ]),
          (1, 0),
        );
      });

      test('returns (2, 1) for incompatible payload [2, 1]', () {
        expect(
          ConnectController.parseBleContractVersionFromManufacturerPayload([
            2,
            1,
          ]),
          (2, 1),
        );
      });

      test('returns null for null manufacturer data', () {
        expect(
          ConnectController.parseBleContractVersionFromManufacturerPayload(
            null,
          ),
          isNull,
        );
      });

      test('returns null for empty list', () {
        expect(
          ConnectController.parseBleContractVersionFromManufacturerPayload([]),
          isNull,
        );
      });

      test('returns null for single byte (missing minor)', () {
        expect(
          ConnectController.parseBleContractVersionFromManufacturerPayload([1]),
          isNull,
        );
      });
    },
  );
}
