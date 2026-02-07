import 'package:flutter/material.dart';

enum AppTab { connect, myNode, nodes, map, settings }

extension AppTabMeta on AppTab {
  String get label {
    switch (this) {
      case AppTab.connect:
        return 'Connect';
      case AppTab.myNode:
        return 'My Node';
      case AppTab.nodes:
        return 'Nodes';
      case AppTab.map:
        return 'Map';
      case AppTab.settings:
        return 'Settings';
    }
  }

  IconData get icon {
    switch (this) {
      case AppTab.connect:
        return Icons.bluetooth_connected;
      case AppTab.myNode:
        return Icons.person_pin_circle;
      case AppTab.nodes:
        return Icons.hub;
      case AppTab.map:
        return Icons.map;
      case AppTab.settings:
        return Icons.settings;
    }
  }
}
