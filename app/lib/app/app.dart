import 'package:flutter/material.dart';

import 'app_shell.dart';

/// Naviga — автономная навигация и связь.
/// Android-first каркас для Mobile v1.
class NavigaApp extends StatelessWidget {
  const NavigaApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Naviga',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const AppShell(),
    );
  }
}
