import 'package:flutter/material.dart';

import '../../shared/widgets/placeholder_screen.dart';

class ConnectScreen extends StatelessWidget {
  const ConnectScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const PlaceholderScreen(
      title: 'Connect',
      subtitle: 'TODO: BLE permissions, scan list, connect state machine.',
    );
  }
}
