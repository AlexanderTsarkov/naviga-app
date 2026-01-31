import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:naviga_app/main.dart';

void main() {
  testWidgets('Naviga app smoke test', (WidgetTester tester) async {
    await tester.pumpWidget(const NavigaApp());
    expect(find.text('Naviga'), findsWidgets);
  });
}
