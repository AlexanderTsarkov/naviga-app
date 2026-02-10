import 'package:flutter_riverpod/legacy.dart';

import '../shared/app_tabs.dart';

final selectedTabProvider = StateProvider<AppTab>((ref) => AppTab.connect);

/// When non-null, Map tab centers on this node (e.g. after "Show on map").
/// When null, Map tab fits all markers. Cleared when user opens Map tab from nav.
final mapFocusNodeIdProvider = StateProvider<int?>((ref) => null);
