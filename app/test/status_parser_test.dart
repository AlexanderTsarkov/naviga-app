import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:naviga_app/features/connect/status_parser.dart';

void main() {
  test('parses GNSS TLV sample with age=10', () {
    final payload = Uint8List.fromList([
      0x01, 0x00, 0x00, 0x00, // header
      0x01, 0x04, // GNSS TLV: type=1, len=4
      0x02, 0x01, 0x0A, 0x00, // state=FIX_3D, valid=1, age=10
    ]);

    final result = BleStatusParser.parse(payload);
    expect(result.warning, isNull);
    expect(result.data, isNotNull);
    expect(result.data!.formatVer, 1);
    expect(result.data!.gnss, isNotNull);
    expect(result.data!.gnss!.stateLabel, 'FIX_3D');
    expect(result.data!.gnss!.posValid, isTrue);
    expect(result.data!.gnss!.posAgeS, 10);
  });

  test('parses GNSS TLV sample with age=12', () {
    final payload = Uint8List.fromList([
      0x01, 0x00, 0x00, 0x00, // header
      0x01, 0x04, // GNSS TLV: type=1, len=4
      0x02, 0x01, 0x0C, 0x00, // state=FIX_3D, valid=1, age=12
    ]);

    final result = BleStatusParser.parse(payload);
    expect(result.warning, isNull);
    expect(result.data, isNotNull);
    expect(result.data!.gnss, isNotNull);
    expect(result.data!.gnss!.posAgeS, 12);
  });

  test('skips unknown TLV and still parses GNSS TLV', () {
    final payload = Uint8List.fromList([
      0x01, 0x00, 0x00, 0x00, // header
      0x63, 0x03, 0xAA, 0xBB, 0xCC, // unknown TLV type=99, len=3
      0x01, 0x04, // GNSS TLV: type=1, len=4
      0x00, 0x00, 0x05, 0x00, // state=NO_FIX, valid=0, age=5
    ]);

    final result = BleStatusParser.parse(payload);
    expect(result.data, isNotNull);
    expect(result.data!.gnss, isNotNull);
    expect(result.data!.gnss!.stateLabel, 'NO_FIX');
    expect(result.data!.gnss!.posValid, isFalse);
    expect(result.data!.gnss!.posAgeS, 5);
  });
}
