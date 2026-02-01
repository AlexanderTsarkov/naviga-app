import 'package:flutter/material.dart';

void main() {
  runApp(const NavigaApp());
}

/// Naviga — автономная навигация и связь.
/// Каркас приложения: код добавляется шаг за шагом.
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
      home: const _HomePage(),
    );
  }
}

class _HomePage extends StatelessWidget {
  const _HomePage();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Naviga'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: const Center(
        child: Text('Naviga — чистый лист. Код добавляется шаг за шагом.'),
      ),
    );
  }
}
