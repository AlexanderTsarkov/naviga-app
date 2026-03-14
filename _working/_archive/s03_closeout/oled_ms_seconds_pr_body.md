## Summary

**Display-only fix.** OLED line 3 previously showed raw `max_silence_10s` values (e.g. 3, 9) while MI and MD were already in human-readable units (seconds, meters). This was inconsistent and could be misleading.

This PR keeps internal/runtime semantics unchanged and only converts `max_silence_10s` to seconds for OLED display (e.g. 30, 90), so the screen is consistent.

## Changes

- **`firmware/src/services/oled_status.cpp`:** When rendering `MS:`, multiply `data.max_silence_10s` by 10 for display. Comment added to clarify that the field is stored in 10-second steps but shown in seconds.

## What is unchanged

- Storage, NVS, provisioning: still `max_silence_10s` in 10s steps
- On-air / NodeTable / BLE: no change
- `OledStatusData.max_silence_10s` and all callers: unchanged

## Verification

- `pio run -e devkit_e220_oled` — build passes
