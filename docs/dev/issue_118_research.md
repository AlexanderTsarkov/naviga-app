# Issue #118 Research: Node Details screen

## 1) Nodes list implementation

- **Screen:** `app/lib/features/nodes/nodes_screen.dart` — ConsumerStatefulWidget, shows `nodesState.recordsSorted` in a SliverList of ListTile (nodeId, shortId, lastSeenAgeS, isSelf).
- **Controller:** `app/lib/features/nodes/nodes_controller.dart` — NodesController holds NodesState (records, recordsSorted, self, isLoading, error, …). No per-node selection state.
- **State:** `app/lib/features/nodes/nodes_state.dart` — records, recordsSorted (List<NodeRecordV1>), self (NodeRecordV1?).

## 2) NodeRecordV1 fields

- **Defined in:** `app/lib/features/connect/connect_controller.dart` (class NodeRecordV1).
- **Fields:** nodeId (int), shortId (int), isSelf (bool), posValid (bool), isGrey (bool), shortIdCollision (bool), lastSeenAgeS (int), latE7 (int), lonE7 (int), posAgeS (int), lastRxRssi (int), lastSeq (int).
- **Stable key for details:** nodeId (int) — unique per node; use it to look up record in recordsSorted after refresh.

## 3) Navigation / routing

- **App structure:** `MaterialApp(home: AppShell())` — no go_router. Tabs via IndexedStack + selectedTabProvider (AppTab: connect, myNode, nodes, map, settings).
- **Place for Node Details:** Push a new route on top of the stack: `Navigator.push(context, MaterialPageRoute(builder: (_) => NodeDetailsScreen(nodeId: nodeId)))`. Details screen has its own AppBar with back; no need to change app_shell or add a new tab.
- **Issue #118 rule:** Tap **local node** (isSelf) → open **My Node** tab (`ref.read(selectedTabProvider.notifier).state = AppTab.myNode`). Tap **other node** → push **Node Details** with nodeId.

## 4) Where to add the new screen and wiring

- **New file:** `app/lib/features/nodes/node_details_screen.dart` — screen that takes `nodeId` (int), uses ref.watch(nodesControllerProvider) to get recordsSorted, finds record by nodeId; shows Key: Value list of all NodeRecordV1 fields; Refresh calls nodesController.refresh(); not connected → message + Go to Connect; node not found → "Node not found (table updated)" + back.
- **Nodes screen change:** Wrap ListTile in InkWell/onTap: if record.isSelf → switch to My Node tab; else → Navigator.push to NodeDetailsScreen(nodeId: record.nodeId).
