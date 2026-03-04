# Radio — User settings review (S03)

**Status:** WIP (spec).  
**Issue:** [#356](https://github.com/AlexanderTsarkov/naviga-app/issues/356) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This doc explains, in **product terms** (not SF/BW/CR), what radio settings exist and how they are represented in S03: channel/frequency abstraction, speed steps / rate tiers, txPower step format (raw step for E22/E220; optional derived dBm), profile model (FACTORY/default → current → previous), and how txPower is exposed in telemetry/on-air (Node_Operational). It does **not** define BLE/UI protocol; **user-defined profile editing is NOT in S03** — only OOTB/default + telemetry.

---

## 1) S03 scope and constraints

- **In scope (S03):** OOTB/default radio settings + telemetry. What exists on a brand-new or factory-reset node: FACTORY default profile (channel, rate tier, tx power step), default/current/previous pointer semantics, and how these are reflected in NodeTable/on-air (e.g. Node_Operational for txPower).
- **Explicitly out of scope (S03):** **User-defined profile creation or editing.** No UI or protocol for adding/changing radio profiles in S03. No JOIN/Mesh/CAD/LBT; no legacy BLE service changes. No SF/BW/CR in user-facing settings — product uses channel_slot, rate_tier, and tx_power_baseline_step as abstractions. See [radio_profiles_model_s03.md](radio_profiles_model_s03.md) ([#382](https://github.com/AlexanderTsarkov/naviga-app/issues/382)), [radio_profiles_policy_v0](../../../areas/radio/policy/radio_profiles_policy_v0.md), [registry_radio_profiles_v0](../../../areas/radio/policy/registry_radio_profiles_v0.md).

---

## 2) Channel / frequency

- **Product abstraction:** **channel_slot** (uint8). Range **0..83** (84 channels). Slot **0** is reserved for dev/test; **factory default = 1**. Mapping to actual frequency is adapter-specific; the product does not expose raw frequency in user-facing settings. See [radio_profiles_model_s03.md](radio_profiles_model_s03.md) §2, [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) §2.
- **E22/E220 mapping:** `channel_slot` → E220 CH register **identity** (clamp to 0..83). Example: slot 1 → 411 MHz (410 + 1). No SF/BW/CR; UART backend uses CH only. See [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) §2.
- **SPI (later):** Same product slot list may map to frequencies (e.g. 410.125 + CH×1 MHz) without changing the product model; adapter owns the mapping.

---

## 3) Speed steps / rate tiers

- **Product abstraction:** **rate_tier** (product-defined step). User-facing semantics: **STANDARD** | **FAST** | **LONG**. These are **not** SF/BW/CR; product does not expose raw LoRa parameters in user-facing settings. See [radio_profiles_model_s03.md](radio_profiles_model_s03.md) §2, [registry_radio_profiles_v0](../../../areas/radio/policy/registry_radio_profiles_v0.md).
- **E220 mapping:** STANDARD → 2.4 kbps (air_data_rate 010); FAST → 4.8 kbps (011). **LONG is NOT SUPPORTED** on E22-400T30D (no slower than 2.4 k); adapter **clamps to STANDARD** and sets diag flag **RATE_TIER_CLAMPED**. See [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) §3.
- **Explicit:** No SF/BW/CR in user-facing settings; airtime/range references are approximate only where useful.

---

## 4) TX power

- **Baseline vs runtime:** **Baseline** = step stored in the active RadioProfile (`tx_power_baseline_step`). **Runtime** = current effective setting (may equal baseline or, in future, differ). Runtime **must not** overwrite baseline. Reboot defaults effective to baseline. See [tx_power_contract_s03.md](tx_power_contract_s03.md) ([#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384)), [radio_profiles_model_s03.md](radio_profiles_model_s03.md) §2.
- **Format for E22/E220:** **Raw step** is canonical for telemetry and on-air in S03. Step values map to dBm: 21, 24, 27, 30 (T30D); T33D adds 33 dBm. **dBm** is **derived/optional** (e.g. for display); product stores and transmits **step** as authoritative. See [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) §4, [tx_power_contract_s03.md](tx_power_contract_s03.md) §5.
- **Module variants and clamping:** E22-400T30D has **4** levels (steps 0..3 → 21/24/27/30 dBm); E22-400T33D has **5** (steps 0..4, step 4 → 33 dBm). Adapter clamps step to backend capability (T30D: step > 3 → 3; T33D: step > 4 → 4). OOTB FACTORY default uses **step 0 = MIN (21 dBm)**; module default 30 dBm is not acceptable for OOTB. See [provisioning_baseline_s03.md](../../firmware/policy/provisioning_baseline_s03.md) §3.3, [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) §4.

---

## 5) Profile model (FACTORY / default → current → previous)

- **FACTORY default (profile id 0):** Immutable, embedded in firmware. Content: channel_slot=1, rate_tier=STANDARD, tx_power_baseline_step=0 (MIN). Not stored in NVS; addressable by id 0. See [radio_profiles_model_s03.md](radio_profiles_model_s03.md) §3, §4.2.
- **default_profile_id:** Always **0** (FACTORY). Product constant. Used when no valid current exists and after factory reset.
- **current_profile_id:** The profile **active** at boot and at runtime. Persisted in NVS (rprof_cur / prof_cur). Phase A applies its **content** (or FACTORY if invalid/missing) to the radio; Phase B persists the pointer. See [radio_profiles_policy_v0](../../../areas/radio/policy/radio_profiles_policy_v0.md) §3.2, [provisioning_baseline_s03.md](../../firmware/policy/provisioning_baseline_s03.md) §4.2.
- **previous_profile_id:** The profile that was active before the current one. Used for quick rollback. Persisted in NVS (rprof_prev / prof_prev). Cleared on factory reset. See [radio_profiles_policy_v0](../../../areas/radio/policy/radio_profiles_policy_v0.md) §3.2.
- **First-boot behavior:** Phase A applies **FACTORY default** to the radio (no NVS read needed for OOTB). Phase B loads pointers; if missing/invalid → set current=0, previous=0 and persist. See [provisioning_baseline_s03.md](../../firmware/policy/provisioning_baseline_s03.md) §4.2, §6.

---

## 6) Telemetry / on-air (txPower in Node_Operational)

- **Where txPower appears:** **Node_Operational** (msg_type **0x04**, legacy BEACON_TAIL2). Per S03 decided constraints, **TX power MUST be sent on-air in Node_Operational** so that other nodes know the current transmit power (for future auto-tuning). See [tail2_packet_encoding_v0](../../../areas/nodetable/contract/tail2_packet_encoding_v0.md) — “Planned (S03): txPower (dynamic adaptive tx power step) will be added as an Operational field once the radio layer exposes the current tx power step.”
- **Encoding / field name:** Source of truth for field layout and packet membership: [NodeTable master table](../../nodetable/master_table/README.md), [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../nodetable/master_table/packets_v0_1.csv). For S03, **raw step** (e.g. tx_power_step_u4 or equivalent in Node_Operational) is the canonical on-air representation; dBm is derived/optional. Master table references: `Node_Operational`, `tx_power_step_u4` / `radio8` (packet_sets_v0_1); tail2_packet_encoding_v0 §3.2 for Operational optional fields. See [field_cadence_v0](../../../areas/nodetable/policy/field_cadence_v0.md) (txPower on change + at forced Core).
- **Observability:** System must expose baseline + runtime (or runtime=unknown) per [tx_power_contract_s03.md](tx_power_contract_s03.md) §4; on-air Node_Operational carries the **effective** (runtime or baseline) step for other nodes.

---

## 7) Related links

| Doc / issue | Description |
|-------------|-------------|
| [#356](https://github.com/AlexanderTsarkov/naviga-app/issues/356) | This task (Radio user settings review). |
| [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) | S03 umbrella planning. |
| [radio_profiles_model_s03.md](radio_profiles_model_s03.md) ([#382](https://github.com/AlexanderTsarkov/naviga-app/issues/382)) | Product-level RadioProfile fields, NVS schema, pointer semantics. |
| [e220_radio_profile_mapping_s03.md](e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)) | E220 channel, rate, tx power mapping; clamping. |
| [tx_power_contract_s03.md](tx_power_contract_s03.md) ([#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384)) | Baseline vs runtime vs override (reserved). |
| [provisioning_baseline_s03.md](../../firmware/policy/provisioning_baseline_s03.md) | Phase A/B, FACTORY default, fallbacks. |
| [registry_radio_profiles_v0](../../../areas/radio/policy/registry_radio_profiles_v0.md) | User-facing profile names (Default/LongDist/Fast); no SF/BW/CR UI. |
| [radio_profiles_policy_v0](../../../areas/radio/policy/radio_profiles_policy_v0.md) | Default vs user profiles, persistence, first boot, factory reset. |
| [tail2_packet_encoding_v0](../../../areas/nodetable/contract/tail2_packet_encoding_v0.md) | Node_Operational (0x04) payload; txPower planned for S03. |
| [NodeTable master table](../../nodetable/master_table/README.md) | [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../nodetable/master_table/packets_v0_1.csv) — consumer truth for packets and fields. |
