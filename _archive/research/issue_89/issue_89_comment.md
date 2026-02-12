# Issue #89 — Final summary comment (paste as issue comment)

```
## Map v1 — Summary for merge

### What changed
- **Map tab** added with flutter_map and online OSM tiles (north-up). Markers for NodeTable nodes with `pos_valid` and sane coordinates; self marker highlighted.
- **Fit behavior:** Direct Map open or return to Map fits active nodes (lastSeen ≤ 25s): 2+ points use fitBounds with padding and a post-fit zoom-out margin; 1 point centers at zoom 17. Show-on-map from Nodes list, Node Details, and My Node switches to Map and centers on the selected node (all markers still shown).
- **Navigation:** No Connect UX changes; map is manual-refresh only (NodeTable refresh from Nodes/My Node).

### Bug found during testing
- **Symptom:** After Connect → Nodes refresh → Map, the map briefly showed ocean-scale zoom, then zoomed to ~15.47 but **both nodes were not visible**; manual zoom out to ~14.7 was needed to see them.
- **Cause:** Fit was too tight (exact bounds + padding left no margin for marker size / rounding).
- **Fix:** (1) Min padding increased from 48px to 96px. (2) After fitBounds for 2+ points, apply an extra zoom-out margin: `move(center, zoom - 0.75)` clamped to minZoom 3. (3) Debounce (250ms + generation token) so only the latest fit runs (avoids tabOpen/onMapReady/focus races). Debug overlay removed for main; kDebugMode MAPFIT summary logs kept for diagnostics.

### How verified
- Cold start → Connect → Nodes refresh → Map: both nodes visible without manual zoom.
- Tab switch away and back to Map: fit runs again, markers visible.
- Show on map from list/Node Details/My Node: Map opens centered on selected node, all markers visible.
- `dart format`, `flutter analyze`, `flutter test` (11 passed).
```
