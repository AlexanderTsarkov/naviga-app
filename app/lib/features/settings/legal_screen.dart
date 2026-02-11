import 'package:flutter/material.dart';

import 'settings_about_text.dart';

class LegalScreen extends StatelessWidget {
  const LegalScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Legal / Ответственность пользователя')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          SelectableText(
            aboutLegal,
            style: Theme.of(context).textTheme.bodyLarge,
          ),
        ],
      ),
    );
  }
}
