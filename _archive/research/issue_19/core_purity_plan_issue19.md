<!--
Issue #19 Step 1: core purity plan and leak map
-->
# Core purity plan (Issue #19)

This document captures the Step 1 audit results and a staged plan to make
`NavigaCore` platform-pure without changing runtime behavior.

## Leak map (current dependencies)

| Module | Dependency | Reason | Target layer |
|---|---|---|---|
| `firmware/src/app/app_services.cpp` | `#include <Arduino.h>` | Pin I/O + delay + Serial logging | Platform adapter / app layer |
| `firmware/src/app/app_services.cpp` | `Serial.*` | Debug/log output | Platform logging |
| `firmware/src/app/app_services.cpp` | `delay(30)` | Role pin settle timing | Platform clock (or app bootstrap) |
| `firmware/src/services/ble_service.cpp` | `#include <Arduino.h>` | BLE callbacks + Serial logging | Platform adapter / service layer |
| `firmware/src/services/ble_service.cpp` | `Serial.*` | BLE status logging | Platform logging |

**Notes**
- No occurrences of `Arduino.h`, `Serial`, `millis`, `delay`, `esp_*`,
  `ESP_OK`, `esp_efuse` were found inside `firmware/lib/NavigaCore/**`.

## Definition of "core"

**Core = `NavigaCore` library** (plus selected services later) that:
- Contains pure domain logic, data models, and interfaces.
- Has **no** Arduino/ESP32 includes, types, or global state.
- Talks to the outside world only via platform abstractions
  (`clock`, `log`, `device_id`, `radio`, `ble`, `gnss`, etc.).

Platform adapters and hardware integrations stay in `firmware/src/platform/**`.

## Step plan

**Step 1 (this PR):**  
Audit dependencies + add platform abstraction headers (no behavior changes).

**Step 2:**  
Introduce concrete adapters in `firmware/src/platform/**` that implement
`IClock`, `ILogger`, `IDeviceIdProvider`. Plumb through app/services as
needed, but keep behavior unchanged.

**Step 3:**  
Move truly core services into `NavigaCore` (or refactor to be platform-agnostic),
leaving Arduino/ESP-only code in platform adapters or app glue.
