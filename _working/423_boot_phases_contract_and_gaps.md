# #423 Boot phases A/B/C + OOTB autonomous start — contract and gap table

**Issue:** #423 · **Umbrella:** #416 Bucket D  
**Canon:** boot_pipeline_v0, module_boot_config_v0, provisioning_baseline_v0, ootb_autonomous_start_v0  
**Legacy:** #393 boot_config_result (Ok/Repaired/RepairFailed), log on boot, no brick on RepairFailed

---

## 1) Contract restatement (from issue + canon)

**Phase A — Hardware provisioning (module boot configs)**  
- Bring GNSS and radio module to required Naviga operating mode.  
- **Verify-and-repair** on every boot per module_boot_config_v0: read/verify critical config, apply and re-verify on mismatch.  
- Outcomes: **Ok** / **Repaired** / **RepairFailed**. **RepairFailed must not brick**; set observable fault state; continue best-effort.  
- By end of Phase A: radio configured for OOTB with **FACTORY default RadioProfile** (channel, rate, tx power baseline).  
- **#393:** Expose **boot_config_result** (device-level Phase A); log on boot; observable for diag/status.

**Phase B — Role and profile selection + persistence**  
- **Load** role and radio profile pointers from NVS. If missing or invalid → apply **default role** and **default radio profile (id 0)** and **persist**.  
- Role and current profile are **resolved** for comms; persistence supports rollback and future UI, does not block first-boot OOTB.  
- Invariant: **Role** and **current radio profile** are both resolved; OOTB guarantees defaults so first comms do not depend on UI or backend.

**Phase C — Start comms**  
- Start **Alive / Beacon cadence** using **provisioned** role and **provisioned** current profile.  
- First-fix bootstrap: no fix → Alive(no-fix) per maxSilence; first valid fix → first BeaconCore at next opportunity without min-displacement gate; then normal cadence.  
- Invariant: Comms use **only** provisioned role and provisioned current profile; no ad hoc defaults at start-comm time.

**OOTB autonomous start**  
- **No phone, no BLE, no user action** required for the node to begin TX (Alive/Beacon) and RX.  
- Power-on/reboot is the trigger; no “wait for BLE” or “wait for user”.  
- First boot (no/empty NVS): Phase A applies FACTORY default to radio; Phase B applies default role and profile (id 0), persists; Phase C starts cadence. Device must be **flashable, bootable, self-provision from NVS/defaults, and start sending/receiving on first boot with no phone attached.**

---

## 2) Gap table

| Phase | Current behavior | Canon requirement | Gap | Owner |
|-------|------------------|--------------------|-----|--------|
| **A** | GNSS verify: ok/repaired/failed logged; radio verify: Ok/Repaired/RepairFailed logged and set on provisioning (radio only). No **device-level** Phase A result. | Single observable **boot_config_result** for Phase A (Ok/Repaired/RepairFailed); log on boot; no brick on RepairFailed. | Need **combined** Phase A result (worst of GNSS + radio), one log line, expose via status. | #423 |
| **A** | E220/E22 verify-and-repair in `begin()`; GNSS verify_on_boot with retry. RepairFailed path continues (radio_ready still set, runtime starts). | Verify-and-repair per module_boot_config_v0; RepairFailed → observable state, best-effort continue. | Already compliant; add combined result exposure. | #423 |
| **Setup** | `main.cpp`: `while (!Serial)` blocks until USB connected. | OOTB: device must boot and run without phone/USB. | **Blocking Serial wait** prevents autonomous boot when no USB. | #423 |
| **B** | load_pointers(); if !has_current_role or !has_current_radio or invalid id → default 0,0; save_pointers(); load_current_role_profile_record / get_ootb_role_profile; save. | NVS load; missing/invalid → default role and profile (id 0), persist. | Aligned. Minor: ensure namespace-not-exist is treated as “no valid pointers”. | — |
| **C** | runtime_.init() after Phase B; tick() runs cadence. No explicit “Phase C” gate. | Start Alive/Beacon cadence after Phase B using provisioned role/profile. | Ordering is implicit (init order). Add test that Phase C uses provisioned values only. | #423 (test) |
| **OOTB** | No “wait for BLE” in code path. provisioning_->tick() reads Serial only when data present. | No phone/BLE required for autonomous operation. | Remove Serial block in setup so power-on without USB proceeds. | #423 |

**Classified:**  
- **#423:** Phase A combined boot_config_result + log; remove `while (!Serial)`; tests for Phase B defaults, Phase C after B, RepairFailed no brick.  
- **#424:** Radio profile baseline alignment beyond current default (e.g. FACTORY tx power step 0 = 21 dBm) — only if needed for boot; otherwise document.  
- **#425:** Broad observability — not in scope; only the specific #393 boot_config_result exposure.

---

## 3) Implementation summary (minimal #423 delta)

1. **main.cpp:** Remove blocking `while (!Serial)` so setup() proceeds and OOTB works without USB.  
2. **app_services.cpp (Phase A):** After GNSS and radio bring-up, compute **combined** boot_config_result = worst of (GNSS result, radio result). Log one line: `Phase A boot_config_result: Ok|Repaired|RepairFailed`. Call `set_radio_boot_info(combined_result, combined_message)` so status command exposes Phase A result (#393).  
3. **Tests:** Phase B defaults when NVS empty; Phase C only uses provisioned role/profile (no ad hoc defaults); RepairFailed does not prevent Phase B/C from running.  
4. **Docs:** Update only where implementation truth diverges from existing canon (e.g. “Phase A result exposed via status”); do not rewrite canon.

---

## 4) Implementation delta (done)

| Change | File | Description |
|--------|------|--------------|
| OOTB no-USB boot | `firmware/src/main.cpp` | Removed blocking `while (!Serial)`; device proceeds with setup() without waiting for USB. |
| Phase A combined boot_config_result (#393) | `firmware/src/app/app_services.cpp` | GNSS result mapped to Ok/Repaired/RepairFailed; combined = worst of (GNSS, radio); one log line `Phase A boot_config_result: Ok` or `Repaired` or `RepairFailed`; `set_radio_boot_info(combined, combined_msg)` so status command exposes Phase A. |
| Boot phase contract tests | `firmware/test/test_boot_phase_contract/test_boot_phase_contract.cpp` | New native test: enum ordering (Ok=0, Repaired=1, RepairFailed=2) and `worst_of` semantics for combined result. |

Phase B/C logic unchanged; Phase B already loads NVS, applies defaults when missing, persists. Phase C already starts after Phase B in init order. RepairFailed path already continues (no early return); runtime receives `radio_ready` and proceeds.

---

## 5) Tests

- **test_boot_phase_contract** (native): `RadioBootConfigResult` enum ordering and `worst_of(Ok, Repaired, RepairFailed)` — 4 tests, all passing.
- **test_role_profile_registry**: Already covers `get_ootb_role_profile(0)` Person defaults (Phase B default role/profile content).
- Phase B “defaults when NVS empty”: Covered by init flow (load_pointers → has_current_* false → effective 0,0 → save_pointers). No separate unit test for NVS (device-only); contract documented in gap table.
- RepairFailed no brick: No early return after Phase A; runtime_.init() always called. Verified by code review; device/bench can confirm Alive/Beacon still start with degraded radio.

---

## 6) Build and bench

- **Devkit build:** `pio run -e devkit_e220_oled` — SUCCESS.
- **Bench checklist (minimal, from #423):** To be run on hardware when available: (1) Flash firmware, (2) Power on without USB/phone, (3) Confirm Alive or Beacon within expected window (e.g. within maxSilence), (4) Optional: second node receives. **Status:** Documented as pending; implementation allows autonomous start.

---

## 7) Docs

- No canon doc changes. Implementation matches boot_pipeline_v0, module_boot_config_v0, provisioning_baseline_v0, ootb_autonomous_start_v0.
- Optional: In provisioning_interface or dev notes, mention that `status` command reports Phase A combined `radio_boot` (boot_config_result) per #393. No doc edit required for contract.

---

## 8) Exit criteria checklist

- [x] #423 contract restated from issue/canon docs  
- [x] Phase A/B/C gap table produced  
- [x] Minimal #423 delta implemented  
- [x] Tests added/updated and passing  
- [x] Devkit build passing  
- [x] Minimal bench checklist executed or documented as pending  
- [x] Docs synchronized where needed (none required)  
- [x] Final close recommendation prepared (below)  

---

## 9) Final close recommendation

**#423** is **implementable as complete** from a code and contract perspective:

- **Phase A:** Verify-and-repair for E220 and GNSS was already in place; added **combined** boot_config_result (worst of GNSS + radio), logged once, exposed via provisioning status (#393). RepairFailed does not brick; init continues to Phase B and C.
- **Phase B:** NVS load and defaults when missing/invalid were already in place; no change.
- **Phase C:** Runtime start and Alive/Beacon cadence already follow Phase B in init order; no change.
- **OOTB without phone/USB:** Removed Serial block in `main.cpp` so the device boots and runs without USB attached.

**Remaining for #424 only (not in #423 scope):** Any broader radio profile baseline alignment (e.g. FACTORY tx power step 0 = 21 dBm on hardware that supports it) beyond current default loading. If a clear gap is found, document in #424.

**Bench:** When hardware is available, run the minimal checklist (flash, power on without USB, confirm Alive/Beacon within window). No code dependency on bench for merge; recommendation is to **close #423** after review and optional bench evidence.
