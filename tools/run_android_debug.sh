#!/usr/bin/env bash
# Run Flutter app in debug on Android device for Map fit testing.
# Device: 988a1b35434c524b53 (Android)
set -e
DEVICE_ID="${MAPFIT_DEVICE_ID:-988a1b35434c524b53}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT/app"

flutter run -d "$DEVICE_ID" --debug
