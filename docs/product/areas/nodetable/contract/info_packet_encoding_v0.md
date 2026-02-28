# NodeTable — Node_OOTB_Informative packet encoding (Contract v0)

**Status:** Canon (contract).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Canon cutover:** [#314](https://github.com/AlexanderTsarkov/naviga-app/issues/314) · **Umbrella:** [#318](https://github.com/AlexanderTsarkov/naviga-app/issues/318) · **Beacon encoding hub:** [beacon_payload_encoding_v0.md](beacon_payload_encoding_v0.md) · **NodeID policy:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) · **Packet header framing:** [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)

This contract defines the **v0 `Node_OOTB_Informative` payload** (`msg_type=0x05`, legacy wire enum: `BEACON_INFO`): byte layout, field semantics, units, ranges, and RX rules. `Node_OOTB_Informative` carries **static or user-configured fields** that rarely change (device identity, liveness config). It is **optional, loss-tolerant**, and **never required for correctness**.

For dynamic runtime fields (`batteryPercent`, `uptimeSec`), see the companion contract: [tail2_packet_encoding_v0.md](tail2_packet_encoding_v0.md) (`Node_OOTB_Operational`, `msg_type=0x04`).

---

## 0) Frame header (framing layer — not part of this contract)

Every on-air `Node_OOTB_Informative` frame is preceded by a **2-byte frame header** defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0).

- **`msg_type` for Node_OOTB_Informative:** `0x05` (`BEACON_INFO`). Dispatched independently at the framing layer.
- **`payload_len`** in the header = number of payload bytes (9 minimum; see §3 below). Does not include the 2-byte header.
- **`payloadVersion`** is the **first byte of the payload** (byte 0 of the payload body). It is not a header field.
- **On-air total size:** 2 (header) + 9 (min payload) = **11 bytes minimum**; 2 + 14 (full payload) = **16 bytes maximum**. Both fit within LongDist budget (24 B).

---

## 1) Role and invariants

- **Node_OOTB_Informative carries static/config state:** Fields that rarely change — set at manufacture (`hwProfileId`) or on OTA (`fwVersionId`), or user-configured (`maxSilence10s`). These are Informative fields; they are sent on-change and at a fixed slow cadence (default 10 min).
- **Optional and loss-tolerant:** Packets may be missing, reordered, or dropped. The product MUST remain fully functional (position tracking, liveness) with zero packets received.
- **Never moves position:** MUST NOT update `lat`, `lon`, `alt`, or any position-derived field. Position is owned exclusively by `Node_OOTB_Core_Pos`.
- **Cadence:** On static/config field change + fixed slow cadence (default 10 min). MUST NOT be sent on every `Node_OOTB_Operational` send.
- **Operational fields are NOT in this packet:** `batteryPercent`, `uptimeSec` are carried by `Node_OOTB_Operational` (`0x04`). See [tail2_packet_encoding_v0.md](tail2_packet_encoding_v0.md).

---

## 2) Packet type and naming

- **Canonical alias:** `Node_OOTB_Informative`. Legacy wire enum: `BEACON_INFO`. See [ootb_radio_v0.md §3.0a](../../../../protocols/ootb_radio_v0.md#30a-canonical-packet-naming) for the full alias table.
- **`msg_type` value:** `0x05` (`BEACON_INFO`) in the 2-byte frame header. See [ootb_radio_v0.md §3.2](../../../../protocols/ootb_radio_v0.md#32-msg_type-registry-v0) for the full registry.
- **Distinct packet family:** `msg_type = 0x05` is dispatched independently from all other `Node_*` packets.

---

## 3) Byte layout (v0)

**Byte order:** Little-endian for all multi-byte integers. **Version tag:** First byte = `payloadVersion`; v0 = `0x00`. Unknown version → discard. The 2-byte frame header precedes this payload; payload byte offsets below start at byte 0 of the payload body (after the header).

### 3.1 Common prefix (MUST)

The first 9 bytes are the **Common prefix** shared by all `Node_*` packets (see [ootb_radio_v0.md §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants)):

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 0 | `payloadVersion` | uint8 | 1 | `0x00` = v0. Determines the entire payload layout. Unknown value → discard. |
| 1–6 | `nodeId` | uint48 LE | 6 | NodeID48 (6-byte LE MAC48); see [nodeid_policy_v0](../../../identity/nodeid_policy_v0.md). |
| 7–8 | `seq16` | uint16 LE | 2 | Global per-node counter. Dedupe key: `(nodeId48, seq16)`. |

**Minimum size:** **9 bytes** (Common only). On-air with 2-byte header: **11 bytes**.

### 3.2 Useful payload — Informative optional fields (v0)

Appended in order after the Common prefix. Fields may be omitted from the end.

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 9 | `maxSilence10s` | uint8 | 1 | **Informative.** Max silence in 10-second steps; range 0..255; `0x00` = absent/unknown; 255 = 2550 s ≈ 42.5 min. Defines the sender's liveness guarantee: within this window the node MUST transmit at least one alive-bearing packet. See [field_cadence_v0.md §2.2](../policy/field_cadence_v0.md). |
| 10–11 | `hwProfileId` | uint16 LE | 2 | Hardware profile identifier. `0xFFFF` = not present. Set at manufacture; changes only on hardware revision. |
| 12–13 | `fwVersionId` | uint16 LE | 2 | Firmware version identifier. `0xFFFF` = not present. Changes only on OTA update. |

**Maximum size (all optional present):** 1+6+2+1+2+2 = **14 bytes** payload; **16 bytes** on-air. Fits within LongDist budget (24 B).

---

## 4) RX rules (normative)

### 4.1 Acceptance (no CoreRef)

`Node_OOTB_Informative` has **no** `ref_core_seq16` linkage. On receiving a packet for node `N`:

1. **Version check:** If `payloadVersion != 0x00` → drop and log; do not apply.
2. **Length check:** If `payload_len < 9` → drop packet.
3. **Apply:** If version and length valid → apply all present fields to NodeTable for node `N`. Update `lastRxAt` and link metrics.
4. **Unknown node:** If no prior state exists for node `N` → create entry and apply fields.

### 4.2 Position invariant (MUST NOT)

MUST NOT update `lat`, `lon`, `alt`, or any position-derived field, regardless of content.

### 4.3 Sentinel handling

| Field | Sentinel | Meaning |
|-------|----------|---------|
| `maxSilence10s` | `0x00` | Absent/unknown; do not overwrite stored value |
| `hwProfileId` | `0xFFFF` | Not present; do not overwrite stored value |
| `fwVersionId` | `0xFFFF` | Not present; do not overwrite stored value |

### 4.4 Version and length guards

| Condition | Action |
|-----------|--------|
| `payloadVersion != 0x00` | Drop and log; do not attempt partial decode |
| `payload_len < 9` (minimum) | Drop packet |
| `payload_len` shorter than offset of a field being decoded | Treat field as absent (use sentinel / not present) |

---

## 5) Loss-tolerance invariant

> The product MUST function correctly (position tracking, liveness, activity derivation) with **zero** `Node_OOTB_Informative` packets received.
>
> Absence is the normal operating condition for low-bandwidth or congested channels. Receivers MUST NOT require this packet for any correctness guarantee.

---

## 6) Examples

### 6.1 Node_OOTB_Informative (9 B): minimum (Common only)

| Field | Section | Value | Bytes (hex) |
|-------|---------|-------|-------------|
| payloadVersion | Common | 0 | `00` |
| nodeId | Common | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | Common | 4 | `04 00` |

**Full hex (9 bytes):** `00 FF EE DD CC BB AA 04 00`

### 6.2 Node_OOTB_Informative (10 B): with maxSilence10s

| Field | Section | Value | Bytes (hex) |
|-------|---------|-------|-------------|
| payloadVersion | Common | 0 | `00` |
| nodeId | Common | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | Common | 4 | `04 00` |
| maxSilence10s | Useful payload | 9 (= 90 s) | `09` |

**Full hex (10 bytes):** `00 FF EE DD CC BB AA 04 00 09`

### 6.3 Node_OOTB_Informative (14 B): full payload

| Field | Section | Value | Bytes (hex) |
|-------|---------|-------|-------------|
| payloadVersion | Common | 0 | `00` |
| nodeId | Common | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | Common | 4 | `04 00` |
| maxSilence10s | Useful payload | 9 (= 90 s) | `09` |
| hwProfileId | Useful payload | 0x0001 | `01 00` |
| fwVersionId | Useful payload | 0x0042 | `42 00` |

**Full hex (14 bytes):** `00 FF EE DD CC BB AA 04 00 09 01 00 42 00`

---

## 7) Versioning / compat

- **Unknown `payloadVersion`:** If `payloadVersion != 0x00`, receiver MUST discard the payload for NodeTable update purposes. Do not attempt partial decode.
- **v0.x:** Reserved for backward-compatible extensions (new optional fields at end). Until defined, only `0x00` is valid.
- **Layer separation:** `msg_type` (frame header) and `payloadVersion` (payload byte 0) are independent axes. See [ootb_radio_v0.md §3.3](../../../../protocols/ootb_radio_v0.md#33-layer-separation).

---

## 8) Related

- **Beacon encoding hub (packet overview):** [beacon_payload_encoding_v0.md](beacon_payload_encoding_v0.md) — §3 packet types table; §4.4 this packet's summary.
- **Node_OOTB_Operational (companion, 0x04):** [tail2_packet_encoding_v0.md](tail2_packet_encoding_v0.md) — dynamic runtime fields: `batteryPercent`, `uptimeSec`.
- **Node_OOTB_Core_Tail:** [tail1_packet_encoding_v0.md](tail1_packet_encoding_v0.md) — Core-attached extension; dual-seq.
- **Alive packet:** [alive_packet_encoding_v0.md](alive_packet_encoding_v0.md) — alive-bearing, non-position; no-fix liveness.
- **RX semantics (apply rules):** [../policy/rx_semantics_v0.md](../policy/rx_semantics_v0.md) — §2 acceptance; dedupe by `(nodeId48, seq16)`.
- **Field cadence (Operational vs Informative packets):** [../policy/field_cadence_v0.md](../policy/field_cadence_v0.md) — §2.2 two-packet split; `maxSilence10s` cadence.
- **Activity state (maxSilence10s consumer):** [../policy/activity_state_v0.md](../policy/activity_state_v0.md) — uses `maxSilence10s` from this packet for liveness thresholds.
- **Role profiles (maxSilence10s source):** [../../../domain/policy/role_profiles_policy_v0.md](../../../domain/policy/role_profiles_policy_v0.md) — role profile defines `maxSilence10s` value.
- **Packet anatomy & naming:** [ootb_radio_v0.md §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants) — Common prefix, seq16 invariants, canonical alias table.
- **NodeID policy:** [../../../identity/nodeid_policy_v0.md](../../../identity/nodeid_policy_v0.md) — [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)
- **Packet header framing:** [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0) — [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
