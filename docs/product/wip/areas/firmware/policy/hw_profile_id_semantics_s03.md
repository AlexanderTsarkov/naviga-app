# Firmware — hwProfileId semantics (S03)

**Status:** WIP (policy). **Issue:** [#406](https://github.com/AlexanderTsarkov/naviga-app/issues/406) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This document defines **hwProfileId** semantics for S03: what it is, why it exists (diagnostics, OOTB baselines, mobile display), how it is assigned and used, and normative rules so it does not affect decoding or correctness. It fits existing canon contracts and the loss-tolerance invariant.

---

## 1) Purpose and scope (S03)

- **Purpose:** Define the meaning, lifecycle, and usage of **hwProfileId** so that hardware configuration identity is consistent across NodeTable, on-air Informative packet, and future UI (S04) display.
- **Scope (S03):** hwProfileId is **informational/diagnostic/config identity only**. It identifies the hardware configuration class (board + radio SKU + GNSS/capabilities). It MUST NOT affect protocol decoding, correctness, or feature behavior. The product MUST remain fully functional with zero Informative packets received (see [info_packet_encoding_v0.md §5](../../../../areas/nodetable/contract/info_packet_encoding_v0.md#5-loss-tolerance-invariant)).

---

## 2) Data type and encoding

- **On-air:** **uint16 LE** (2 bytes). Canon contract: [info_packet_encoding_v0.md §3.2](../../../../areas/nodetable/contract/info_packet_encoding_v0.md#32-useful-payload--informative-optional-fields-v0).
- **NodeTable:** Persisted in NodeEntry (see [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv): `hwProfileId` row; `node_entry_member` = `hw_profile_id`; persisted = Y).
- **Sentinel:** **0xFFFF** = not present / absent. On RX, do not overwrite stored value. Canon: [info_packet_encoding_v0.md §4.3](../../../../areas/nodetable/contract/info_packet_encoding_v0.md#43-sentinel-handling).

No protocol layout changes; hwProfileId remains uint16 LE in Informative; sentinel rules unchanged.

---

## 3) Meaning

- **hwProfileId** = **Hardware configuration profile identifier** (opaque).
- It denotes the **hardware variant class**: board, radio SKU (e.g. E22-400T30D vs T33D), GNSS/capabilities family, power class, and optionally sensor/debug flags. Human-readable labels and implied attributes (radio_sku, gnss_profile family, power class) are obtained via a **registry** (see [hw_profile_id_registry_v0.md](../../../../areas/firmware/registry/hw_profile_id_registry_v0.md)); on-air and in NodeTable only the uint16 value is stored.

---

## 4) Why it exists

- **Diagnostics:** Identify which hardware configuration a node has for support and telemetry.
- **OOTB baselines:** Different radio SKUs have different safe defaults (e.g. TX power max differs by module; see [e220_radio_profile_mapping_s03.md](../../radio/policy/e220_radio_profile_mapping_s03.md), [tx_power_contract_s03.md](../../radio/policy/tx_power_contract_s03.md)). hwProfileId allows the registry to describe implied constraints without changing protocol behavior.
- **Mobile display (S04):** Lookup human label and optional capability hints for UI; app may embed the registry for offline display.

---

## 5) What it MUST NOT do (normative)

- **MUST NOT** change decode rules based on hwProfileId. Only **payloadVersion** governs how the payload is decoded. hwProfileId is for identity/diagnostics only; it MUST NOT affect parsing, validation, or correctness of position/liveness/activity or any other protocol behavior.
- **MUST NOT** be required for correctness. The loss-tolerance invariant holds: the product MUST function with zero Informative packets. No “feature flags” derived from hwProfileId that alter protocol behavior in S03.

---

## 6) Lifecycle

- **Assigned per hardware variant:** The value is determined by the physical configuration class (board + radio SKU + GNSS profile family, etc.). It is **stable across firmware builds**; it changes only when the physical config class changes (e.g. different radio module, different board revision).
- **Set at manufacture / provisioning:** Typically fixed for a given unit (e.g. stamped at factory or set once at first boot from hardware detection). Not a runtime pointer like role or radio profile; see [provisioning_baseline_s03.md](provisioning_baseline_s03.md) for Phase A/B hardware vs product provisioning.
- **NodeTable:** On self, the value is written from hardware identity (e.g. build constant or one-time detection); on remote nodes, it is updated when an Informative packet with present hwProfileId is received (sentinel 0xFFFF: do not overwrite).

---

## 7) Minimal schema (implied attributes in registry)

The registry may describe **implied attributes** per hwProfileId (for documentation and optional UI); these do not change wire format or decode rules. Typical attributes:

- **radio_sku** (e.g. T30D / T33D) — E220 variant; affects TX power step bounds and mapping (see [e220_radio_profile_mapping_s03.md](../../radio/policy/e220_radio_profile_mapping_s03.md)).
- **gnss_profile family** — GNSS capability class (e.g. u-blox NEO, etc.).
- **power class** — Power/battery class if relevant for diagnostics.
- **Optional:** sensor flags, debug/OLED presence, etc., where useful for display or provisioning hints.

See [hw_profile_id_registry_v0.md](../../../../areas/firmware/registry/hw_profile_id_registry_v0.md) for the table and allocation rules.

---

## 8) Where used

| Use | Description |
|-----|-------------|
| **On-air** | Informative packet (Node_OOTB_Informative, msg_type=0x05). Optional field; not required for correctness. See [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) §3.2. |
| **Persistence** | NodeTable (NodeEntry); persisted. See [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv) (hwProfileId row). |
| **UI (S04)** | Lookup human label and optional implied attributes via registry; mobile app may embed the registry table for offline display. |

---

## 9) Examples and sentinel behavior

| Value (hex) | Meaning |
|-------------|---------|
| **0x0001** | Example: maps to a human label and implied attributes (e.g. “E220 T30D, GNSS NEO”) in the registry (see [hw_profile_id_registry_v0.md](../../../../areas/firmware/registry/hw_profile_id_registry_v0.md)). |
| **0xFFFF** | Sentinel: absent / not present. Do not overwrite stored value on RX. |
| **0x0000** | Optional reserved: “unknown / unstamped”; use only if defined in registry rules. |

---

## 10) Related

- **Canon contract (sentinel + loss tolerance):** [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) — §3.2 byte layout, §4.3 sentinel handling, §5 loss-tolerance invariant.
- **Provisioning baseline (where hardware identity comes from):** [provisioning_baseline_s03.md](provisioning_baseline_s03.md) — Phase A hardware provisioning, Phase B product provisioning; hwProfileId is hardware config identity, not a pointer from NVS like role/profile.
- **E220 mapping / TX power (why hwProfile matters for baselines):** [e220_radio_profile_mapping_s03.md](../../radio/policy/e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)), [tx_power_contract_s03.md](../../radio/policy/tx_power_contract_s03.md) ([#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384)) — radio SKU affects TX power step bounds; [boot_pipeline_v0](../../../../areas/firmware/policy/boot_pipeline_v0.md) Phase A/B/C.
- **NodeTable master table (ownership):** [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv) — hwProfileId row: persisted, Informative packet, wire 2 B, optional.
- **Packet sets (u16 unchanged):** [packet_sets_v0_1.md](../../nodetable/policy/packet_sets_v0_1.md) — §4.2 conflict note; hwProfileId and fwVersionId remain uint16 LE (2 B each); no silent change.
- **Registry (human labels + implied attributes):** [hw_profile_id_registry_v0.md](../../../../areas/firmware/registry/hw_profile_id_registry_v0.md) — allocation rules, reserved/sentinel, table hwProfileId → label → implied attributes.
- **Future:** S04 BLE exposure of hwProfileId (and optional registry lookup for label/capabilities) for mobile UI; out of scope for S03.
