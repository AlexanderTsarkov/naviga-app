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
- **Power:** shared 3V3, common GND

**HW notes:**
- E220 decoupling: 1000uF + 0.1uF VCC-GND (local)
- M0 and M1 each have 10k pulldown to GND

## Planned profile stub: devkit_e220_oled_gnss
- **GPS module:** NEO-M8N (UART)
  - GPS_TX -> GPIO15
  - GPS_RX -> GPIO16
- **Decoupling:** 0.1uF local (VCC-GND)

## Wiring reference
См. Fritzing: `docs/hardware/naviga-breadboard-v0.3.fzz`.
