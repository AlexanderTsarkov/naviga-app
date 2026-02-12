# Map fit — guided manual test (Android device 988a1b35434c524b53)

Harness to capture MAPFIT logs and reproduce fit/visibility issues.

## Step-by-step checklist

1. **Run the app in debug**
   ```bash
   ./tools/run_android_debug.sh
   ```
   (Or: `cd app && flutter run -d 988a1b35434c524b53 --debug`)

2. **In another terminal, start log capture**
   ```bash
   ./tools/watch_mapfit.sh
   ```
   This clears logcat and streams `flutter:I` into `tools/mapfit_live.log`. The script prints:
   > Now: open app, go to Nodes -> Refresh, then open Map tab.

3. **Do the UI steps**
   - Open the app (if not already).
   - Go to **Nodes** → **Refresh**.
   - Open **Map** tab.
   - Optionally: use **Show on map** from Nodes list or Node Details.
   - Switch to another tab, then back to **Map**.

4. **Stop log capture** (Ctrl+C in the `watch_mapfit.sh` terminal).

5. **Paste back the MAPFIT lines** from `tools/mapfit_live.log` (or console):
   - `MAPFIT reason=...` (firstOpen / tabOpen / focusSet / focusCleared)
   - `MAPFIT forFit nodeId=...`
   - `MAPFIT bounds south=... spanM=...`

## Device override

To use another device, set `MAPFIT_DEVICE_ID`:

```bash
export MAPFIT_DEVICE_ID=your_device_id
./tools/run_android_debug.sh
./tools/watch_mapfit.sh
```

## Reason meanings

- **firstOpen** — first time map became ready (onMapReady).
- **tabOpen** — user opened Map tab (with no focus node); fit runs so first Map entry always emits a log.
- **focusSet** — “Show on map” was used; map centers on that node.
- **focusCleared** — focus was cleared (e.g. user tapped Map in nav); fit to active markers.
