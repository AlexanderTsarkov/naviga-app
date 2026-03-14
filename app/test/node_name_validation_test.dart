import 'dart:convert';

import 'package:flutter_test/flutter_test.dart';
import 'package:naviga_app/features/connect/connect_controller.dart';

void main() {
  group('ConnectController node name (S04 #466)', () {
    test('truncateNodeNameToLimit: max 12 code points', () {
      final long = 'abcdefghijklmnop';
      expect(
        ConnectController.truncateNodeNameToLimit(long).runes.length,
        lessThanOrEqualTo(12),
      );
      expect(ConnectController.truncateNodeNameToLimit(long), 'abcdefghijkl');
    });

    test(
      'truncateNodeNameToLimit: max 24 UTF-8 bytes, does not split multibyte',
      () {
        // 12 Cyrillic chars = 24 bytes UTF-8; 13th would be 26 bytes
        const cyrillic12 = 'абвгдежзийкл';
        expect(utf8.encode(cyrillic12).length, 24);
        expect(
          ConnectController.truncateNodeNameToLimit(cyrillic12),
          cyrillic12,
        );
        const cyrillic13 = 'абвгдежзийклм';
        expect(utf8.encode(cyrillic13).length, 26);
        final truncated = ConnectController.truncateNodeNameToLimit(cyrillic13);
        expect(utf8.encode(truncated).length, lessThanOrEqualTo(24));
        expect(truncated.runes.length, lessThanOrEqualTo(12));
        // Must be valid UTF-8 (no split code point)
        expect(() => utf8.decode(utf8.encode(truncated)), returnsNormally);
      },
    );

    test('isNodeNameAllowed: accepts Latin, digits, allowed symbols', () {
      expect(ConnectController.isNodeNameAllowed('Node1'), true);
      expect(ConnectController.isNodeNameAllowed('A-B_C#=@+'), true);
      expect(ConnectController.isNodeNameAllowed('123'), true);
    });

    test('isNodeNameAllowed: accepts Cyrillic', () {
      expect(ConnectController.isNodeNameAllowed('Узел'), true);
      expect(ConnectController.isNodeNameAllowed('Мой_узел'), true);
    });

    test('isNodeNameAllowed: rejects control characters', () {
      expect(ConnectController.isNodeNameAllowed('Node\x01'), false);
      expect(ConnectController.isNodeNameAllowed('A\tB'), false);
    });

    test('isNodeNameAllowed: rejects emoji and disallowed symbols', () {
      expect(ConnectController.isNodeNameAllowed('Node😀'), false);
      expect(ConnectController.isNodeNameAllowed('Node!'), false);
      expect(ConnectController.isNodeNameAllowed('Node.'), false);
      expect(ConnectController.isNodeNameAllowed('Node '), false);
    });

    test('validateAndTruncateNodeName: empty trim returns empty', () {
      expect(ConnectController.validateAndTruncateNodeName('  '), '');
    });

    test(
      'validateAndTruncateNodeName: valid name returns truncated string',
      () {
        expect(
          ConnectController.validateAndTruncateNodeName('MyNode'),
          'MyNode',
        );
        expect(
          ConnectController.validateAndTruncateNodeName('  MyNode  '),
          'MyNode',
        );
      },
    );

    test('validateAndTruncateNodeName: invalid characters return null', () {
      expect(ConnectController.validateAndTruncateNodeName('Node!'), null);
      expect(ConnectController.validateAndTruncateNodeName('Node😀'), null);
    });

    test(
      'truncateNodeNameToLimit: 24-byte boundary never leaves invalid UTF-8',
      () {
        // 11 Cyrillic (22 bytes) + € U+20AC (3 bytes) = 25 bytes; truncate at 24 must not split
        const input = 'абвгдежзийк\u20AC';
        expect(utf8.encode(input).length, 25);
        final truncated = ConnectController.truncateNodeNameToLimit(input);
        expect(utf8.encode(truncated).length, lessThanOrEqualTo(24));
        expect(() => utf8.decode(utf8.encode(truncated)), returnsNormally);
      },
    );

    test('validateAndTruncateNodeName: 11 Cyrillic + emoji never throws', () {
      const input = 'абвгдежзийк😀';
      expect(
        () => ConnectController.validateAndTruncateNodeName(input),
        returnsNormally,
      );
    });

    test('validateAndTruncateNodeName: never throws for any input', () {
      expect(
        () => ConnectController.validateAndTruncateNodeName('Valid'),
        returnsNormally,
      );
      expect(
        () => ConnectController.validateAndTruncateNodeName('Нода!'),
        returnsNormally,
      );
      expect(
        () => ConnectController.validateAndTruncateNodeName(''),
        returnsNormally,
      );
    });
  });
}
