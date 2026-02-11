# u-blox UART Driver Plan v0

Purpose: implement a minimal UBLOX GNSS provider for OOTB firmware without changing BLE/mobile/radio protocols.

## Scope

- Message: UBX-NAV-PVT only (`class=0x01`, `id=0x07`)
- Transport: UART on ESP32
  - GPS_TX (module) -> GPIO15 (ESP32 TX to module RX)
  - GPS_RX (module) -> GPIO16 (ESP32 RX from module TX)
  - Baud: 9600 (common NEO-M8N default)
- Provider contract: `GnssSnapshot`
  - `fix_state`
  - `pos_valid`
  - `lat_e7`, `lon_e7`
  - `last_fix_ms`

## UBX framing

Read byte stream and parse bounded frames:

- Sync bytes: `0xB5 0x62`
- Header: `class`, `id`, `length_le`
- Payload: `length` bytes
- Checksum: `CK_A`, `CK_B` over `class..payload`

Invalid checksum or oversized payload is ignored.

## NAV-PVT mapping

For payload length 92 bytes:

- `fixType` at offset 20
- `lon` at offset 24 (int32, 1e-7 degrees)
- `lat` at offset 28 (int32, 1e-7 degrees)

Mapping policy:

- `fixType == 2` -> `FIX_2D`
- `fixType >= 3` -> `FIX_3D`
- otherwise -> `NO_FIX`
- `pos_valid = (fix_state != NO_FIX)`

`last_fix_ms` policy:

- update on each valid NAV-PVT sample
- keep previous value during no-fix

This keeps `pos_age_s` consistent with existing app runtime calculation.

## Configuration policy

Read-only startup policy for v0:

- Do not send UBX-CFG messages in this task.
- Accept module output as-is and parse NAV-PVT when available.

If field logs show no NAV-PVT output, configuration work is a follow-up and out of scope here.

## Validation

- Build:
  - `pio run -e devkit_e220_oled`
  - `pio run -e devkit_e220_oled_gnss`
- Unit:
  - parser framing/checksum test (native)
  - `pio test -e test_native -f test_hal_mocks`
- Manual (hardware):
  - boot log shows `gnss_provider: UBLOX`
  - outdoors: observe transition to valid fix and non-zero lat/lon
