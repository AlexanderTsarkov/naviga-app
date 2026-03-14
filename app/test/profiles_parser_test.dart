import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';

import 'package:naviga_app/features/connect/profiles_parser.dart';

void main() {
  group('BleProfilesParser.parseList', () {
    test('parses list with 1 radio and 3 user ids', () {
      // n_radio=1, radio_id=0, n_user=3, user_ids=0,1,2. LE.
      final bytes = <int>[1, 0, 0, 0, 0, 3, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0];
      final result = BleProfilesParser.parseList(bytes);
      expect(result, isNotNull);
      expect(result!.radioIds, [0]);
      expect(result.userIds, [0, 1, 2]);
    });

    test('returns null for too short payload', () {
      expect(BleProfilesParser.parseList([]), isNull);
      expect(BleProfilesParser.parseList([1]), isNull);
    });
  });

  group('BleProfilesParser.parseProfileResponse', () {
    test('parses radio profile (type 0)', () {
      // profile_id=0, kind=0, channel_slot=1, rate_tier=2, tx=0, label_len=0 (min 10 bytes)
      final bytes = [0, 0, 0, 0, 0, 1, 2, 0, 0, 0];
      final result = BleProfilesParser.parseProfileResponse(0, bytes);
      expect(result, isA<ParsedProfileRadio>());
      final r = (result as ParsedProfileRadio).data;
      expect(r.profileId, 0);
      expect(r.kind, 0);
      expect(r.channelSlot, 1);
      expect(r.rateTier, 2);
      expect(r.txPowerBaselineStep, 0);
      expect(r.label, '');
    });

    test('parses role profile (type 1)', () {
      // role_id=0, min_interval_sec=22, max_silence_10s=11, min_displacement_m=30.0
      final buffer = ByteData(11);
      buffer.setUint32(0, 0, Endian.little);
      buffer.setUint16(4, 22, Endian.little);
      buffer.setUint8(6, 11);
      buffer.setFloat32(7, 30.0, Endian.little);
      final bytes = buffer.buffer.asUint8List().toList();
      final result = BleProfilesParser.parseProfileResponse(1, bytes);
      expect(result, isA<ParsedProfileRole>());
      final r = (result as ParsedProfileRole).data;
      expect(r.roleId, 0);
      expect(r.minIntervalSec, 22);
      expect(r.maxSilence10s, 11);
      expect(r.minDisplacementM, closeTo(30.0, 0.01));
    });

    test('returns null for invalid type or short payload', () {
      expect(BleProfilesParser.parseProfileResponse(0, [1, 2, 3]), isNull);
      expect(BleProfilesParser.parseProfileResponse(1, [1, 2, 3]), isNull);
    });
  });
}
