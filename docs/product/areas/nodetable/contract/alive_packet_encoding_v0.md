# NodeTable — Alive packet encoding (Contract v0)

**Status:** Canon (contract).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **NodeID policy:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) · **Packet header framing:** [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)

This contract defines the **v0 Alive packet**: a separate packet type used when the node has no valid GNSS fix. It is **alive-bearing, non-position-bearing**. Byte layout is minimal; semantics and when to send it are in [field_cadence_v0](../policy/field_cadence_v0.md) and [activity_state_v0](../policy/activity_state_v0.md). Receiver behaviour is in [rx_semantics_v0](../policy/rx_semantics_v0.md).

---

## 0) Frame header (framing layer — not part of this contract)

Every on-air Alive frame is preceded by a **2-byte frame header** defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0).

- **`msg_type` for Alive:** `0x02` (`BEACON_ALIVE`; canonical alias: `Node_OOTB_I_Am_Alive`). This is a **distinct** `msg_type` from `Node_OOTB_Core_Pos` (`0x01`). Alive is **not** encoded as "Core without a fix" — it is a separate packet family dispatched independently at the framing layer.
- **`payload_len`** in the header = number of payload bytes (9 or 10; see §3 below). Does not include the 2-byte header.
- **`payloadVersion`** is the **first byte of the payload** (byte 0 of the payload body). It is not a header field.
- **On-air total size:** 2 (header) + 9 (min payload) = **11 bytes minimum**; 2 + 10 = **12 bytes with aliveStatus**. Both fit within LongDist budget (24 B).

---

## 1) Scope and role

- **Alive packet** is a distinct message type from BeaconCore and BeaconTail-1/2. It carries **identity + freshness** only; it does **not** carry position.
- **Purpose:** Satisfy the maxSilence liveness requirement when the node cannot send BeaconCore (no valid fix). See [beacon_payload_encoding_v0](beacon_payload_encoding_v0.md) § Position-bearing vs Alive-bearing.
- **Normative:** Alive packet is **alive-bearing, non-position-bearing**. BeaconCore is **position-bearing** and is sent only when fix is valid.

---

## 2) Packet type and naming

- **Canonical alias:** `Node_OOTB_I_Am_Alive`. Legacy wire enum: `BEACON_ALIVE`. See [ootb_radio_v0.md §3.0a](../../../../protocols/ootb_radio_v0.md#30a-canonical-packet-naming) for the full alias table.
- **`msg_type` value:** `0x02` (`BEACON_ALIVE`) in the 2-byte frame header. See [ootb_radio_v0.md §3.2](../../../../protocols/ootb_radio_v0.md#32-msg_type-registry-v0) for the full registry.
- **Not a subtype of Core:** Alive is a **distinct packet family** (`msg_type = 0x02`), not a variant of `Node_OOTB_Core_Pos` (`msg_type = 0x01`). A receiver dispatches Alive and Core independently at the framing layer; it does not infer packet type from payload length or `pos_valid` flags.

---

## 3) Byte layout (v0)

**Byte order:** Little-endian for all multi-byte integers. **Version tag:** First byte = `payloadVersion`; v0 = `0x00`. Unknown version → discard (same rule as beacon). The 2-byte frame header precedes this payload; payload byte offsets below start at byte 0 of the payload body (after the header).

### 3.1 Minimum payload (MUST)

The first 9 bytes are the **Common prefix** shared by all `Node_*` packets (see [ootb_radio_v0.md §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants)):

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 0 | payloadVersion | uint8 | 1 | `0x00` = v0 |
| 1–6 | nodeId | uint48 | 6 | NodeID48 (6-byte LE MAC48); same semantics as `Node_OOTB_Core_Pos`. See [nodeid_policy_v0](../../../identity/nodeid_policy_v0.md). |
| 7–8 | seq16 | uint16 LE | 2 | Global per-node counter. Alive uses the **same per-node seq16 counter** as all other `Node_*` packets (single counter across packet types during uptime). Dedupe key: `(nodeId48, seq16)`. |

**Minimum size:** **9 bytes** (Common only). On-air with 2-byte header: **11 bytes**.

### 3.2 Optional: aliveStatus (v0)

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| aliveStatus | uint8 | 1 | V1-A may use only **0x00 = alive_no_fix**. All other values **reserved** for future use; receiver MUST treat as 0x00 for liveness. |

When present, payload is **10 bytes** (version + nodeId + seq16 + aliveStatus). On-air with 2-byte header: **12 bytes**. Omission of aliveStatus is allowed; then payload is 9 bytes (11 bytes on-air).

---

## 4) Examples

### 4.1 Alive (9 B): minimum

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0000_AABB_CCDD_EEFF | FF EE DD CC BB AA |
| seq16 | 1 | 01 00 |

**Full hex:** `00 FF EE DD CC BB AA 01 00`

### 4.2 Alive (10 B): with aliveStatus

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0000_AABB_CCDD_EEFF | FF EE DD CC BB AA |
| seq16 | 2 | 02 00 |
| aliveStatus | 0x00 (alive_no_fix) | 00 |

**Full hex:** `00 FF EE DD CC BB AA 02 00 00`

---

## 5) Related

- **Position-bearing vs Alive-bearing:** [beacon_payload_encoding_v0](beacon_payload_encoding_v0.md) — Core only with valid fix; maxSilence via Alive when no fix.
- **When to send Alive:** [field_cadence_v0](../policy/field_cadence_v0.md), [activity_state_v0](../policy/activity_state_v0.md).
- **Receiver rules:** [rx_semantics_v0](../policy/rx_semantics_v0.md) — accepted/duplicate/ooo; lastRxAt and Activity on Alive.
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
- **NodeID policy (wire format, source, ShortId):** [../../../identity/nodeid_policy_v0.md](../../../identity/nodeid_policy_v0.md) — [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)
- **Packet header framing (2B 7+3+6, msg_type=0x02 for Alive):** [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0) — [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)
