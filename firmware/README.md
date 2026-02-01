# Naviga Firmware

Прошивка устройств Naviga (на базе Meshtastic).

Исходники и сборка — в отдельном репозитории / локальном клоне (см. docs/firmware и REFERENCE_REPOS.md). Здесь — место под submodule или ссылки при необходимости.

Бинарники сборки: см. `firmware-release/` в корне (артефакты, в .gitignore) или инструкции в docs/firmware.

## How to build (minimal skeleton)

From repo root:

```bash
cd firmware && pio run
```

Requires [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/). No hardware upload; build only.
