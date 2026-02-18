# NodeTable — Alive packet encoding (Contract v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This contract defines the **v0 Alive packet**: a separate packet type used when the node has no valid GNSS fix. It is **alive-bearing, non-position-bearing**. Byte layout is minimal; semantics and when to send it are in [field_cadence_v0](../policy/field_cadence_v0.md) and [activity_state_v0](../policy/activity_state_v0.md). Receiver behaviour is in [rx_semantics_v0](../policy/rx_semantics_v0.md).

---

## 1) Scope and role

- **Alive packet** is a distinct message type from BeaconCore and BeaconTail-1/2. It carries **identity + freshness** only; it does **not** carry position.
- **Purpose:** Satisfy the maxSilence liveness requirement when the node cannot send BeaconCore (no valid fix). See [beacon_payload_encoding_v0](beacon_payload_encoding_v0.md) § Position-bearing vs Alive-bearing.
- **Normative:** Alive packet is **alive-bearing, non-position-bearing**. BeaconCore is **position-bearing** and is sent only when fix is valid.

---

## 2) Packet type and naming

- **Message type name:** **Alive** (or **BeaconAlive** in logs/docs when disambiguation is needed). This name explicitly distinguishes it from BeaconCore and BeaconTail-1/2.

---

## 3) Byte layout (v0)

**Byte order:** Little-endian for all multi-byte integers. **Version tag:** First byte = payload format version; v0 = `0x00`. Unknown version → discard (same rule as beacon).

### 3.1 Minimum payload (MUST)

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| payloadVersion | uint8 | 1 | 0x00 = v0 |
| nodeId | uint64 | 8 | DeviceId (same semantics as BeaconCore) |
| seq16 | uint16 | 2 | Freshness; monotonic per node. Alive uses the **same per-node seq16 counter** as BeaconCore and Tails (single counter across packet types during uptime). Same semantics for ordering/duplicate detection. |

**Minimum size:** **11 bytes.**


### 3.2 Optional: aliveStatus (v0)

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| aliveStatus | uint8 | 1 | V1-A may use only **0x00 = alive_no_fix**. All other values **reserved** for future use; receiver MUST treat as 0x00 for liveness. |

When present, payload is **12 bytes** (version + nodeId + seq16 + aliveStatus). Omission of aliveStatus is allowed; then payload is 11 bytes.

---

## 4) Examples

### 4.1 Alive (11 B): minimum

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| seq16 | 1 | 01 00 |

**Full hex:** `00 EF CD AB 89 67 45 23 01 01 00`

### 4.2 Alive (12 B): with aliveStatus

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| seq16 | 2 | 02 00 |
| aliveStatus | 0x00 (alive_no_fix) | 00 |

**Full hex:** `00 EF CD AB 89 67 45 23 01 02 00 00`

---

## 5) Related

- **Position-bearing vs Alive-bearing:** [beacon_payload_encoding_v0](beacon_payload_encoding_v0.md) — Core only with valid fix; maxSilence via Alive when no fix.
- **When to send Alive:** [field_cadence_v0](../policy/field_cadence_v0.md), [activity_state_v0](../policy/activity_state_v0.md).
- **Receiver rules:** [rx_semantics_v0](../policy/rx_semantics_v0.md) — accepted/duplicate/ooo; lastRxAt and Activity on Alive.
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
