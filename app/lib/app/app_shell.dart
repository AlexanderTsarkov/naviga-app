import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../features/connect/connect_screen.dart';
import '../features/map/map_screen.dart';
import '../features/my_node/my_node_screen.dart';
import '../features/nodes/nodes_screen.dart';
import '../features/nodes/nodes_controller.dart';
import '../features/settings/settings_screen.dart';
import '../shared/app_tabs.dart';
import 'app_state.dart';

class AppShell extends ConsumerWidget {
  const AppShell({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    // Always instantiate so debug hook can be set when kDebugFetchNodeTableOnConnect is true.
    ref.watch(nodesControllerProvider);
    final currentTab = ref.watch(selectedTabProvider);

    return Scaffold(
      appBar: AppBar(
        title: Text(currentTab.label),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: IndexedStack(
        index: currentTab.index,
        children: const [
          ConnectScreen(),
          MyNodeScreen(),
          NodesScreen(),
          MapScreen(),
          SettingsScreen(),
        ],
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: currentTab.index,
        onDestinationSelected: (index) {
          final tab = AppTab.values[index];
          if (tab == AppTab.map) {
            ref.read(mapFocusNodeIdProvider.notifier).state = null;
          }
          ref.read(selectedTabProvider.notifier).state = tab;
        },
        destinations: [
          for (final tab in AppTab.values)
            NavigationDestination(icon: Icon(tab.icon), label: tab.label),
        ],
      ),
    );
  }
}
