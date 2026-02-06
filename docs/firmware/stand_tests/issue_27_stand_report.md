# Issue #27 Stand Test Report (Template)

**Issue:** #27 — [OOTB v0] 2.9 Интеграция и тест firmware на стенде  
**Date:** TBD  
**Tester:** TBD  
**Firmware build:** TBD (commit / branch / binary name)  
**Hardware env:** TBD (esp32dev / devkit_e220_oled / devkit_e220_oled_gnss)

## 1) Hardware / setup
- 2× dongles (ESP32 + E220), USB power + serial
- Antennas attached, same band/channel
- Optional: one dongle can be replaced by a stub if available

## 2) Flash steps / commands
- `pio run -e esp32dev` (or matching env)
- `pio run -e esp32dev -t upload --upload-port <PORT>`
- `pio device monitor -b 115200 -p <PORT>`

## 3) What to observe (expected signals)
- **UART logs (logging v0):**
  - Beacon TX / RX entries (timestamp + seq)
  - GEO_BEACON decode ok / error
  - NodeTable updates (node_id, last_seen, RSSI)
- **Radio activity:**
  - Both devices periodically transmit GEO_BEACON
  - Each device receives peer beacons (RX count increases)
- **BLE expectations:**
  - BLE advertising visible on each dongle
  - GATT read-only characteristics (DeviceInfo + NodeTable pages) **only if runtime BLE bridge wiring is enabled**

## 4) Results (fill after bench run)
- **Build flash result:** TBD
- **UART logs:** TBD
- **Radio exchange:** TBD
- **NodeTable updates:** TBD
- **BLE visibility:** TBD

## 5) Notes / follow-ups
- TBD
