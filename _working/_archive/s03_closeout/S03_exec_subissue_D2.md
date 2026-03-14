## Goal

Align **radio profile baseline** (FACTORY/current pointers) with implementation: default profile applied on first boot; current profile read/write and readback verify per canon; no user-defined profiles in S03. Canon: [radio_profiles_policy_v0](docs/product/areas/radio/policy/radio_profiles_policy_v0.md), [module_boot_config_v0](docs/product/areas/firmware/policy/module_boot_config_v0.md).

## Definition of done

- FACTORY/default profile is the OOTB default; current profile pointer used for TX/runtime.
- Readback verify after applying profile (air rate, channel, etc.) per module_boot_config_v0.
- Persistence of current profile (NVS or equivalent) so reboot keeps behavior.

## Touched paths

- Radio adapter / E220 driver (profile apply + readback); NVS or config storage for current profile; boot Phase A/B.
- Canon: radio_profiles_policy_v0, registry_radio_profiles_v0, module_boot_config_v0.

## Tests

- Unit: default profile applied when no stored profile; readback matches requested params (or documented deviation).
- Smoke: after reboot, radio still uses expected profile (manual or bench).

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: D — Provisioning / boot pipeline conformance (P1).
