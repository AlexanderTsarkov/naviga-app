# Firmware — hwProfileId registry v0

**Status:** Canon (registry). **Issue:** [#406](https://github.com/AlexanderTsarkov/naviga-app/issues/406) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This document defines the **hwProfileId** registry: allocation discipline, reserved values, and a table mapping uint16 values to human-readable labels and **implied attributes** (radio SKU, GNSS class, power class, notes). It is the single source for “what does this id mean” and can be embedded in the mobile app for offline display.

---

## 1) Allocation rules

- **Discipline:** Values are allocated per **hardware variant** (board + radio SKU + GNSS/capabilities class). One id per distinct physical configuration class that the product needs to distinguish (e.g. for OOTB baselines or diagnostics).
- **Reserved:** **0xFFFF** = sentinel “not present” (canon: [info_packet_encoding_v0.md §4.3](../../nodetable/contract/info_packet_encoding_v0.md#43-sentinel-handling)). Do not allocate.
- **Optional reserved:** **0x0000** = “unknown / unstamped” (e.g. legacy or undetected hardware). If used, receivers treat as “hardware class unknown”; do not use for a real variant id if 0x0000 is reserved.
- **Range:** Allocated ids should stay within 0x0001–0xFFFE unless 0x0000 is used for unknown.

---

## 2) Registry table

| hwProfileId (hex) | Human label | Implied attributes (radio SKU, gnss class, notes) |
|-------------------|-------------|---------------------------------------------------|
| 0x0000 | (reserved: unknown/unstamped) | Optional; do not allocate for variants. |
| 0x0001 | E220 T30D, GNSS NEO (example) | radio_sku: E22-400T30D; gnss_profile: NEO family; OOTB baseline step 0 = 21 dBm. |
| … | … | Add rows as hardware variants are defined. |
| 0xFFFF | (reserved: not present) | Sentinel; never allocated. |

*Mobile app may embed this table (or a generated mapping) to display labels and optional capability hints offline.*

---

## 3) Related

- **Semantics (what/why/how):** [hw_profile_id_semantics_s03.md](../../wip/areas/firmware/policy/hw_profile_id_semantics_s03.md) ([#406](https://github.com/AlexanderTsarkov/naviga-app/issues/406)) — S03 WIP policy; encoding, lifecycle, “must not affect decoding,” implied attributes.
- **Canon contract:** [info_packet_encoding_v0.md](../../nodetable/contract/info_packet_encoding_v0.md) — §3.2 hwProfileId uint16 LE, §4.3 sentinel 0xFFFF.
- **E220 / TX power (why SKU matters):** [e220_radio_profile_mapping_s03.md](../../radio/policy/e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)), [tx_power_contract_s03.md](../../radio/policy/tx_power_contract_s03.md) ([#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384)).
