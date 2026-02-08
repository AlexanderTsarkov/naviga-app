import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';

import 'package:naviga_app/features/connect/connect_controller.dart';

Uint8List _buildRecord({
  required int nodeId,
  required int shortId,
  required int flags,
  required int lastSeenAgeS,
  required int latE7,
  required int lonE7,
  required int posAgeS,
  required int lastRxRssi,
  required int lastSeq,
}) {
  final data = ByteData(26);
  data.setUint64(0, nodeId, Endian.little);
  data.setUint16(8, shortId, Endian.little);
  data.setUint8(10, flags);
  data.setUint16(11, lastSeenAgeS, Endian.little);
  data.setInt32(13, latE7, Endian.little);
  data.setInt32(17, lonE7, Endian.little);
  data.setUint16(21, posAgeS, Endian.little);
  data.setInt8(23, lastRxRssi);
  data.setUint16(24, lastSeq, Endian.little);
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
  test('parses header + single record', () {
    final record = _buildRecord(
      nodeId: 0x0102030405060708,
      shortId: 0xBEEF,
      flags: 0x08,
      lastSeenAgeS: 42,
      latE7: 123456789,
      lonE7: -123456789,
      posAgeS: 7,
      lastRxRssi: -72,
      lastSeq: 99,
    );
    final payload = _buildPayload(
      snapshotId: 10,
      totalNodes: 1,
      pageIndex: 0,
      pageSize: 10,
      pageCount: 1,
      recordFormatVer: 1,
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
    expect(page.header.recordFormatVer, 1);

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
    expect(r.lastSeq, 99);
  });

  test('fails on short payload', () {
    final payload = Uint8List.fromList(List<int>.filled(9, 0));
    final result = BleNodeTableParser.parsePage(payload);
    expect(result.data, isNull);
    expect(result.warning, contains('too short'));
  });

  test('fails on invalid record length', () {
    final payload = Uint8List.fromList(List<int>.filled(11, 0));
    final result = BleNodeTableParser.parsePage(payload);
    expect(result.data, isNull);
    expect(result.warning, contains('size invalid'));
  });
}
