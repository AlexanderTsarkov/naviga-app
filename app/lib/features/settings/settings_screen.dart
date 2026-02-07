import 'package:flutter/material.dart';

import '../../shared/widgets/placeholder_screen.dart';

class SettingsScreen extends StatelessWidget {
  const SettingsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const PlaceholderScreen(
      title: 'Settings',
      subtitle: 'TODO: About + diagnostics (no BLE config in v1).',
    );
  }
}
