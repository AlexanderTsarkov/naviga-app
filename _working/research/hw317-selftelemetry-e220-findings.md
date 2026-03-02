# HW-317 Research: SelfTelemetry wiring gap + E220 REPAIR_FAILED

**Iteration:** S02__2026-03__docs_promotion_and_arch_audit (Evidence Fix Exception)
**Issues:** #317 (bench evidence), #323 (SelfTelemetry wiring)
**Date:** 2026-03-01
**Status:** Research complete; no code changes in this doc.

---

## A. Why 0x04/0x05 are never emitted

### A1. Source map

| Field | Packet | Source in codebase | Where computed | Where it must be wired | Current status |
|---|---|---|---|---|---|
| `uptimeSec` | 0x04 Operational | `naviga::uptime_ms()` → `millis()` | `firmware/src/platform/timebase.cpp` | `AppServices::tick()` → `M1Runtime` → `self_telemetry_` | **MISSING** — `now_ms` is passed to OLED (`oled_data.uptime_ms = now_ms`) but never set on `self_telemetry_` |
| `batteryPercent` | 0x04 Operational | No real source on devkit | N/A (USB-powered) | `self_telemetry_.battery_percent` stub | **MISSING** — no stub, `has_battery = false` |
| `txPower` | 0x04 (planned, S03) | E220 `Configuration` struct (not read back) | `e220_radio.cpp::begin()` reads config once but discards it | N/A — field not in payload yet | **NOT IN PAYLOAD** — planned only; see §C |
| `maxSilence10s` | 0x05 Informative | `effective_max_silence_10s` in `AppServices::init()` | `naviga_storage.cpp::load_current_role_profile_record()` → `role_profile_ootb.cpp::get_ootb_role_profile()` | `self_telemetry_.max_silence_10s` | **MISSING** — value is computed and used for `max_silence_ms` but never stored on `self_telemetry_` |
| `hwProfileId` | 0x05 Informative | `get_hw_profile().name` (string, not uint16) | `firmware/src/hw_profiles/get_hw_profile.cpp` | `self_telemetry_.hw_profile_id` stub | **MISSING** — no uint16 mapping exists; string name only |
| `fwVersionId` | 0x05 Informative | `kFirmwareVersion = "ootb-74-m1-runtime"` (string) | `firmware/src/app/app_services.cpp:29` | `self_telemetry_.fw_version_id` stub | **MISSING** — no uint16 hash/constant exists |

### A2. Dataflow / call graph

```
AppServices::init()
  └─ load_current_role_profile_record(&profile_record, &valid)   // naviga_storage.cpp
       └─ Preferences NVS read → profile_record.max_silence_10s  // real value, e.g. 9
  └─ get_ootb_role_profile(role_id, &profile_record)             // fallback
  └─ effective_max_silence_10s = profile_record.max_silence_10s  // local variable only
  └─ max_silence_ms = effective_max_silence_10s * 10 * 1000
  └─ runtime_.init(..., max_silence_ms, ...)                     // passed to BeaconLogic
  └─ self_telemetry_ = {}                                        // DEFAULT-INIT: all has_* = false ← GAP

AppServices::tick(now_ms)
  └─ runtime_.tick(now_ms)
       └─ M1Runtime::handle_tx(now_ms)
            └─ beacon_logic_.update_tx_queue(now_ms, self_fields_, self_telemetry_, allow_core_send_)
                 // beacon_logic.cpp:187 — gate: telemetry.has_battery || telemetry.has_uptime
                 // → NEVER true → kSlotTail2 never enqueued
                 // beacon_logic.cpp:205 — gate: has_max_silence || has_hw_profile || has_fw_version
                 // → NEVER true → kSlotInfo never enqueued
  └─ oled_data.uptime_ms = now_ms                               // uptime IS available here
  └─ oled_.update(now_ms, ...)                                  // OLED uses uptime; runtime does not
```

**Root cause (single sentence):** `self_telemetry_` is declared in `M1Runtime` (line 90 of `m1_runtime.h`) and default-initialised with all `has_* = false`; nothing in `AppServices::init()` or `tick()` ever writes to it.

### A3. Fix surface — minimal wiring required

All wiring belongs in `AppServices` (the composition layer), not inside `M1Runtime` or `BeaconLogic`.

**Option 1 — Wire in `init()` for static fields + `tick()` for dynamic fields (preferred):**

In `AppServices::init()` (after Phase B, before `runtime_.init()`):
```cpp
// Populate static telemetry fields known at boot.
// effective_max_silence_10s already computed above.
self_telemetry_.has_max_silence = true;
self_telemetry_.max_silence_10s = effective_max_silence_10s;
// hwProfileId: stub until uint16 mapping is defined (see issue TBD).
self_telemetry_.has_hw_profile  = false;  // TODO: wire when hwProfileId formalized
// fwVersionId: stub until hash/constant is defined (see issue TBD).
self_telemetry_.has_fw_version  = false;  // TODO: wire when fwVersionId formalized
// batteryPercent: USB devkit stub (no real ADC).
self_telemetry_.has_battery     = true;
self_telemetry_.battery_percent = 100;    // TODO: replace with real battery service
```

In `AppServices::tick()` (before `runtime_.tick()`):
```cpp
// Update dynamic telemetry each tick.
self_telemetry_.has_uptime  = true;
self_telemetry_.uptime_sec  = now_ms / 1000U;
```

Then pass `self_telemetry_` into `runtime_` — either via a new setter method or by exposing it as a parameter. The cleanest approach: add `M1Runtime::set_self_telemetry(const domain::SelfTelemetry&)` (one-liner setter), called from `tick()` before `runtime_.tick()`.

**What does NOT need to change:**
- `beacon_logic_.update_tx_queue()` — formation gates already correct.
- `encode_tail2_frame()` / `encode_info_frame()` — codecs already correct.
- Protocol layout — no changes.
- Unit tests for `update_tx_queue` — existing tests already cover the enqueue path with `has_*=true`; a new test for the runtime wiring path would be an integration-level test.

### A4. Stub decisions (for implementation step)

| Field | Decision | Rationale |
|---|---|---|
| `batteryPercent` | Stub: `has_battery=true`, value=100 | USB devkit; proves transport works; explicit TODO comment |
| `uptimeSec` | Real: `now_ms / 1000U` | Already in scope at `tick()`; zero cost |
| `maxSilence10s` | Real: from `profile_record.max_silence_10s` | Already computed in `init()`; just needs to be stored |
| `hwProfileId` | Stub: `has_hw_profile=false` | No uint16 mapping exists; needs separate issue |
| `fwVersionId` | Stub: `has_fw_version=false` | No uint16 hash/constant exists; needs separate issue |

With these stubs: `has_battery=true` triggers 0x04 enqueue; `has_max_silence=true` triggers 0x05 enqueue. Both packets will appear on wire.

---

## B. E220 REPAIR_FAILED "read failed"

### B1. Exact failure point

File: `firmware/src/platform/e220_radio.cpp`, function `E220Radio::begin()`

```
begin()
  └─ radio_.begin()                          // LoRa_E220 library init; sets up UART + M0/M1/AUX pins
  └─ radio_.getConfiguration()               // Sends 0xC1 0xC1 0xC1 command in config mode
       └─ ResponseStructContainer.status.code != E220_SUCCESS  ← FAILURE POINT (line 83)
            OR config.data == nullptr
  └─ last_boot_result_ = RepairFailed
  └─ last_boot_message_ = "read failed"      // line 86
  └─ return true (radio still "up" for TX)
```

The `getConfiguration()` call requires the E220 to be in **config mode** (M0=0, M1=1). The LoRa_E220 library switches modes internally before the call, but success depends on AUX going low then high within a timeout, and the UART responding with the config register bytes.

### B2. Hypotheses (ranked by likelihood on devkit hardware)

1. **AUX pin not responding / wrong GPIO** — AUX is the E220's "ready" signal. If the AUX pin mapping is wrong (e.g. `lora_aux=14` in `devkit_e220_oled_gnss.cpp` is not the physical AUX wire), the library times out waiting for AUX to go high after mode switch. Result: `getConfiguration()` returns error. **Most likely on a new/unverified devkit wiring.**

2. **M0/M1 pins not switching the module** — M0=10, M1=11 in the profile. If these GPIOs are not connected to the E220's M0/M1 pins (or are inverted), the module never enters config mode. UART command 0xC1 0xC1 0xC1 is sent in normal mode → no response → timeout.

3. **UART conflict / wrong port** — `HardwareSerial serial_{2}` uses UART2 on ESP32-S3. If `lora_rx=12` / `lora_tx=13` are swapped relative to the E220's TX/RX, or if another peripheral shares UART2, the config read returns garbage or nothing.

4. **Power brownout / E220 not ready at boot** — E220 needs ~100 ms after power-on before it can enter config mode. If `begin()` is called immediately after power-on without a delay, AUX may not be stable. The library has an internal wait, but its timeout may be too short on some modules.

5. **CH340 / USB serial conflict** — On some devkits, UART0 (USB serial / log output) and UART2 share the same physical pins or the CH340 holds the bus. Less likely if TX/RX are on dedicated GPIOs (12/13), but worth checking if the UART2 pins are also routed to the USB bridge.

6. **Baud rate mismatch at first boot** — If the E220 was previously configured to a non-9600 baud rate (e.g. by another firmware), the library initialises at 9600 but the module responds at a different rate → garbage → `getConfiguration()` fails. This would be a one-time issue fixed by a factory reset.

### B3. What to verify on hardware (bench checklist)

- [ ] Probe AUX pin (GPIO 14) with oscilloscope or logic analyser during `begin()`: should go LOW then HIGH within ~500 ms of M1 going HIGH.
- [ ] Confirm M0=GPIO10, M1=GPIO11 are physically connected to E220 M0/M1 (not swapped, not floating).
- [ ] Confirm UART2 RX=GPIO12 ↔ E220 TX, UART2 TX=GPIO13 ↔ E220 RX (not swapped).
- [ ] Add a 200 ms delay before `radio_.begin()` call and retest — rules out power-on timing.
- [ ] Check if `radio_boot=REPAIR_FAILED "read failed"` appears on both nodes or only one — if only one, it's likely a wiring/hardware issue on that specific devkit.
- [ ] After REPAIR_FAILED, check if TX still works (it does per current code — `return true` even on failure): if `pkt tx` lines appear in logs, the radio is transmitting despite config uncertainty.

### B4. Impact on 0x04/0x05 bench

REPAIR_FAILED does **not** block TX. `E220Radio::begin()` returns `true` even when config read fails; `radio_ready_` is set to `true`; `M1Runtime::tick()` proceeds normally. The RSSI feature is assumed enabled (`rssi_enabled_ = (kExpectedRssiEnable == RSSI_ENABLED)` — set to true as a fallback). So once SelfTelemetry is wired, 0x04 and 0x05 **will** be enqueued and transmitted even with REPAIR_FAILED. The REPAIR_FAILED issue is separate and does not block the #323 fix.

---

## C. txPower / Radio profile persistence — current status

### C1. What exists today

- **E220 `Configuration` struct** is read once in `E220Radio::begin()` via `radio_.getConfiguration()`. If successful, `critical_params_match()` checks: baud, parity, RSSI enable, LBT enable, sub-packet setting, RSSI ambient noise. **txPower is NOT in the critical params list** — it is not read, not checked, not stored.
- **`PersistedPointers`** stores `current_radio_profile_id` (uint32) in NVS key `"prof_cur"`. Currently only profile ID 0 is valid (`effective_radio_id != 0` → clamped to 0). The profile ID is a pointer only — there is no `RadioProfileRecord` struct analogous to `RoleProfileRecord`.
- **No `RadioProfileRecord` in NVS** — `naviga_storage.h` has `RoleProfileRecord` (cadence params) with full load/save/validate, but no equivalent for radio params (channel, txPower, air data rate, etc.).
- **Channel** is hardcoded: `const uint8_t effective_channel = (effective_radio_id == 0) ? 1 : 1;` (always 1, regardless of profile).
- **txPower field in payload**: `tail2_codec.h` line 24 says `txPower: planned field (S03, not yet implemented)`. It is not in `Tail2Fields`, not in `SelfTelemetry`, not in any codec.

### C2. Missing pieces for a full txPower/profile cycle

1. **Read txPower from E220 config** — `getConfiguration()` returns a `Configuration` struct that includes `OPTION.transmissionPower` (LoRa_E220 library). Currently discarded after `critical_params_match()`. Need to store it.
2. **`RadioProfileRecord` struct** — define params: channel, txPower (dBm or enum), air data rate. Add NVS load/save (analogous to `RoleProfileRecord`).
3. **Apply-on-boot** — after loading profile, call `setConfiguration()` to apply channel + txPower. Currently only critical params (baud/parity/RSSI/LBT) are repaired; channel and txPower are not touched.
4. **Verify after apply** — re-read and confirm txPower was accepted.
5. **Expose txPower to `SelfTelemetry`** — once reliably read, wire into `self_telemetry_.tx_power_dbm` (field not yet defined in `SelfTelemetry` or `Tail2Fields`).
6. **Payload addition** — add `txPower` field to `Tail2Fields` + codec + NodeTable (protocol change, requires canon update).

**None of steps 1–6 should be done in #323.** This is a separate full-cycle work item.

### C3. Recommended follow-up issues

- **Issue A: "Radio profile persistence: RadioProfileRecord + apply-on-boot"** — define `RadioProfileRecord` (channel + txPower), NVS load/save, apply on boot, verify. No payload change.
- **Issue B: "hwProfileId / fwVersionId formalization"** — define uint16 mapping for HW profile names; define stable fwVersionId (build hash or semver encoding). Wire into `SelfTelemetry` and remove stubs.

---

## D. Design note — next step plan

### Step 1 (minimal, in `app_services.cpp` + `m1_runtime.h/.cpp`)
- Add `M1Runtime::set_self_telemetry(const domain::SelfTelemetry& t)` setter (1 line in `.h`, 1 line in `.cpp`).
- Store `effective_max_silence_10s` as a member of `AppServices` (currently local to `init()`).
- In `AppServices::init()`: populate static fields of `self_telemetry_` (maxSilence, battery stub).
- In `AppServices::tick()`: update `self_telemetry_.uptime_sec = now_ms / 1000U; has_uptime = true;` then call `runtime_.set_self_telemetry(self_telemetry_)` before `runtime_.tick()`.

### Step 2 (unit test)
- Add test in `test_beacon_logic` (or a new `test_m1_runtime` if integration-level): verify that `update_tx_queue` enqueues `kSlotTail2` when `has_battery=true` and `kSlotInfo` when `has_max_silence=true`. (These tests already exist in `test_beacon_logic.cpp` — confirm they cover the exact formation path; add a "no telemetry → not enqueued" negative test if missing.)

### Step 3 (bench rerun)
- Flash both devkits with the fix.
- Enable instrumentation (`instr on` in provisioning shell).
- Observe `pkt tx type=TAIL2` and `pkt tx type=INFO` in serial logs within one maxSilence window.
- Capture log lines with `msg_type`, `seq`, and timestamp; attach to #317 comment.

### Step 4 (follow-up issues)
- Create issue for Radio profile persistence / txPower (§C3 Issue A).
- Create issue for hwProfileId / fwVersionId formalization (§C3 Issue B).
- Both are out of scope for #323 PR.

---

## E. Files to touch in #323 PR

| File | Change |
|---|---|
| `firmware/src/app/app_services.h` | Add `domain::SelfTelemetry self_telemetry_{}` member; add `uint8_t effective_max_silence_10s_` member |
| `firmware/src/app/app_services.cpp` | Populate `self_telemetry_` in `init()` and `tick()` |
| `firmware/src/app/m1_runtime.h` | Add `set_self_telemetry()` setter declaration |
| `firmware/src/app/m1_runtime.cpp` | Implement `set_self_telemetry()` |
| `firmware/test/test_beacon_logic/test_beacon_logic.cpp` | Confirm/add negative test: empty telemetry → no 0x04/0x05 enqueued |

**Files NOT touched:** `beacon_logic.*`, `node_table.*`, all codecs, all protocol docs, `e220_radio.*`.
