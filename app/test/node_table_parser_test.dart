import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';

import 'package:naviga_app/features/connect/connect_controller.dart';

/// S04 #464: Build one canon BLE record (72 bytes, format ver 2).
Uint8List _buildRecordFormat2({
  required int nodeId,
  required int shortId,
  int flags1 = 0,
  required int lastSeenAgeS,
  required int latE7,
  required int lonE7,
  required int posAgeS,
  int flags2 = 0,
  int posFlags = 0,
  int sats = 0,
  int flags3 = 0,
  int batteryPercent = 0xff,
  int uptimeSec = 0xffffffff,
  int maxSilence10s = 0,
  int hwProfileId = 0xffff,
  int fwVersionId = 0xffff,
  required int lastRxRssi,
  required int snrLast,
  String nodeName = '',
}) {
  final data = ByteData(kNodeRecordBytesBleCanon);
  data.setUint64(0, nodeId, Endian.little);
  data.setUint16(8, shortId, Endian.little);
  data.setUint8(10, flags1);
  data.setUint16(11, lastSeenAgeS, Endian.little);
  data.setUint32(13, latE7.toUnsigned(32), Endian.little);
  data.setUint32(17, lonE7.toUnsigned(32), Endian.little);
  data.setUint16(21, posAgeS, Endian.little);
  data.setUint8(23, flags2);
  data.setUint8(24, posFlags);
  data.setUint8(25, sats);
  data.setUint8(26, flags3);
  data.setUint8(27, batteryPercent);
  data.setUint32(28, uptimeSec, Endian.little);
  data.setUint8(32, maxSilence10s);
  data.setUint16(33, hwProfileId, Endian.little);
  data.setUint16(35, fwVersionId, Endian.little);
  data.setInt8(37, lastRxRssi);
  data.setInt8(38, snrLast);
  final nameBytes = nodeName.isEmpty ? 0 : nodeName.length.clamp(0, 32);
  data.setUint8(39, nameBytes);
  for (var i = 0; i < 32; i++) {
    data.setUint8(40 + i, i < nodeName.length ? nodeName.codeUnitAt(i) : 0);
  }
  return data.buffer.asUint8List();
}

Uint8List _buildPayload({
  required int snapshotId,
  required int totalNodes,
  required int pageIndex,
  required int pageSize,
  required int pageCount,
  required int recordFormatVer,
  required List<Uint8List> records,
}) {
  final header = ByteData(10);
  header.setUint16(0, snapshotId, Endian.little);
  header.setUint16(2, totalNodes, Endian.little);
  header.setUint16(4, pageIndex, Endian.little);
  header.setUint8(6, pageSize);
  header.setUint16(7, pageCount, Endian.little);
  header.setUint8(9, recordFormatVer);

  final bytes = <int>[];
  bytes.addAll(header.buffer.asUint8List());
  for (final record in records) {
    bytes.addAll(record);
  }
  return Uint8List.fromList(bytes);
}

void main() {
  test('parses header + single record (format 2)', () {
    final record = _buildRecordFormat2(
      nodeId: 0x0102030405060708,
      shortId: 0xBEEF,
      flags1: 0x0a,
      lastSeenAgeS: 42,
      latE7: 123456789,
      lonE7: -123456789,
      posAgeS: 7,
      lastRxRssi: -72,
      snrLast: -5,
      nodeName: 'Node1',
    );
    final payload = _buildPayload(
      snapshotId: 10,
      totalNodes: 1,
      pageIndex: 0,
      pageSize: 10,
      pageCount: 1,
      recordFormatVer: 2,
      records: [record],
    );

    final result = BleNodeTableParser.parsePage(payload);
    expect(result.data, isNotNull);
    expect(result.warning, isNull);

    final page = result.data!;
    expect(page.header.snapshotId, 10);
    expect(page.header.totalNodes, 1);
    expect(page.header.pageIndex, 0);
    expect(page.header.pageSize, 10);
    expect(page.header.pageCount, 1);
    expect(page.header.recordFormatVer, 2);

    expect(page.records.length, 1);
    final r = page.records.first;
    expect(r.nodeId, 0x0102030405060708);
    expect(r.shortId, 0xBEEF);
    expect(r.shortIdCollision, isTrue);
    expect(r.lastSeenAgeS, 42);
    expect(r.latE7, 123456789);
    expect(r.lonE7, -123456789);
    expect(r.posAgeS, 7);
    expect(r.lastRxRssi, -72);
    expect(r.lastSeq, 0);
    expect(r.nodeName, 'Node1');
    expect(r.snrLast, -5);
  });

  test('fails on short payload', () {
    final payload = Uint8List.fromList(List<int>.filled(9, 0));
    final result = BleNodeTableParser.parsePage(payload);
    expect(result.data, isNull);
    expect(result.warning, contains('too short'));
  });

  test('fails on invalid record length (format 2)', () {
    final header = ByteData(10);
    header.setUint8(9, 2);
    final payload = Uint8List.fromList([...header.buffer.asUint8List(), 0]);
    final result = BleNodeTableParser.parsePage(payload);
    expect(result.data, isNull);
    expect(result.warning, contains('invalid'));
  });
}
