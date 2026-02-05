# Firmware Testing

This repo uses two distinct build worlds:
- Embedded firmware builds (Arduino/ESP32)
- Native unit tests (host build)

Canonical commands:
```
cd firmware
pio run -e esp32dev
pio run -e devkit_e220_oled
pio run -e devkit_e220_oled_gnss
pio test -e test_native
```

CI rule:
- Never run `pio run` without an explicit `-e` list.
- Native tests must run via `pio test -e test_native`.

Architecture rule:
- `test_native` does NOT compile `firmware/src`.
- Unit tests target `firmware/lib/NavigaCore` plus test sources.
