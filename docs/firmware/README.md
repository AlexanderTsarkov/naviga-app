# Документация: прошивка

Документы по прошивке Naviga.

## Build + flash (PlatformIO)
PlatformIO проект находится в `firmware/`.

### Build
```bash
cd firmware
pio run -e devkit_e220_oled
```

### Flash (USB)
```bash
cd firmware
pio run -e devkit_e220_oled -t upload
```

### Serial monitor
```bash
cd firmware
pio device monitor -b 115200
```

## BLE (read-only) test with nRF Connect
1) Flash and boot the device.
2) Open **nRF Connect**, scan for **Naviga**.
3) Connect and read the **Naviga** service characteristics:

**Service UUID**
- `6e4f0001-1b9a-4c3a-9a3b-000000000001`

**DeviceInfo (read)**
- UUID: `6e4f0002-1b9a-4c3a-9a3b-000000000001`
- Payload (little-endian):
  - `fw_version` (u8 len + bytes)
  - `hw_profile` (u8 len + bytes)
  - `band_id` (u16 LE, 433)
  - `public_channel_id` (u8, 1)
  - `full_id_u64` (u64 LE)
  - `short_id` (u16 LE)

**NodeTablePage0..3 (read)**
- Page size fixed to 10 entries.
- UUIDs:
  - Page0: `6e4f0003-1b9a-4c3a-9a3b-000000000001`
  - Page1: `6e4f0004-1b9a-4c3a-9a3b-000000000001`
  - Page2: `6e4f0005-1b9a-4c3a-9a3b-000000000001`
  - Page3: `6e4f0006-1b9a-4c3a-9a3b-000000000001`

**NodeEntry layout (fixed-size, little-endian)**
- `full_id` (u64 LE)
- `short_id` (u16 LE)
- `lat_e7` (i32 LE)
- `lon_e7` (i32 LE)
- `last_seen_ms` (u32 LE)
- `flags` (u8)
