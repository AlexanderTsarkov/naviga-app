## Goal

Ensure boot phases A/B/C reflect the provisioning baseline and that **OOTB autonomous start** works without phone: new device can be flashed, powered on, and start sending/receiving per canon. Align with [provisioning_baseline_v0](docs/product/areas/firmware/policy/provisioning_baseline_v0.md), [ootb_autonomous_start_v0](docs/product/areas/firmware/policy/ootb_autonomous_start_v0.md), [boot_pipeline_v0](docs/product/areas/firmware/policy/boot_pipeline_v0.md).

## Definition of done

- Phase A: module boot / verify-and-repair (E220, GNSS) per module_boot_config_v0.
- Phase B: provision role + radio profile from NVS; defaults applied when no stored config.
- Phase C: start Alive/Beacon cadence; first packets sent per OOTB rules.
- No phone/BLE required for autonomous operation after first boot.

## Touched paths

- Boot/init sequence (e.g. `firmware/src/main.cpp` or app init); provisioning load/save; M1Runtime or composition point that starts radio cadence.
- Canon: boot_pipeline_v0, module_boot_config_v0, provisioning_baseline_v0, ootb_autonomous_start_v0.

## Tests

- Smoke test / minimal HW bench checklist (manual): flash, power on, confirm Alive or Core sent within expected window; optional: second node receives.
- Unit or integration: Phase B defaults applied when NVS empty; Phase C starts when Phase B complete.

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: D — Provisioning / boot pipeline conformance (P1).
