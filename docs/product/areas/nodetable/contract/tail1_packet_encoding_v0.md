# NodeTable — BeaconTail-1 packet encoding (Contract v0)

**Status:** Canon (contract).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Tail productization:** [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307) · **Beacon encoding hub:** [beacon_payload_encoding_v0.md](beacon_payload_encoding_v0.md) · **NodeID policy:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) · **Packet header framing:** [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)

This contract defines the **v0 BeaconTail-1 payload**: byte layout, field semantics, units, ranges, and RX rules. BeaconTail-1 is an **optional, loss-tolerant** extension that qualifies one specific BeaconCore sample via `ref_core_seq16`. It is **never required for correctness**; product MUST function correctly with zero Tail-1 packets received.

---

## 0) Frame header (framing layer — not part of this contract)

Every on-air `Node_OOTB_Core_Tail` frame is preceded by a **2-byte frame header** defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0).

- **`msg_type` for Node_OOTB_Core_Tail:** `0x03` (`BEACON_TAIL1`). Dispatched independently at the framing layer.
- **`payload_len`** in the header = number of payload bytes (11 minimum; see §3 below). Does not include the 2-byte header.
- **`payloadVersion`** is the **first byte of the payload** (byte 0 of the payload body). It is not a header field.
- **On-air total size:** 2 (header) + 11 (min payload) = **13 bytes minimum**; 2 + 13 (with posFlags+sats) = **15 bytes**. Both fit within LongDist budget (24 B).

---

## 0.1) Payload structure: Common prefix vs Useful payload

Every `Node_*` payload begins with the same **9-byte Common prefix** (see [ootb_radio_v0.md §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants)):

| Offset | Field | Bytes | Notes |
|--------|-------|-------|-------|
| 0 | `payloadVersion` | 1 | Version byte; determines entire payload layout. |
| 1–6 | `nodeId48` | 6 | NodeID48 (6-byte LE MAC48); sender identity. |
| 7–8 | `seq16` | 2 | Global per-node counter (uint16 LE). One counter shared across all `Node_*` packet types. Dedupe key: `(nodeId48, seq16)`. |

After the Common prefix, each packet type has its own **Useful payload** (bytes 9+):

| Canonical alias | Useful payload (bytes 9+) |
|---|---|
| `Node_OOTB_Core_Pos` | `lat_u24(3)` + `lon_u24(3)` |
| `Node_OOTB_I_Am_Alive` | optional `aliveStatus(1)` |
| **`Node_OOTB_Core_Tail`** | **`ref_core_seq16(2)`** (Core linkage key) + optional `posFlags(1)` + optional `sats(1)` |
| `Node_OOTB_Operational` | optional `batteryPercent(1)`, `uptimeSec(4)` |
| `Node_OOTB_Informative` | optional `maxSilence10s(1)`, `hwProfileId(2)`, `fwVersionId(2)` |

**Key distinction for `Node_OOTB_Core_Tail`:** This packet carries **two** seq fields:
- `seq16` in Common (bytes 7–8) — the sender's own global counter value for **this** packet; its unique packet id; used for dedupe.
- `ref_core_seq16` in Useful payload (bytes 9–10) — a **back-reference** to the `seq16` of the specific `Node_OOTB_Core_Pos` sample this Tail qualifies. This is a Core linkage key, not the sender's current counter. `seq16 ≠ ref_core_seq16`.

---

## 1) Role and invariants

- **`Node_OOTB_Core_Tail` is sample-attached:** It qualifies exactly one `Node_OOTB_Core_Pos` sample, identified by `ref_core_seq16`.
- **One tail per Core sample (TX invariant):** The sender MUST generate **at most one** `Node_OOTB_Core_Tail` per `Node_OOTB_Core_Pos` sample. Core_Tail is generated once, immediately after the corresponding Core_Pos, to capture `ref_core_seq16`. In the TX queue, a pending Core_Tail for a given `ref_core_seq16` MAY be replaced (e.g. if a newer Core_Pos is sent before the Tail is transmitted), but MUST NOT be duplicated. The receiver MUST treat any second `Node_OOTB_Core_Tail` referencing the same `ref_core_seq16` as an unexpected duplicate and ignore its payload (see [rx_semantics_v0.md §1](../policy/rx_semantics_v0.md)).
- **Optional and loss-tolerant:** `Node_OOTB_Core_Tail` packets may be missing, reordered, or dropped. The product MUST remain fully functional (position tracking, liveness) with zero packets received.
- **Never moves position:** MUST NOT update `lat`, `lon`, `alt`, or any position-derived field. Position is owned exclusively by `Node_OOTB_Core_Pos`.
- **Does not revoke Core position:** A Core_Tail with `posFlags = 0` (or any other value) MUST NOT clear or invalidate position already received in `Node_OOTB_Core_Pos` for that node. Position is overwritten only by a newer `Node_OOTB_Core_Pos`.

---

## 2) Packet type and naming

- **Canonical alias:** `Node_OOTB_Core_Tail`. Legacy wire enum: `BEACON_TAIL1`. See [ootb_radio_v0.md §3.0a](../../../../protocols/ootb_radio_v0.md#30a-canonical-packet-naming) for the full alias table.
- **`msg_type` value:** `0x03` (`BEACON_TAIL1`) in the 2-byte frame header. See [ootb_radio_v0.md §3.2](../../../../protocols/ootb_radio_v0.md#32-msg_type-registry-v0) for the full registry.
- **Distinct packet family:** `msg_type = 0x03` is dispatched independently from `Node_OOTB_Core_Pos` (`0x01`), `Node_OOTB_I_Am_Alive` (`0x02`), `Node_OOTB_Operational` (`0x04`), and `Node_OOTB_Informative` (`0x05`).

---

## 3) Byte layout (v0)

**Byte order:** Little-endian for all multi-byte integers. **Version tag:** First byte = `payloadVersion`; v0 = `0x00`. Unknown version → discard (same rule as BeaconCore). The 2-byte frame header precedes this payload; payload byte offsets below start at byte 0 of the payload body (after the header).

### 3.1 Minimum payload (MUST)

The first 9 bytes are the **Common prefix** shared by all `Node_*` packets. Bytes 9–10 are the Useful payload minimum for `Node_OOTB_Core_Tail`.

| Offset | Field | Type | Bytes | Section | Encoding |
|--------|-------|------|-------|---------|----------|
| 0 | `payloadVersion` | uint8 | 1 | Common | `0x00` = v0. Determines the entire payload layout. Unknown value → discard. |
| 1–6 | `nodeId` | uint48 LE | 6 | Common | NodeID48 (6-byte LE MAC48); same semantics as `Node_OOTB_Core_Pos`. See [nodeid_policy_v0](../../../identity/nodeid_policy_v0.md). |
| 7–8 | `seq16` | uint16 LE | 2 | Common | **Global per-node counter.** This packet's own unique id; increments the sender's global counter. Used for dedupe: `(nodeId48, seq16)`. |
| 9–10 | `ref_core_seq16` | uint16 LE | 2 | Useful payload | **Core linkage key:** the `seq16` of the `Node_OOTB_Core_Pos` sample this Tail qualifies. A back-reference; `ref_core_seq16 ≠ seq16`. MUST match `last_core_seq16` stored for this node on RX (see §4). |

**Minimum size:** **11 bytes** (Common + ref_core_seq16). On-air with 2-byte header: **13 bytes**.

### 3.2 Optional: position quality fields (v0)

Appended in order after the minimum payload. Fields may be omitted from the end.

| Offset | Field | Type | Bytes | Encoding |
|--------|-------|------|-------|----------|
| 11 | `posFlags` | uint8 | 1 | Position-quality flags for this Core sample. `0x00` = not present / unknown. Bit definitions: bit 0 = position valid at TX time; bits 1–7 reserved. Does NOT revoke Core position (§1). See [position_quality_v0](../policy/position_quality_v0.md). |
| 12 | `sats` | uint8 | 1 | Satellite count at TX time; `0x00` = not present. |

With both optional fields: **13 bytes** payload; **15 bytes** on-air.

---

## 4) RX rules (normative)

### 4.1 Core linkage check (MUST)

On receiving a BeaconTail-1 for node `N`:

1. **Match check:** Compare `ref_core_seq16` in the Tail-1 against `last_core_seq16` stored in NodeTable for node `N`.
2. **Apply on match:** If `ref_core_seq16 == last_core_seq16[N]` → apply Tail-1 fields to the sample record for that node (update posFlags, sats as present).
3. **Ignore on mismatch:** If `ref_core_seq16 != last_core_seq16[N]` (stale, reordered, or orphaned Tail-1) → **drop silently**. MUST NOT overwrite any NodeTable field.
4. **No node yet:** If no BeaconCore has ever been received from node `N` → drop silently (no `last_core_seq16` to match against).

### 4.2 Position invariant (MUST NOT)

Tail-1 MUST NOT update `lat`, `lon`, `alt`, or any position-derived field, regardless of `posFlags` value or any other content. Position is owned exclusively by BeaconCore.

### 4.3 Version and length guards

| Condition | Action |
|-----------|--------|
| `payloadVersion != 0x00` | Drop and log; do not attempt partial decode |
| `payload_len < 11` (minimum: Common + ref_core_seq16) | Drop packet |
| `payload_len` shorter than offset of a field being decoded | Treat field as absent (sentinel / not present) |

### 4.4 lastRxAt and link metrics

Even when Tail-1 payload is **not** applied (`ref_core_seq16` mismatch), the receiver SHOULD update `lastRxAt` and link metrics (rssiLast, snrLast) from the received frame. This keeps "last heard" accurate. See [rx_semantics_v0.md §3.2](../policy/rx_semantics_v0.md).

---

## 5) Loss-tolerance invariant

> The product MUST function correctly (position tracking, liveness, activity derivation) with **zero** `Node_OOTB_Core_Tail` packets received.
>
> Core_Tail absence is the normal operating condition for low-bandwidth or congested channels. Receivers MUST NOT require Core_Tail for any correctness guarantee.

---

## 6) Examples

### 6.1 Node_OOTB_Core_Tail (11 B): minimum

| Field | Section | Value | Bytes (hex) |
|-------|---------|-------|-------------|
| payloadVersion | Common | 0 | `00` |
| nodeId | Common | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | Common | 5 (own packet id) | `05 00` |
| ref_core_seq16 | Useful payload | 1 (matches last Core) | `01 00` |

**Full hex (11 bytes):** `00 FF EE DD CC BB AA 05 00 01 00`

### 6.2 Node_OOTB_Core_Tail (13 B): with posFlags + sats

| Field | Section | Value | Bytes (hex) |
|-------|---------|-------|-------------|
| payloadVersion | Common | 0 | `00` |
| nodeId | Common | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | Common | 5 (own packet id) | `05 00` |
| ref_core_seq16 | Useful payload | 1 | `01 00` |
| posFlags | Useful payload | 0x01 (position valid) | `01` |
| sats | Useful payload | 8 | `08` |

**Full hex (13 bytes):** `00 FF EE DD CC BB AA 05 00 01 00 01 08`

### 6.3 RX rule: ignore on mismatch

Node `N` has `last_core_seq16 = 7`. Incoming Core_Tail carries `ref_core_seq16 = 5`.
→ `5 ≠ 7` → drop silently. No NodeTable fields updated (except optionally lastRxAt and link metrics).

---

## 7) Versioning / compat

- **Unknown `payloadVersion`:** If `payloadVersion != 0x00`, receiver MUST discard the payload for NodeTable update purposes. Do not attempt partial decode.
- **v0.x:** Reserved for backward-compatible extensions (new optional fields at end). Until defined, only `0x00` is valid.
- **Layer separation:** `msg_type` (frame header) and `payloadVersion` (payload byte 0) are independent axes. See [ootb_radio_v0.md §3.3](../../../../protocols/ootb_radio_v0.md#33-layer-separation).

---

## 8) Related

- **Beacon encoding hub (Core/Tail overview):** [beacon_payload_encoding_v0.md](beacon_payload_encoding_v0.md) — §3 packet types table; §4.1 BeaconCore layout.
- **Node_OOTB_Operational:** [tail2_packet_encoding_v0.md](tail2_packet_encoding_v0.md) — Operational slow state; no CoreRef.
- **Node_OOTB_Informative:** [info_packet_encoding_v0.md](info_packet_encoding_v0.md) — Informative/static config; no CoreRef.
- **Alive packet:** [alive_packet_encoding_v0.md](alive_packet_encoding_v0.md) — alive-bearing, non-position; no-fix liveness.
- **RX semantics (CoreRef-lite rule):** [../policy/rx_semantics_v0.md](../policy/rx_semantics_v0.md) — §2 BeaconTail-1 apply rules; §4 no revocation of Core.
- **Field cadence (Tail-1 tier):** [../policy/field_cadence_v0.md](../policy/field_cadence_v0.md) — Tier B fields; degrade-under-load order.
- **Position quality flags:** [../policy/position_quality_v0.md](../policy/position_quality_v0.md) — posFlags bit definitions.
- **NodeID policy:** [../../../identity/nodeid_policy_v0.md](../../../identity/nodeid_policy_v0.md) — [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)
- **Packet header framing:** [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0) — [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)
- **Tail productization issue:** [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307)
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
