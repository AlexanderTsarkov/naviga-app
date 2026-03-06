# Firmware — fwVersionId semantics (S03)

**Status:** WIP (policy). **Issue:** [#405](https://github.com/AlexanderTsarkov/naviga-app/issues/405) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This document defines **fwVersionId** semantics for S03: what it is, why it is opaque (not SemVer-on-wire), how it is assigned and used, and normative rules so it does not affect decoding or correctness. It fits existing canon contracts and the loss-tolerance invariant.

---

## 1) Purpose and scope (S03)

- **Purpose:** Define the meaning, lifecycle, and usage of **fwVersionId** so that firmware build/release identity is consistent across NodeTable, on-air Informative packet, and future UI (S04) display.
- **Scope (S03):** fwVersionId is **informational/diagnostic only**. It identifies which firmware build/release a node is running. It MUST NOT affect protocol decoding, correctness, or feature behavior. The product MUST remain fully functional with zero Informative packets received (see [info_packet_encoding_v0.md §5](../../../../areas/nodetable/contract/info_packet_encoding_v0.md#5-loss-tolerance-invariant)).

---

## 2) Data type and encoding

- **On-air:** **uint16 LE** (2 bytes). Canon contract: [info_packet_encoding_v0.md §3.2](../../../../areas/nodetable/contract/info_packet_encoding_v0.md#32-useful-payload--informative-optional-fields-v0).
- **NodeTable:** Persisted in NodeEntry (see [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv): `fwVersionId` row; `node_entry_member` = `fw_version_id`; persisted = Y).
- **Sentinel:** **0xFFFF** = not present / absent. On RX, do not overwrite stored value. Canon: [info_packet_encoding_v0.md §4.3](../../../../areas/nodetable/contract/info_packet_encoding_v0.md#43-sentinel-handling).

No protocol layout changes; fwVersionId remains uint16 LE in Informative; sentinel rules unchanged.

---

## 3) Meaning

- **fwVersionId** = **Firmware build/release identifier** (opaque).
- It denotes which firmware build or release is running on the node. Human-readable labels (e.g. SemVer + tag) are obtained via a **registry** (see [fw_version_id_registry_v0.md](../../../../areas/firmware/registry/fw_version_id_registry_v0.md)); on-air and in NodeTable only the uint16 value is stored.

---

## 4) Why not SemVer-on-wire

- **Protocol stability:** Keeping a single uint16 avoids bitfield churn and layout changes when versioning schemes evolve.
- **Loss-tolerance:** Informative packet is optional; receivers MUST NOT depend on it for correctness. An opaque id plus registry lookup preserves that invariant while allowing flexible labeling (SemVer, tag, build id) in the registry and UI.
- **Decoding rule (normative):** Only **payloadVersion** (first byte of payload) governs decoding. fwVersionId MUST NOT be used to change decode rules or protocol behavior in S03. No “feature flags” derived from fwVersionId that alter how payloads are parsed or applied.

---

## 5) Update rules and lifecycle

- **Assigned at build/release time:** The value is set when the firmware image is built (e.g. from a build script or version header). It changes only when the firmware binary changes (e.g. new release, OTA).
- **Monotonic or curated allocation:** Allocation is either monotonic per build or curated from a registry; see [fw_version_id_registry_v0.md](../../../../areas/firmware/registry/fw_version_id_registry_v0.md) for allocation discipline and reserved ranges.
- **NodeTable:** On self, the value is written at boot from the build constant (or from a future NVS slot if “stamped” post-flash); on remote nodes, it is updated when an Informative packet with present fwVersionId is received (sentinel 0xFFFF: do not overwrite).

---

## 6) Where used

| Use | Description |
|-----|-------------|
| **On-air** | Informative packet (Node_OOTB_Informative, msg_type=0x05). Optional field; not required for correctness. See [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) §3.2. |
| **Persistence** | NodeTable (NodeEntry); persisted. See [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv) (fwVersionId row). |
| **UI (S04)** | Lookup human label via registry; mobile app may embed the registry table for offline display. |

---

## 7) Compatibility rule (normative)

- **MUST NOT** change decode rules based on fwVersionId. Only **payloadVersion** governs how the payload is decoded. fwVersionId is for identity/diagnostics only; it MUST NOT affect parsing, validation, or correctness of position/liveness/activity or any other protocol behavior.

---

## 8) Examples

| Value (hex) | Meaning |
|-------------|---------|
| **0x0042** | Example: maps to a human label such as “S03 OOTB v0.1.7” in the registry (see [fw_version_id_registry_v0.md](../../../../areas/firmware/registry/fw_version_id_registry_v0.md)). |
| **0xFFFF** | Sentinel: absent / not present. Do not overwrite stored value on RX. |

---

## 9) Related

- **Canon contract (sentinel + loss tolerance):** [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) — §3.2 byte layout, §4.3 sentinel handling, §5 loss-tolerance invariant.
- **Provisioning baseline (where product identity comes from):** [provisioning_baseline_s03.md](provisioning_baseline_s03.md) — Phase B product provisioning; fwVersionId is build identity, not a pointer from NVS like role/profile.
- **NodeTable master table (ownership):** [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv) — fwVersionId row: persisted, Informative packet, wire 2 B, optional.
- **Packet sets (u16 unchanged):** [packet_sets_v0_1.md](../../nodetable/policy/packet_sets_v0_1.md) — §4.2 conflict note; hwProfileId and fwVersionId remain uint16 LE (2 B each); no silent change.
- **Registry (human labels):** [fw_version_id_registry_v0.md](../../../../areas/firmware/registry/fw_version_id_registry_v0.md) — allocation rules, reserved/sentinel, table fwVersionId → label.
- **Future:** S04 BLE exposure of fwVersionId (and optional registry lookup for label) for mobile UI; out of scope for S03.
