import 'package:flutter/material.dart';

import '../../shared/widgets/placeholder_screen.dart';

class MyNodeScreen extends StatelessWidget {
  const MyNodeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const PlaceholderScreen(
      title: 'My Node',
      subtitle: 'TODO: Show self telemetry from DeviceInfo/Health.',
    );
  }
}
