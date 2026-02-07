import 'package:flutter_riverpod/legacy.dart';

import '../shared/app_tabs.dart';

final selectedTabProvider = StateProvider<AppTab>((ref) => AppTab.connect);
