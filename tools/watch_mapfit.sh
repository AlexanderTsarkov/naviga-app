#!/usr/bin/env bash
# Capture flutter logcat for Map fit debugging. Run from repo root or tools/.
# Device: 988a1b35434c524b53 (Android)
set -e
DEVICE_ID="${MAPFIT_DEVICE_ID:-988a1b35434c524b53}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

adb -s "$DEVICE_ID" logcat -c
echo "Logcat cleared. Capturing flutter logs to tools/mapfit_live.log ..."
echo ""
echo "  Now: open app, go to Nodes -> Refresh, then open Map tab."
echo ""
adb -s "$DEVICE_ID" logcat -s flutter:I | tee tools/mapfit_live.log
