# #380 — HW Provisioning Inventory Report (S03)

**Scope:** GNSS (u-blox) + E220 UART modem. Audit-only; no code or doc edits.  
**Purpose:** Factual inventory for S03 policy (#381) and provisioning baseline (#353).

---

## 1) HW Provisioning Inventory

| Module | Param | Why product needs it | Current behavior | Code location (file:function) | Persisted? | Verified/healed on boot? | Gaps/TBD |
|--------|--------|------------------------|------------------|-------------------------------|------------|---------------------------|----------|
| **GNSS** | UART baud | Host–receiver link; must match receiver. | Fixed 9600; UART opened at 9600 every boot. | `gnss_ublox_service.h` (kUartBaud=9600); `gnss_ublox_service.cpp::init()` → `io_->begin(kUartBaud, ...)`; `gnss_ubx_uart_io.cpp::begin()` | No (not in NVS). | No readback. Verify = “at least one byte received” within timeout (link alive). | Baud not read from receiver; if receiver was at different baud, verify fails. No explicit “repair baud” path. |
| **GNSS** | Protocol / message set | Beacon needs position; parser expects UBX NAV-PVT. | UBX only. Init sends CFG-MSG to enable NAV-PVT on UART1 (rate 1). No NMEA config. | `gnss_ublox_service.cpp::init()` → `send_cfg_enable_nav_pvt()`; `send_cfg_enable_nav_pvt()` (UBX-CFG-MSG payload) | No. Re-sent every boot in init. | No. Verify is “bytes received”; if receiver in NMEA we get no valid UBX frame (diag emits “hint=NMEA”). No automatic protocol “repair”. | Protocol/message set not read back from receiver; repair = re-send CFG-MSG only (already done in init). |
| **GNSS** | Pins (RX/TX) | Physical link. | From HW profile; passed to UART begin. | `gnss_ublox_service.cpp::init()` → `get_hw_profile()`, `io_->begin(..., profile.pins.gps_rx, profile.pins.gps_tx)` | No (compile-time profile). | N/A. | None. |
| **GNSS** | Link responsiveness | Ensure GNSS UART is alive for position. | Within timeout_ms, tick until at least one byte received. If fail, init() called again and verify retried once. | `gnss_ublox_service.cpp::verify_on_boot()`; `app_services.cpp::init()` (first_verify / second_verify, log “ok” / “repaired” / “failed”) | N/A. | Yes: verify_on_boot(500 ms); retry once on failure. | Policy (module_boot_config_v0) says “minimal verify-on-boot; repair on mismatch”; current “repair” is retry init + verify, not baud/protocol reconfig. |
| **E220** | UART baud | Host–modem link. | Fixed 9600 (UART_BPS_9600). Library and serial use 9600. | `e220_radio.cpp` (kE220BaudRate=UART_BPS_RATE_9600, kExpectedUartBaudRate); `critical_params_match` / `apply_critical` use kExpectedUartBaudRate | On-module (WRITE_CFG_PWR_DWN_SAVE). Not in ESP NVS. | Yes: getConfiguration(); if mismatch apply_critical + setConfiguration + re-read + verify_preset_readback. | None. |
| **E220** | UART parity | Frame format. | MODE_00_8N1. | `e220_radio.cpp` (kExpectedUartParity); `critical_params_match` / `apply_critical` | On-module. | Yes (verify-and-repair every boot). | None. |
| **E220** | RSSI enable | Link metrics; NodeTable last_rx_rssi; activity. | Always ON (RSSI_ENABLED). Applied and verified every boot. | `e220_radio.cpp` (kExpectedRssiEnable, TRANSMISSION_MODE.enableRSSI); `apply_critical`; recv uses receiveMessageRSSI() when rssi_enabled_ | On-module. | Yes. | None. |
| **E220** | RSSI ambient noise | RSSI byte validity. | RSSI_AMBIENT_NOISE_ENABLED. | `e220_radio.cpp` (kExpectedRssiAmbientNoise, OPTION.RSSIAmbientNoise); `apply_critical` | On-module. | Yes. | None. |
| **E220** | LBT enable | Policy: sense unsupported; jitter-only. | OFF (0). | `e220_radio.cpp` (kExpectedLbtEnable, TRANSMISSION_MODE.enableLBT); `critical_params_match` / `apply_critical` | On-module. | Yes. | None. |
| **E220** | Air data rate | Same air speed for all nodes; preset. | From RadioPreset (Default=2 = 2.4 kbps). Normalized (clamp &lt; 2 to 2). | `radio_factory.cpp::create_radio()` → get_radio_preset(RadioPresetId::Default); `e220_radio.cpp::begin(preset)` (normalize_air_rate); ApplyTarget.air_data_rate; `apply_critical` (SPED.airDataRate) | On-module. | Yes: readback airDataRate vs normalized preset; repair + verify_preset_readback. | Preset source is compile-time Default only; no NVS-derived preset yet. |
| **E220** | Channel | Same channel for all nodes; preset. | From RadioPreset (Default channel=1). | `e220_radio.cpp::begin(preset)` (ApplyTarget.channel); `apply_critical` (CHAN); verify_preset_readback(readback_channel) | On-module. | Yes. | Same: preset from Default only. |
| **E220** | Sub-packet setting | Payload/framing contract. | SPS_200_00. | `e220_radio.cpp` (kExpectedSubPacketSetting, OPTION.subPacketSetting); `apply_critical` | On-module. | Yes. | None. |
| **E220** | Address (ADDH/ADDL) | Transparent; not used for addressing. | 0x00, 0x00. | `e220_radio.cpp` (kExpectedAddressHigh/Low, ADDH/ADDL); `apply_critical` | On-module. | Yes. | None. |
| **E220** | Fixed transmission | Transparent mode. | FT_TRANSPARENT_TRANSMISSION. | `e220_radio.cpp` (kExpectedFixedTx, TRANSMISSION_MODE.fixedTransmission); `apply_critical` | On-module. | Yes. | None. |
| **E220** | TX power | OOTB power level; future telemetry. | **Not applied via UART.** RadioPreset has tx_power_dbm (advisory); E220 begin() does not write power to module. Module keeps factory or previous value. | `radio_preset.h` (tx_power_dbm “advisory; not part of UART config frame”); `e220_radio.cpp::begin()` — no power field in apply_critical / Configuration | On-module (whatever module had). Not set by our code. | No. We do not read or write TX power in E220 config. | **Gap:** TX power not in verify-and-repair; not read back. Policy/telemetry (S03) may need explicit “readback not available” or module-default only. |
| **E220** | Config mode settling | Reliable getConfiguration after mode switch. | M0/M1 HIGH, delay 200 ms, flush serial, then getConfiguration. | `e220_radio.cpp::begin()` (#326 fix: digitalWrite M0/M1 HIGH, delay(200), flush, drain) | N/A. | N/A (part of boot sequence). | None. |
| **E220** | Preset source | Which channel/air-rate to apply. | Compile-time Default only. create_radio() uses get_radio_preset(RadioPresetId::Default). | `radio_factory.cpp::create_radio()`; `radio_preset.h::get_radio_preset()` | No. Phase B persists “current radio profile id” in NVS but effective_radio_id is clamped to 0; preset not loaded from NVS. | N/A. | **Gap:** NVS has prof_cur but code only uses profile 0; no RadioProfileRecord in NVS; preset is always Default. |

---

## 2) Gaps / decisions needed (for S03 policy #381)

- **GNSS baud:** No readback from receiver; verify is “link alive” only. Decide whether to document “assume 9600, no baud repair” or add explicit baud-detection/repair in policy.
- **GNSS protocol:** We send CFG-MSG each boot but do not read back. If receiver is in NMEA, we only detect via “no UBX” / diag hint. Decide if “repair” should be defined as “re-send UBX config” only (current behavior) or require protocol readback when available.
- **Boot phase placement:** Hardware provisioning (GNSS init+verify, E220 begin) is in Phase A (app_services.cpp). Role/radio pointers are Phase B. Document in #381 that “hardware provisioning” = Phase A; “role/radio profile application” = Phase B; no overlap.
- **Verify strategy for GNSS:** module_boot_config_v0 says “minimal verify-on-boot; repair on mismatch.” Current “repair” = retry init + verify_once. Decide whether to call this “repair” in policy or “retry”; and whether baud/protocol repair (if any) is first-boot-only vs every boot.
- **E220 TX power:** Not in critical params; not read/written. Decide: (1) document as “module default only; no readback in S03” for telemetry contract, or (2) add TX power to module config path when E220 library/product supports it.
- **E220 preset source:** Today preset is always Default (radio_factory). NVS stores prof_cur but code clamps to 0. Decide for S03: either (1) keep “OOTB default only” and document that prof_cur is reserved, or (2) wire prof_cur to preset selection and add RadioPreset/NVS schema (per #382/#383).
- **RSSI append:** E220: RSSI enabled in config; receiveMessageRSSI() used; last_rssi_dbm() exposed. No “append” to payload by us — library/receiver append. Document “RSSI byte enabled; read on RX; no SNR (E220 unsupported).”
- **Drift detection E220:** Implemented: getConfiguration() every boot; critical_params_match; on mismatch apply_critical + setConfiguration + readback verify. Document in #381 as “verify-and-repair every boot” per module_boot_config_v0.
- **Fallback behavior GNSS:** If verify_on_boot fails twice, we log “GNSS boot: failed” but do not prevent Phase B/C. Decide whether “GNSS failed” should block comms start or only log (policy).
- **Fallback behavior E220:** If begin() returns RepairFailed, radio_ready is still set from begin() return (true); ready_ may be true. Comms can start. Document intended fallback: allow comms with “repair failed” or require “no comms until Ok or Repaired.”
- **Single preset in code:** Only Default preset is used; Fast/LongRange exist in enum but create_radio() does not take profile id from NVS. Align with “OOTB/default only” in S03 and document in baseline.
- **Config persistence location:** E220 config lives on the module (WRITE_CFG_PWR_DWN_SAVE). GNSS has no NVS; we re-send CFG-MSG each boot. Document “no NVS for radio or GNSS module config; only role/radio pointers and role record in NVS.”
- **Heal contract wording:** module_boot_config_v0 §2 and §2a describe E220 verify-and-repair. #381 should reference this and state explicitly: “Phase A: hardware provisioning per module_boot_config_v0; E220 critical params verified and repaired every boot; GNSS minimal verify (link), config re-sent each boot.”
- **Message set GNSS:** Only NAV-PVT enabled (UBX). No rate/config for other messages. Document as “required output: NAV-PVT on UART1 rate 1; no other message contract in code.”
- **Readback verification E220:** airDataRate and channel are read back and compared (verify_preset_readback). TX power is not. Document “readback verify: air_rate + channel only; TX power not read back.”

---

**References**

- `docs/product/areas/firmware/policy/module_boot_config_v0.md` — critical params, verify-and-repair, GNSS strategy.
- `docs/product/areas/firmware/policy/boot_pipeline_v0.md` — Phase A/B/C.
- `firmware/src/app/app_services.cpp` — Phase A (GNSS, radio begin), Phase B (role/radio pointers).
- `firmware/src/services/gnss_ublox_service.cpp` — init, verify_on_boot, send_cfg_enable_nav_pvt.
- `firmware/src/platform/gnss_ubx_uart_io.cpp` — UART begin.
- `firmware/src/platform/e220_radio.cpp` — begin, critical_params_match, apply_critical, getConfiguration/setConfiguration, RSSI.
- `firmware/lib/NavigaCore/include/naviga/hal/radio_preset.h` — RadioPreset, get_radio_preset(Default).
- `firmware/src/platform/radio_factory.cpp` — create_radio, preset from Default.
