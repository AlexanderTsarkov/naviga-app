import 'package:flutter/material.dart';

import '../../shared/widgets/placeholder_screen.dart';

class NodesScreen extends StatelessWidget {
  const NodesScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const PlaceholderScreen(
      title: 'Nodes',
      subtitle: 'TODO: NodeTable list + tap to Node Details.',
    );
  }
}
