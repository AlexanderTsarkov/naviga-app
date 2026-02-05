## Issue Link
Refs/Closes #...

## Summary
- 

## Test Plan
```
cd firmware
pio run -e esp32dev
pio run -e devkit_e220_oled
pio run -e devkit_e220_oled_gnss
pio test -e test_native
```

## Checklist
- [ ] CI uses explicit envs (no bare pio run)
- [ ] test_native uses pio test (not pio run)
