import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:naviga_app/app/app.dart';

void main() {
  testWidgets('Naviga app smoke test', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: NavigaApp()));

    expect(find.text('Connect'), findsWidgets);
    expect(find.text('My Node'), findsWidgets);
    expect(find.text('Nodes'), findsWidgets);
    expect(find.text('Map'), findsWidgets);
    expect(find.text('Settings'), findsWidgets);
  });
}
