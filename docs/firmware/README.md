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
