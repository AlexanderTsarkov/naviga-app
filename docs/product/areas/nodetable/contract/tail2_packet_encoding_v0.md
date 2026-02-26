# NodeTable — BeaconTail-2 packet encoding (Contract v0)

**Status:** Canon (contract).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Tail productization:** [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307) · **Beacon encoding hub:** [beacon_payload_encoding_v0.md](beacon_payload_encoding_v0.md) · **NodeID policy:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) · **Packet header framing:** [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)

This contract defines the **v0 BeaconTail-2 payload**: byte layout, field semantics, units, ranges, and RX rules. BeaconTail-2 carries **slow-state / diagnostic** data (battery, temperature, device identity) that is **not** tied to a specific Core sample. It is **optional, loss-tolerant**, and **never required for correctness**.

---

## 0) Frame header (framing layer — not part of this contract)

Every on-air BeaconTail-2 frame is preceded by a **2-byte frame header** defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0).

- **`msg_type` for BeaconTail-2:** `0x04` (`BEACON_TAIL2`). Dispatched independently at the framing layer.
- **`payload_len`** in the header = number of payload bytes (7 minimum; see §3 below). Does not include the 2-byte header.
- **`payloadVersion`** is the **first byte of the payload** (byte 0 of the payload body). It is not a header field.
- **On-air total size:** 2 (header) + 7 (min payload) = **9 bytes minimum**; 2 + 17 (full payload) = **19 bytes maximum**. Both fit within LongDist budget (24 B).

---

## 1) Role and invariants

- **BeaconTail-2 is uncoupled slow state:** Unlike Tail-1, it carries no `core_seq16` and is not attached to a specific Core sample. It is applied unconditionally when received (version and nodeId valid).
- **Optional and loss-tolerant:** Tail-2 packets may be missing, reordered, or dropped. The product MUST remain fully functional (position tracking, liveness) with zero Tail-2 packets received.
- **Never moves position:** Tail-2 MUST NOT update `lat`, `lon`, `alt`, or any position-derived field. Position is owned exclusively by BeaconCore.
- **Two scheduling classes:** See [field_cadence_v0.md §2.2](../policy/field_cadence_v0.md) — Operational (on change + at forced Core) and Informative (on change + default 10 min). `maxSilence10s` is Informative and MUST NOT be included on every operational Tail-2 send unless its value changed.

---

## 2) Packet type and naming

- **Message type name:** **BeaconTail-2** (or **Tail-2** in policy/cadence docs).
- **`msg_type` value:** `0x04` (`BEACON_TAIL2`) in the 2-byte frame header. See [ootb_radio_v0.md §3.2](../../../../protocols/ootb_radio_v0.md#32-msg_type-registry-v0) for the full registry.
- **Distinct packet family:** `msg_type = 0x04` is dispatched independently from BeaconCore (`0x01`), BeaconAlive (`0x02`), and BeaconTail-1 (`0x03`).

---

## 3) Byte layout (v0)

**Byte order:** Little-endian for all multi-byte integers. **Signed fields:** `temp_x10` is signed (int16 LE); `rssi_dbm` is signed (int8). **Version tag:** First byte = `payloadVersion`; v0 = `0x00`. Unknown version → discard. The 2-byte frame header precedes this payload; payload byte offsets below start at byte 0 of the payload body (after the header).

### 3.1 Minimum payload (MUST)

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 0 | `payloadVersion` | uint8 | 1 | `0x00` = v0. Determines the entire payload layout. Unknown value → discard. |
| 1 | `nodeId` | uint48 LE | 6 | NodeID48 (6-byte LE MAC48); same semantics as BeaconCore. See [nodeid_policy_v0](../../../identity/nodeid_policy_v0.md). |

**Minimum size:** **7 bytes** (payload only). On-air with 2-byte header: **9 bytes**.

### 3.2 Optional: liveness hint (v0)

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 7 | `maxSilence10s` | uint8 | 1 | **Informative.** Max silence in 10-second steps; clamp ≤ 90 (= 15 min). `0x00` = not specified. **MUST NOT** be included on every operational Tail-2 send unless value changed. See [field_cadence_v0.md §2.2](../policy/field_cadence_v0.md). |

With liveness hint: **8 bytes** payload; **10 bytes** on-air.

### 3.3 Optional: health fields (v0)

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 8 | `batteryPercent` | uint8 | 1 | Battery level 0–100 %. `0xFF` = not present. |
| 9 | `temp_x10` | int16 LE | 2 | Temperature × 10 °C, **signed**. Range: −3276.7 °C to +3276.7 °C (practical: −40 °C to +85 °C). `0x8000` = not present (sentinel). Example: 23.5 °C → `235` (`0x00EB`); −10.0 °C → `−100` (`0xFF9C`). |
| 11 | `rssi_dbm` | int8 | 1 | Self-measured RSSI at TX time, **signed**, in dBm. Range: −128 to +127. `0x80` (= −128) = not present (sentinel). Example: −85 dBm → `0xAB`. |

### 3.4 Optional: device identity fields (v0)

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 12 | `hwProfileId` | uint16 LE | 2 | Hardware profile identifier. `0xFFFF` = not present. |
| 14 | `fwVersionId` | uint16 LE | 2 | Firmware version identifier. `0xFFFF` = not present. |
| 16 | `uptimeSec` | uint32 LE | 4 | Node uptime in seconds since last boot. `0xFFFFFFFF` = not present. |

**Maximum size (all optional present):** 1+6+1+1+2+1+2+2+4 = **20 bytes** payload; **22 bytes** on-air. Fits within LongDist budget (24 B).

> **Note on field ordering:** Fields MUST be included in the order defined above. Fields may be omitted from the end of the payload, but MUST NOT be reordered or skipped in the middle. If a field in the middle is not present, use its sentinel value and include it; do not skip to a later field.

---

## 4) RX rules (normative)

### 4.1 Acceptance (no CoreRef)

BeaconTail-2 has **no** `core_seq16` linkage. On receiving a BeaconTail-2 for node `N`:

1. **Version check:** If `payloadVersion != 0x00` → drop and log; do not apply.
2. **Length check:** If `payload_len < 7` → drop packet.
3. **Apply:** If version and length valid → apply all present fields to NodeTable for node `N`. Update `lastRxAt` and link metrics.
4. **Unknown node:** If no prior state exists for node `N` → create entry and apply fields.

### 4.2 Position invariant (MUST NOT)

Tail-2 MUST NOT update `lat`, `lon`, `alt`, or any position-derived field, regardless of content.

### 4.3 Sentinel handling

| Field | Sentinel | Meaning |
|-------|----------|---------|
| `batteryPercent` | `0xFF` | Not present; do not overwrite stored value |
| `temp_x10` | `0x8000` | Not present; do not overwrite stored value |
| `rssi_dbm` | `0x80` (= −128) | Not present; do not overwrite stored value |
| `hwProfileId` | `0xFFFF` | Not present; do not overwrite stored value |
| `fwVersionId` | `0xFFFF` | Not present; do not overwrite stored value |
| `uptimeSec` | `0xFFFFFFFF` | Not present; do not overwrite stored value |

### 4.4 Version and length guards

| Condition | Action |
|-----------|--------|
| `payloadVersion != 0x00` | Drop and log; do not attempt partial decode |
| `payload_len < 7` (minimum) | Drop packet |
| `payload_len` shorter than offset of a field being decoded | Treat field as absent (use sentinel / not present) |

---

## 5) Loss-tolerance invariant

> The product MUST function correctly (position tracking, liveness, activity derivation) with **zero** BeaconTail-2 packets received.
>
> Tail-2 absence is the normal operating condition for low-bandwidth or congested channels. Receivers MUST NOT require Tail-2 for any correctness guarantee.

---

## 6) Examples

### 6.1 BeaconTail-2 (7 B): minimum

| Field | Value | Bytes (hex) |
|-------|-------|-------------|
| payloadVersion | 0 | `00` |
| nodeId | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |

**Full hex (7 bytes):** `00 FF EE DD CC BB AA`

### 6.2 BeaconTail-2 (8 B): with maxSilence10s

| Field | Value | Bytes (hex) |
|-------|-------|-------------|
| payloadVersion | 0 | `00` |
| nodeId | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| maxSilence10s | 9 (= 90 s) | `09` |

**Full hex (8 bytes):** `00 FF EE DD CC BB AA 09`

### 6.3 BeaconTail-2 (12 B): operational with health

| Field | Value | Bytes (hex) |
|-------|-------|-------------|
| payloadVersion | 0 | `00` |
| nodeId | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| maxSilence10s | 6 (= 60 s) | `06` |
| batteryPercent | 85 | `55` |
| temp_x10 | 235 (= 23.5 °C) | `EB 00` |
| rssi_dbm | −85 dBm | `AB` |

**Full hex (12 bytes):** `00 FF EE DD CC BB AA 06 55 EB 00 AB`

### 6.4 BeaconTail-2 (20 B): full payload

| Field | Value | Bytes (hex) |
|-------|-------|-------------|
| payloadVersion | 0 | `00` |
| nodeId | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| maxSilence10s | 6 | `06` |
| batteryPercent | 72 | `48` |
| temp_x10 | −100 (= −10.0 °C) | `9C FF` |
| rssi_dbm | −90 dBm | `A6` |
| hwProfileId | 0x0001 | `01 00` |
| fwVersionId | 0x0042 | `42 00` |
| uptimeSec | 3600 | `10 0E 00 00` |

**Full hex (20 bytes):** `00 FF EE DD CC BB AA 06 48 9C FF A6 01 00 42 00 10 0E 00 00`

---

## 7) Versioning / compat

- **Unknown `payloadVersion`:** If `payloadVersion != 0x00`, receiver MUST discard the payload for NodeTable update purposes. Do not attempt partial decode.
- **v0.x:** Reserved for backward-compatible extensions (new optional fields at end). Until defined, only `0x00` is valid.
- **Layer separation:** `msg_type` (frame header) and `payloadVersion` (payload byte 0) are independent axes. See [ootb_radio_v0.md §3.3](../../../../protocols/ootb_radio_v0.md#33-layer-separation).

---

## 8) Related

- **Beacon encoding hub (Core/Tail overview):** [beacon_payload_encoding_v0.md](beacon_payload_encoding_v0.md) — §3 packet types table; §4.1 BeaconCore layout.
- **BeaconTail-1:** [tail1_packet_encoding_v0.md](tail1_packet_encoding_v0.md) — sample-attached Tail; CoreRef-lite via core_seq16.
- **Alive packet:** [alive_packet_encoding_v0.md](alive_packet_encoding_v0.md) — alive-bearing, non-position; no-fix liveness.
- **RX semantics (Tail-2 apply rules):** [../policy/rx_semantics_v0.md](../policy/rx_semantics_v0.md) — §2 BeaconTail-2 acceptance.
- **Field cadence (Tail-2 scheduling classes):** [../policy/field_cadence_v0.md](../policy/field_cadence_v0.md) — §2.2 Operational vs Informative; Tier C fields.
- **NodeID policy:** [../../../identity/nodeid_policy_v0.md](../../../identity/nodeid_policy_v0.md) — [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)
- **Packet header framing:** [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0) — [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)
- **Tail productization issue:** [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307)
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
