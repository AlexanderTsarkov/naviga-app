# PR Body (paste into GitHub PR description)

```
Fixes #89

## Implemented behavior
- **Map tab** with flutter_map and online OSM tiles (north-up only).
- **Markers** from NodeTable with `pos_valid` and sane coords (lat/lon in range); self marker visually distinct (larger, primary color).
- **Fit behavior:** 2+ points → fitBounds with padding (min 96px) + conservative zoom-out margin (0.75); 1 point → center at zoom 17. Debounce (250ms) avoids tab/focus races.
- **Show on map** from Nodes list, Node Details, and My Node (when self has valid position).
- **Manual refresh only** (no auto updates); map does not change Connect UX.

## Manual test checklist (Android)
1. Cold start → Connect → pair/connect to device.
2. Nodes → Refresh. Wait for NodeTable with at least one node that has valid position.
3. Open Map tab. Both nodes (if 2+) visible without manual zoom; single node centered at zoom 17.
4. From Nodes list or Node Details: tap "Show on map" → Map tab opens centered on that node; all markers still visible.
5. From My Node (with valid pos): tap "Show on map" → Map tab centers on self.
6. Switch to another tab, then back to Map → fit runs again (focus cleared), markers visible.
```
