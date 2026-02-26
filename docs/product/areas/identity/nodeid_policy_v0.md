# NodeID — Policy v0

**Status:** Canon (policy).

**Work Area:** Product Specs · **Issue:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)

This document is the **single source of truth** for NodeID wire format, hardware source, domain representation, ShortId derivation, and terminology. All other docs MUST defer to this policy; do not redefine these rules inline.

---

## 1) NodeID wire format

**On-air NodeID = 6 bytes, little-endian (NodeID48).**

| Property | Value |
|----------|-------|
| Wire name | `nodeId` |
| Wire size | **6 bytes** |
| Byte order | Little-endian |
| Value range | 48-bit unsigned integer (MAC48 space) |
| Type in payload tables | `uint48` (6-byte LE) |

**Rationale:** The hardware source is a 48-bit MAC address. Encoding 8 bytes on-air wastes 2 bytes per packet for zero information. Switching to 6 bytes saves 2 bytes in every BeaconCore, Alive, Tail-1, and Tail-2 packet — meaningful for LongDist profile (budget 24 B).

---

## 2) Domain representation

**Domain NodeId = `uint64_t`; lower 48 bits = device identifier; upper 16 bits reserved = `0x0000`.**

| Property | Value |
|----------|-------|
| Domain type | `uint64_t` |
| Lower 48 bits | Device identifier (MAC48) |
| Upper 16 bits | Reserved; MUST be `0x0000` |

**Serialization to wire:** pack lower 6 bytes of `uint64_t` as little-endian (bytes 0–5 of the wire field = bits 0–47 of the uint64).

**Deserialization from wire:** zero-extend 6-byte LE value into `uint64_t`; upper 2 bytes = `0x0000`.

---

## 3) Hardware source policy (ESP32)

**NodeId source = factory/base eFuse MAC via `esp_efuse_mac_get_default` only.**

- **MUST use:** `esp_efuse_mac_get_default(base_mac)` — returns the factory-programmed 48-bit base MAC address from eFuse. Stable across reboots, firmware updates, and factory resets.
- **MUST NOT use:** `ESP_MAC_BLE`, `ESP_MAC_BT`, or any other interface-derived MAC. Interface MACs may differ from the base eFuse MAC on some ESP32 variants and are not guaranteed stable.
- **MUST NOT use:** a randomly generated MAC address as NodeId.

**Factory reset policy:** NodeId MUST NOT change on factory reset. Factory reset resets settings and profiles only; the hardware-derived NodeId is immutable.

**Future MCU without hardware MAC:** Synthesize a 48-bit locally-administered address (LAA; set bit 1 of byte 0 to `1`) on first boot; store in NVS; use as NodeId from that point. The synthesized ID MUST be stable across reboots and factory resets (read from NVS, not regenerated). This path is out of scope for V1-A.

---

## 4) ShortId (display key)

**ShortId = CRC16-CCITT-FALSE over the 6-byte little-endian wire representation of NodeId.**

| Property | Value |
|----------|-------|
| Name | `ShortId` (docs) / `short_id` (code) |
| Type | `uint16_t` |
| Algorithm | CRC16-CCITT-FALSE (`init = 0xFFFF`, `poly = 0x1021`) |
| Input | 6-byte LE wire representation of NodeId (same bytes as on-air field) |
| Purpose | Display / UI label; collision detection |

**ShortId is display-only.** It MUST NOT be used as a protocol key, dedup key, or routing identifier in any packet or protocol logic.

**Reserved values:** If CRC16 produces `0x0000` or `0xFFFF`, apply a deterministic substitution: XOR the result with `0x0001`. Document this substitution in the implementation.

**Collision handling:** When two different NodeIds produce the same ShortId, set `short_id_collision = true` on both entries. Display format: `ABCD` (no collision) or `ABCD*` / `ABCD-2` (collision). Show full NodeId hex on user request.

---

## 5) Terminology rule

**`short_id` refers exclusively to the NodeTable display key (ShortId, uint16, CRC16-derived).**

Any other use of the identifier `short_id` in docs or code is a terminology error and MUST be corrected:

| Concept | Correct term | Forbidden alias |
|---------|-------------|-----------------|
| NodeTable display key (uint16, CRC16) | `ShortId` (docs) / `short_id` (code) | — |
| Mesh session slot index (0..63) | `session_slot_id` | ~~`short_id`~~ |

**Mesh session slot (`session_slot_id`):** A 6-bit index (0..63) assigned to a node within a mesh session. Used as `origin_id`, `bridge_hop_id`, and bit index in `covered_mask`. Assignment mechanism is part of session management (TBD). This concept is distinct from NodeTable ShortId and MUST use the term `session_slot_id` in all docs and code.

---

## 6) Rationale summary

| Decision | Rationale |
|----------|-----------|
| 6-byte wire NodeId | Saves 2 B/packet; upper 16 bits are always 0x0000 anyway |
| eFuse base MAC only | Stable, factory-unique, avoids interface MAC drift across ESP32 variants |
| CRC16 over 6-byte LE input | Single canonical input aligned with on-air wire representation |
| Reserved values substitution | Avoids sentinel collisions (0x0000/0xFFFF used as "not present" in some fields) |
| `session_slot_id` rename | Eliminates naming collision with NodeTable ShortId before mesh is implemented |

---

## 7) Related

- **Beacon payload encoding:** [../nodetable/contract/beacon_payload_encoding_v0.md](../nodetable/contract/beacon_payload_encoding_v0.md) — on-air byte layouts using NodeID48.
- **Alive packet encoding:** [../nodetable/contract/alive_packet_encoding_v0.md](../nodetable/contract/alive_packet_encoding_v0.md) — Alive packet using NodeID48.
- **Fields inventory:** [../nodetable/policy/nodetable_fields_inventory_v0.md](../nodetable/policy/nodetable_fields_inventory_v0.md) — ShortId row.
- **Mesh concept:** [../../naviga_mesh_protocol_concept_v1_4.md](../../naviga_mesh_protocol_concept_v1_4.md) — uses `session_slot_id` (0..63) for mesh slot.
- **Audit:** [../../wip/research/nodeid_audit_s02.md](../../wip/research/nodeid_audit_s02.md) — findings that drove this policy.
- **Issue:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)
