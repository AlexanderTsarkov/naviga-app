import 'package:flutter/material.dart';

import '../../shared/widgets/placeholder_screen.dart';

class MapScreen extends StatelessWidget {
  const MapScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const PlaceholderScreen(
      title: 'Map',
      subtitle: 'TODO: flutter_map + markers (online tiles only).',
    );
  }
}
