import 'package:flutter/material.dart';

import 'settings_about_text.dart';

class AboutExtendedScreen extends StatelessWidget {
  const AboutExtendedScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Подробнее о проекте')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          SelectableText(
            aboutExtendedDescription,
            style: Theme.of(context).textTheme.bodyLarge,
          ),
        ],
      ),
    );
  }
}
