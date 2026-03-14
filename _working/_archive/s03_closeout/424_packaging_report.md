# #424 Packaging report

## Branch

- **Name:** `issue/424-radio-profile-baseline`
- **Base:** `origin/main` (single-issue branch)

## Commit(s)

- **54df1d2** — feat(#424): align radio profile baseline with FACTORY/current canon  
  - firmware/lib/NavigaCore/include/naviga/hal/radio_preset.h (Default 21 dBm)  
  - firmware/src/app/app_services.cpp (Phase B comments, kRadioProfileIdFactoryDefault)  
  - firmware/src/platform/naviga_storage.cpp (rprof_ver)  
  - firmware/src/platform/radio_factory.cpp (preset_from_factory_default)  
  - firmware/test/test_radio_preset/test_radio_preset.cpp (Default 21 dBm test)  
  - firmware/test/test_role_profile_registry/test_role_profile_registry.cpp (comment)  
  - _working/424_contract_and_gaps.md  

## PR

- **Link:** https://github.com/AlexanderTsarkov/naviga-app/pull/442  
- **Title:** feat(#424): align radio profile baseline with FACTORY/current canon  
- **Body:** Phase A/B changes, 21 dBm default, baseline/runtime separation, validation, ESP32-side factory-profile note, User Profile out of scope.

## Issue comment

- **Link:** https://github.com/AlexanderTsarkov/naviga-app/issues/424#issuecomment-4046446280  
- **Content:** PR link, summary of changes, validation status, note that User Profile baseline is separate.

## Exit criteria

- [x] Changes committed (single commit)
- [x] PR opened (#442)
- [x] Accurate PR body (no overclaim of native coverage; User Profile out of scope)
- [x] #424 issue comment prepared and posted
- [x] Short packaging report (this file)
