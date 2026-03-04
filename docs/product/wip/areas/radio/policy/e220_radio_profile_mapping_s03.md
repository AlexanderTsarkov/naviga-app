# Radio — E220 UART-modem profile mapping (S03)

**Status:** WIP (spec).  
**Issue:** [#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383) · **Epic:** [#353](https://github.com/AlexanderTsarkov/naviga-app/issues/353) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This doc defines the **deterministic mapping** from the S03 product-level [RadioProfile model](radio_profiles_model_s03.md) ([#382](https://github.com/AlexanderTsarkov/naviga-app/issues/382)) to E220 UART-modem (E22-400T30D / E22-400T33D) configuration. It does **not** define BLE/UI protocol, firmware implementation, or SPI mapping (SPI will map the same product tiers to SF/BW/CR later; one short note only). Aligns with [boot_pipeline_v0](../../../areas/firmware/policy/boot_pipeline_v0.md) Phase A OOTB invariant and [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md) (module-critical vs profile-applied, failure behavior).

---

## 1) Purpose and scope

- **Purpose:** Provide a single normative reference for mapping product-level `channel_slot`, `rate_tier`, and `tx_power_baseline_step` to E220 register/behavior; define clamping and diag flags; document RSSI/SNR behavior and verify/readback limitations.
- **Scope:** E22-400T30D and E22-400T33D UART backends only. No firmware code changes in this doc; no auto TX power algorithms (baseline mapping only).
- **Non-goals:** No SPI mapping (short “later” note only); no BLE/UI; no SF/BW/CR equivalence claims—airtime references only as approximate engineering estimates where useful.

---

## 2) Channel mapping (E22-400T30D datasheet aligned)

| Product | E220 CH | Notes |
|---------|---------|--------|
| **channel_slot** range | **0..83** (84 channels) | Product range. |
| Slot **0** | Reserved | **Dev/test only**; not used for OOTB. |
| **Factory default** | **Slot 1** | OOTB uses slot 1 (e.g. 411 MHz for 410 + 1). |
| Mapping | **Identity** | `channel_slot` → E220 CH register as-is. **Clamp** product value to 0..83 before writing; slot 0 allowed for dev/test but not factory default. |

**Note:** A future SPI backend may map the same product slot list to frequencies (e.g. 410.125 + CH×1 MHz) without changing the product model; this spec is UART-modem only.

---

## 3) Rate mapping (E22-400T30D air data rate bits)

**Datasheet (E220 SPED air data rate bits [2:0]):**

| Bits | Rate | Notes |
|------|------|--------|
| 010 | 2.4 kbps | Default. |
| 011 | 4.8 kbps | |
| 100 | 9.6 kbps | |
| 101 | 19.2 kbps | |
| 110 | 38.4 kbps | |
| 111 | 62.5 kbps | |

**Product tiers for UART-modem backend (S03):**

| Product rate_tier | E220 air_data_rate (bits) | Rate | Notes |
|-------------------|---------------------------|------|--------|
| **STANDARD** | 010 (2) | 2.4 kbps | **FACTORY default.** |
| **FAST** | 011 (3) | 4.8 kbps | |
| **LONG** | **NOT SUPPORTED** on T30D | — | T30D has no slower than 2.4 k. **Clamp to STANDARD** (2.4 k) and set diag flag **RATE_TIER_CLAMPED**. |

**Clamping rule:** If product requests LONG (or any tier not supported by the module), adapter MUST apply STANDARD (2.4 k) and set an observable **RATE_TIER_CLAMPED** diag flag so that boot/status can report the clamp. No silent fallback.

**Important:** Do **not** claim SF/BW/CR equivalence for UART-modem. Airtime or range references may be mentioned only as approximate engineering estimates where useful; the normative mapping is product tier → E220 air_data_rate bits above.

---

## 4) TX power mapping (OOTB requirement)

**Datasheet (E22-400T30D TX power bits):**

| Bits | dBm | Note |
|------|-----|------|
| 00 | 30 | **Module default.** |
| 01 | 27 | |
| 10 | 24 | |
| 11 | 21 | **Minimum.** |

**Product rule:** `tx_power_baseline_step`: **step 0 = MIN** (21 dBm). OOTB FACTORY default MUST use step 0.

**Deterministic mapping:**

| Product step | T30D (4 levels) | T33D (5 levels) | Clamp |
|--------------|------------------|-----------------|--------|
| 0 | 21 dBm | 21 dBm | — |
| 1 | 24 dBm | 24 dBm | — |
| 2 | 27 dBm | 27 dBm | — |
| 3 | 30 dBm | 30 dBm | — |
| 4 | *(n/a)* | 33 dBm | T30D: clamp step > 3 to 3. T33D: clamp step > 4 to 4. |

**OOTB invariant:** Module default (30 dBm) is **NOT** acceptable for OOTB. The FACTORY default profile MUST apply **MIN** (step 0 → 21 dBm) at boot (Phase A), once this mapping is implemented. See [boot_pipeline_v0](../../../areas/firmware/policy/boot_pipeline_v0.md) §3 and [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md) (profile-applied params).

---

## 5) RSSI / SNR behavior

- **RSSI append:** Enabling RSSI append is **module-critical** (see [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md)). It MUST be enabled for the UART-modem backend. The **receiver** MUST strip the appended RSSI byte(s) from the payload and store RSSI separately (e.g. for NodeTable `last_rx_rssi` / link metrics). RSSI is **runtime** only; not a profile baseline field.
- **SNR:** The **UART-modem backend has no SNR**. Document as a **limitation**: adapter/runtime MUST use a sentinel value (e.g. NA / unavailable) for SNR in telemetry and NodeTable. No SF/BW/CR or “real LoRa” knobs on this backend.

---

## 6) Verify / readback notes

**Fields that can be read back reliably** (via E220 getConfiguration or equivalent) and **should** be verified today:

| Field | Read back | Use in verify |
|-------|-----------|----------------|
| **Channel (CH)** | Yes | Compare to applied channel_slot (identity mapping). Part of module_boot_config verify-and-repair. |
| **Air data rate (SPED [2:0])** | Yes | Compare to applied rate_tier mapping. Part of verify-and-repair. |

**Fields that may not be read back** in current implementation:

| Field | Read back | Note |
|-------|-----------|------|
| **TX power** | Module-dependent; current UART impl may not read | If adapter does not read TX power back, verify outcome is **Ok** or **Repaired** based on channel + air_rate only; set **observable fault state** if apply fails. TX power apply is still required for OOTB (step 0 = MIN). |

**Tie to module_boot_config result states:** [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md) §4 defines **Ok** / **Repaired** / **RepairFailed**. When verify-and-repair runs at boot:

- **Ok:** Channel and air_rate (and TX power if read back) already match applied profile; no write needed.
- **Repaired:** One or more params were mismatched; adapter applied correct values and readback verified (for those params that are read back). Log/diag may include which params were repaired.
- **RepairFailed:** Apply or readback verify failed. Device MUST **not** brick; MUST set an **observable fault state** (e.g. boot_config_result = RepairFailed) and continue best-effort per module_boot_config_v0 §4 (progressive signaling: logs now, LED later, phone notification on connect later).

---

## 7) UART-modem limitations (explicit)

- **No “real LoRa” knobs:** E220 UART interface exposes channel, air_data_rate, and (where implemented) TX power bits—not SF/BW/CR. Do not imply SF/BW/CR equivalence for this backend.
- **No SNR:** Hardware does not provide SNR on this backend; use sentinel (NA) in telemetry.
- **Rate:** LONG (slower than 2.4 k) is not supported on T30D; clamp to STANDARD and set RATE_TIER_CLAMPED.
- **TX power:** Module default is 30 dBm; product OOTB requires MIN (21 dBm). Adapter MUST apply step 0 at boot once mapping is implemented.

---

## 8) SPI backend (short note)

A future **SPI** backend will map the **same** product model (channel_slot, rate_tier, tx_power_baseline_step) to SF/BW/CR/preamble and txPower register(s). Product model does not change; only the adapter mapping. This doc does not define SPI mapping.

---

## 9) Related

- **Product model:** [radio_profiles_model_s03.md](radio_profiles_model_s03.md) ([#382](https://github.com/AlexanderTsarkov/naviga-app/issues/382)).
- **Boot / module config:** [boot_pipeline_v0](../../../areas/firmware/policy/boot_pipeline_v0.md) (Phase A OOTB invariant), [module_boot_config_v0](../../../areas/firmware/policy/module_boot_config_v0.md) ([#381](https://github.com/AlexanderTsarkov/naviga-app/issues/381)) — module-critical vs profile-applied, failure behavior §4.
- **TX power baseline vs runtime:** [#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384).
- **Radio profiles policy:** [radio_profiles_policy_v0](../../../areas/radio/policy/radio_profiles_policy_v0.md).
