# NodeTable — BeaconTail-1 packet encoding (Contract v0)

**Status:** Canon (contract).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Tail productization:** [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307) · **Beacon encoding hub:** [beacon_payload_encoding_v0.md](beacon_payload_encoding_v0.md) · **NodeID policy:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) · **Packet header framing:** [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)

This contract defines the **v0 BeaconTail-1 payload**: byte layout, field semantics, units, ranges, and RX rules. BeaconTail-1 is an **optional, loss-tolerant** extension that qualifies one specific BeaconCore sample via `core_seq16`. It is **never required for correctness**; product MUST function correctly with zero Tail-1 packets received.

---

## 0) Frame header (framing layer — not part of this contract)

Every on-air BeaconTail-1 frame is preceded by a **2-byte frame header** defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0).

- **`msg_type` for BeaconTail-1:** `0x03` (`BEACON_TAIL1`). Dispatched independently at the framing layer.
- **`payload_len`** in the header = number of payload bytes (9 minimum; see §3 below). Does not include the 2-byte header.
- **`payloadVersion`** is the **first byte of the payload** (byte 0 of the payload body). It is not a header field.
- **On-air total size:** 2 (header) + 9 (min payload) = **11 bytes minimum**; 2 + 11 (with posFlags+sats) = **13 bytes**. Both fit within LongDist budget (24 B).

---

## 1) Role and invariants

- **BeaconTail-1 is sample-attached:** It qualifies exactly one BeaconCore sample, identified by `core_seq16`.
- **Optional and loss-tolerant:** Tail-1 packets may be missing, reordered, or dropped. The product MUST remain fully functional (position tracking, liveness) with zero Tail-1 packets received.
- **Never moves position:** Tail-1 MUST NOT update `lat`, `lon`, `alt`, or any position-derived field. Position is owned exclusively by BeaconCore.
- **Does not revoke Core position:** A Tail-1 with `posFlags = 0` (or any other value) MUST NOT clear or invalidate position already received in BeaconCore for that node. Position is overwritten only by a newer BeaconCore.

---

## 2) Packet type and naming

- **Message type name:** **BeaconTail-1** (or **Tail-1** in policy/cadence docs).
- **`msg_type` value:** `0x03` (`BEACON_TAIL1`) in the 2-byte frame header. See [ootb_radio_v0.md §3.2](../../../../protocols/ootb_radio_v0.md#32-msg_type-registry-v0) for the full registry.
- **Distinct packet family:** `msg_type = 0x03` is dispatched independently from BeaconCore (`0x01`), BeaconAlive (`0x02`), and BeaconTail-2 (`0x04`).

---

## 3) Byte layout (v0)

**Byte order:** Little-endian for all multi-byte integers. **Version tag:** First byte = `payloadVersion`; v0 = `0x00`. Unknown version → discard (same rule as BeaconCore). The 2-byte frame header precedes this payload; payload byte offsets below start at byte 0 of the payload body (after the header).

### 3.1 Minimum payload (MUST)

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 0 | `payloadVersion` | uint8 | 1 | `0x00` = v0. Determines the entire payload layout. Unknown value → discard. |
| 1 | `nodeId` | uint48 LE | 6 | NodeID48 (6-byte LE MAC48); same semantics as BeaconCore. See [nodeid_policy_v0](../../../identity/nodeid_policy_v0.md). |
| 7 | `core_seq16` | uint16 LE | 2 | `seq16` of the BeaconCore sample this Tail-1 qualifies. Little-endian. MUST match `last_core_seq16` for this node on RX (see §4). |

**Minimum size:** **9 bytes** (payload only). On-air with 2-byte header: **11 bytes**.

### 3.2 Optional: position quality fields (v0)

Appended in order after the minimum payload. Fields may be omitted from the end.

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 9 | `posFlags` | uint8 | 1 | Position-quality flags for this Core sample. `0x00` = not present / unknown. Bit definitions: bit 0 = position valid at TX time; bits 1–7 reserved. Does NOT revoke Core position (§1). See [position_quality_v0](../policy/position_quality_v0.md). |
| 10 | `sats` | uint8 | 1 | Satellite count at TX time; `0x00` = not present. |

With both optional fields: **11 bytes** payload; **13 bytes** on-air.

---

## 4) RX rules (normative)

### 4.1 Core linkage check (MUST)

On receiving a BeaconTail-1 for node `N`:

1. **Match check:** Compare `core_seq16` in the Tail-1 against `last_core_seq16` stored in NodeTable for node `N`.
2. **Apply on match:** If `core_seq16 == last_core_seq16[N]` → apply Tail-1 fields to the sample record for that node (update posFlags, sats as present).
3. **Ignore on mismatch:** If `core_seq16 != last_core_seq16[N]` (stale, reordered, or orphaned Tail-1) → **drop silently**. MUST NOT overwrite any NodeTable field.
4. **No node yet:** If no BeaconCore has ever been received from node `N` → drop silently (no `last_core_seq16` to match against).

### 4.2 Position invariant (MUST NOT)

Tail-1 MUST NOT update `lat`, `lon`, `alt`, or any position-derived field, regardless of `posFlags` value or any other content. Position is owned exclusively by BeaconCore.

### 4.3 Version and length guards

| Condition | Action |
|-----------|--------|
| `payloadVersion != 0x00` | Drop and log; do not attempt partial decode |
| `payload_len < 9` (minimum) | Drop packet |
| `payload_len` shorter than offset of a field being decoded | Treat field as absent (sentinel / not present) |

### 4.4 lastRxAt and link metrics

Even when Tail-1 payload is **not** applied (core_seq16 mismatch), the receiver SHOULD update `lastRxAt` and link metrics (rssiLast, snrLast) from the received frame. This keeps "last heard" accurate. See [rx_semantics_v0.md §3.2](../policy/rx_semantics_v0.md).

---

## 5) Loss-tolerance invariant

> The product MUST function correctly (position tracking, liveness, activity derivation) with **zero** BeaconTail-1 packets received.
>
> Tail-1 absence is the normal operating condition for low-bandwidth or congested channels. Receivers MUST NOT require Tail-1 for any correctness guarantee.

---

## 6) Examples

### 6.1 BeaconTail-1 (9 B): minimum

| Field | Value | Bytes (hex) |
|-------|-------|-------------|
| payloadVersion | 0 | `00` |
| nodeId | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| core_seq16 | 1 (matches last Core) | `01 00` |

**Full hex (9 bytes):** `00 FF EE DD CC BB AA 01 00`

### 6.2 BeaconTail-1 (11 B): with posFlags + sats

| Field | Value | Bytes (hex) |
|-------|-------|-------------|
| payloadVersion | 0 | `00` |
| nodeId | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| core_seq16 | 1 | `01 00` |
| posFlags | 0x01 (position valid) | `01` |
| sats | 8 | `08` |

**Full hex (11 bytes):** `00 FF EE DD CC BB AA 01 00 01 08`

### 6.3 RX rule: ignore on mismatch

Node `N` has `last_core_seq16 = 7`. Incoming Tail-1 carries `core_seq16 = 5`.
→ `5 ≠ 7` → drop silently. No NodeTable fields updated (except optionally lastRxAt and link metrics).

---

## 7) Versioning / compat

- **Unknown `payloadVersion`:** If `payloadVersion != 0x00`, receiver MUST discard the payload for NodeTable update purposes. Do not attempt partial decode.
- **v0.x:** Reserved for backward-compatible extensions (new optional fields at end). Until defined, only `0x00` is valid.
- **Layer separation:** `msg_type` (frame header) and `payloadVersion` (payload byte 0) are independent axes. See [ootb_radio_v0.md §3.3](../../../../protocols/ootb_radio_v0.md#33-layer-separation).

---

## 8) Related

- **Beacon encoding hub (Core/Tail overview):** [beacon_payload_encoding_v0.md](beacon_payload_encoding_v0.md) — §3 packet types table; §4.1 BeaconCore layout.
- **BeaconTail-2:** [tail2_packet_encoding_v0.md](tail2_packet_encoding_v0.md) — slow-state Tail; no CoreRef.
- **Alive packet:** [alive_packet_encoding_v0.md](alive_packet_encoding_v0.md) — alive-bearing, non-position; no-fix liveness.
- **RX semantics (CoreRef-lite rule):** [../policy/rx_semantics_v0.md](../policy/rx_semantics_v0.md) — §2 BeaconTail-1 apply rules; §4 no revocation of Core.
- **Field cadence (Tail-1 tier):** [../policy/field_cadence_v0.md](../policy/field_cadence_v0.md) — Tier B fields; degrade-under-load order.
- **Position quality flags:** [../policy/position_quality_v0.md](../policy/position_quality_v0.md) — posFlags bit definitions.
- **NodeID policy:** [../../../identity/nodeid_policy_v0.md](../../../identity/nodeid_policy_v0.md) — [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)
- **Packet header framing:** [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0) — [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)
- **Tail productization issue:** [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307)
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
