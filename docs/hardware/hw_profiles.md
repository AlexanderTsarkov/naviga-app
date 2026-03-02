# HW Profiles v0 (PlatformIO)

Цель: описать аппаратные профили (пины/варианты модулей) и убрать хардкод пинов в коде.

## Profile: devkit_e220_oled (current breadboard)
- **Board:** ESP32-S3 DevKit (WROOM-1 N16R8)
- **Radio module:** LoRa E220 (UART)
  - M0 = GPIO10
  - M1 = GPIO11
  - RXD = GPIO12
  - TXD = GPIO13
  - AUX = GPIO14
- **I2C OLED**
  - SCL = GPIO17
  - SDA = GPIO18
- **Role strap**
  - ROLE_PIN = GPIO8 (INPUT_PULLUP; LOW at boot => INIT, else RESP)
- **Power:** shared 3V3, common GND

**HW notes:**
- E220 decoupling: 1000uF + 0.1uF VCC-GND (local)
- M0 and M1 each have 10k pulldown to GND

## Profile: devkit_e220_oled_gnss
- **Board:** ESP32-S3 DevKit (WROOM-1 N16R8)
- **Radio module:** LoRa E220 (UART) — same GPIO as `devkit_e220_oled`
- **GPS module:** NEO-M8N (UART)
  - GPS_TX → GPIO15
  - GPS_RX → GPIO16
- **Build flag:** `HW_PROFILE_DEVKIT_E220_OLED_GNSS`
- **GNSS provider:** `GNSS_PROVIDER_UBLOX`

## Profile: devkit_e22_oled_gnss
- **Board:** ESP32-S3 DevKit (WROOM-1 N16R8)
- **Radio module:** LoRa E22-400T30D (UART) — pin-compatible with E220 on breadboard
  - M0 = GPIO4, M1 = GPIO5
  - RXD = GPIO1, TXD = GPIO2
  - AUX = GPIO47
- **I2C OLED:** SCL = GPIO17, SDA = GPIO18
- **Role strap:** ROLE_PIN = GPIO8
- **GPS module:** NEO-M8N (UART) — GPS_TX → GPIO16, GPS_RX → GPIO15
- **Build flag:** `HW_PROFILE_DEVKIT_E22_OLED_GNSS`
- **GNSS provider:** `GNSS_PROVIDER_UBLOX`
- **Radio library:** `xreef/EByte LoRa E22 library` (separate from E220; cannot be included in same TU)
- **Frequency / power variant build flags:** `-DFREQUENCY_433 -DE22_30`

**HW notes (E22):**
- E22 config mode = M0=LOW, M1=HIGH (differs from E220 config mode M0=HIGH, M1=HIGH).
- E22-400T30D V2 firmware clamps airRate < 2 to 2 (2.4 kbps) — see `docs/hardware/radio_modules_naviga.md` §D.

## Wiring reference
См. Fritzing: `docs/hardware/naviga-breadboard-v0.3.fzz`.
