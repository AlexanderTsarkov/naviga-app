# NodeTable — Beacon payload encoding (Contract v0)

**Status:** Canon (contract).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) · **NodeID policy:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) · **Geo encoding productization:** [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301) · **Packet header framing:** [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)

This contract defines the **v0 beacon payload**: byte layout, size budgets, and their coupling to RadioProfile classes (Default / LongDist / Fast). It implements the encoding deferred by [link-telemetry-minset-v0.md](link-telemetry-minset-v0.md). **Core/Tail split** (which fields are Core vs Tail) is driven by [field_cadence_v0](../policy/field_cadence_v0.md) policy. Semantic truth is this contract and the minset; no reference to OOTB or UI as normative.

---

## 0) Frame header (framing layer — not part of this contract)

Every on-air frame is preceded by a **2-byte frame header** defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0). This contract defines **payload bytes only**; the header is handled at the framing layer.

**Key invariants:**

- `payload_len` in the header counts **payload bytes only** (not including the 2-byte header itself).
- `payloadVersion` is the **first byte of the payload** (byte 0 of the payload body). It is **not** a header field.
- `msg_type` in the header identifies the packet family (BEACON_CORE = `0x01`, BEACON_TAIL1 = `0x03`, BEACON_TAIL2 = `0x04`). See registry in [ootb_radio_v0.md §3.2](../../../../protocols/ootb_radio_v0.md#32-msg_type-registry-v0).
- The payload byte offsets in §4 below are **unchanged** by the presence of the frame header.

**On-air total size** = 2 (header) + payload bytes. Example: BeaconCore = 2 + 15 = **17 bytes on-air**.

---

## 1) Scope guardrails

- **In scope:** Byte layout for the **frequent beacon** carrying a subset of Link/Telemetry minset fields; max payload size per profile class; versioning/compat for v0.
- **Out of scope:** JOIN/session/master/mesh protocol; CAD/LBT (sense OFF + jitter-only default); fragmentation/chunking as a goal; backend stats, channel discovery.

---

## 2) Constraints and rationale

### 2.1 Time-on-air and reliability

- **Rationale:** Success probability over distance is driven by (1) radio profile (SF/BW/CR) and (2) **payload size → time-on-air (ToA) → bit-error exposure**. Longer payload → more symbols → higher probability that any bit is lost → packet invalid. So payload size must be **bounded per profile class** to keep range credible.
- **Implication:** v0 defines a **strict max payload size** per RadioProfile class. Implementations **MUST NOT** exceed the budget for that profile; if more data is needed, it **MUST** use another packet type or lower frequency, not grow the beacon.

### 2.2 v0 payload max size (global target)

- **v0 beacon payload MAX size:** **32 bytes** (octets) for the **Default** profile class. This is the normative reference; LongDist and Fast have **tighter or looser** budgets as defined in § Payload budgets per RadioProfile class.
- **Rationale:** 32 B keeps ToA moderate for typical SF7–SF9 spreads; allows identity + core telemetry (uptime, battery, hw id) without forcing fragmentation. Chosen to be achievable on all profile classes with the minimal field set.

### 2.3 Ban list (v0)

The following **MUST NOT** appear inside the v0 beacon payload:

- **Human-readable names** (networkName, localAlias, any string used for display).
- **Long or variable-length strings** (e.g. free-text fw version string). Only fixed-width identifiers (e.g. fwVersionId, hwProfileId) are allowed.
- **Rich objects** (nested structures, maps, repeated variable-length fields).
- **Per-packet mesh/routing state** (hops, via, forwarding tables); that belongs to a different packet type.

**Allowed in v0 beacon:** Fixed-width fields only; split by packet type (Core / Tail-1 / Tail-2) per §3. Optionality within a type is expressed by sentinel values where defined (Tail-2, optional Tail-1 fields); BeaconCore position has no sentinel — Core is transmitted only with valid fix (§3.1).

---

## 3) Packet types (v0)

Policy and semantics for Core / Tail-1 / Tail-2 are in [field_cadence_v0.md](../policy/field_cadence_v0.md) §2. This section defines the **v0 byte layouts** only.

| Type | Purpose | MUST fields | Optional fields | Notes |
|------|---------|-------------|-----------------|--------|
| **BeaconCore** | Minimal sample: WHO + WHERE + freshness. Strict cadence; MUST deliver. | payloadVersion(1), nodeId48(6), seq16(2), lat_u24(3), lon_u24(3) | — | Smallest possible; **15 B fixed** (+ 2B frame header = 17 B on-air). lat/lon MUST be valid coordinates; Core MUST NOT be transmitted without valid fix (§3.1). |
| **BeaconTail-1** | Attached-to-Core extension; qualifies one Core sample. | payloadVersion(1), nodeId(6), core_seq16(2) | posFlags(1), sats(1); then attached fields (hwProfileId, fwVersionId, uptimeSec, etc.) | Receiver applies only if **core_seq16 == lastCoreSeq**; else ignore. See [field_cadence_v0](../policy/field_cadence_v0.md) §2. |
| **BeaconTail-2** | Uncoupled slow state; no CoreRef. | payloadVersion(1), nodeId(6); operational send may omit maxSilence10s | maxSilence10s(1), batteryPercent, hwProfileId, fwVersionId, uptimeSec (optional tail) | Two scheduling classes: Operational (on change + at forced Core) and Informative (on change + default 10 min). See field_cadence §2.1. |

- **Byte order:** Little-endian for all multi-byte integers.
- **Frame header:** Every on-air frame is preceded by a 2-byte header (7+3+6 bit layout) defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0). `msg_type` values: BeaconCore = `0x01`, BeaconTail-1 = `0x03`, BeaconTail-2 = `0x04`. The header is **not part of this contract**; payload byte offsets below start at byte 0 of the payload body.
- **Layer separation:** `msg_type` lives in the **frame header** (separate from payload; not part of this contract). `payloadVersion` is the **first byte of the payload** and determines the **entire payload layout** for that `msg_type`. Unknown `payloadVersion` → discard (per §7).
- **Version tag:** `payloadVersion` = `0x00` for v0. This is a payload-layer version, not a header/frame version.

### 3.1 Position-bearing vs Alive-bearing

- **BeaconCore is position-bearing:** BeaconCore **MUST** only be transmitted when the sender has a valid GNSS fix. If a receiver obtains a BeaconCore, the lat/lon in that packet are valid; they are **not** revoked or invalidated by a later Tail-1 or any other packet.
- **maxSilence liveness when no fix:** The maxSilence liveness requirement (node MUST transmit at least one alive signal within the maxSilence window) **may** be satisfied by an **Alive packet** when the node has no valid fix and therefore does not send BeaconCore. Alive packet encoding is defined in [alive_packet_encoding_v0.md](alive_packet_encoding_v0.md). Alive is **alive-bearing, non-position-bearing**; it carries identity + seq16 (and optional aliveStatus) only.

---

## 4) Byte layouts by packet type

### 4.1 BeaconCore

**Decision:** [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301) — packed24 absolute coords, `payloadVersion=0x00`.

**Strict order:** payloadVersion(1) | nodeId48(6) | seq16(2) | lat_u24(3) | lon_u24(3).

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| payloadVersion | uint8 | 1 | `0x00` = v0. Determines the **entire payload layout** for this `msg_type`. Unknown value → discard. |
| nodeId48 | uint48 LE | 6 | NodeID48 (6-byte LE MAC48); see [nodeid_policy_v0](../../../identity/nodeid_policy_v0.md). |
| seq16 | uint16 LE | 2 | Freshness counter; monotonic per node (wrap-around). |
| lat_u24 | uint24 LE | 3 | Latitude packed24: `round((lat_deg + 90.0) / 180.0 × 16777215)`. Range [0, 16777215]. Valid coordinates only; Core MUST NOT be sent without valid fix — when no fix, send Alive packet (§3.1). |
| lon_u24 | uint24 LE | 3 | Longitude packed24: `round((lon_deg + 180.0) / 360.0 × 16777215)`. Range [0, 16777215]. Valid coordinates only; same transmit rule as lat_u24. |

**Encode (sender) — clamp rule:**
- Before encoding, clamp: `lat_deg ∈ [−90, +90]`, `lon_deg ∈ [−180, +180]`.
- If the input coordinates are invalid, NaN, or the node has no valid GNSS fix: **MUST NOT send BeaconCore**; send Alive packet instead (§3.1).

**Decode (receiver):**
- `lat_deg = lat_u24 / 16777215.0 × 180.0 − 90.0`
- `lon_deg = lon_u24 / 16777215.0 × 360.0 − 180.0`

**Precision:** ≈ 1.1 m (lat), ≈ 2.4 m (lon at equator). Sufficient for V1-A OOTB use cases (map display, proximity).

**`pos_valid` / `pos_age_s` are NOT part of BeaconCore.** Core is transmitted only when fix is valid (§3.1); the packet type itself encodes validity. Per-sample position quality (posFlags, sats) and age are carried by the optional Tail-1 packet (§4.2), which may be lost or omitted without breaking the product.

**Size:** **15 bytes** (payload only). With 2-byte frame header ([ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0)): **17 bytes on-air**. Fits within LongDist (24), Default (32), Fast (40).

### 4.2 BeaconTail-1

**MUST (minimum):** payloadVersion(1) | nodeId(6) | core_seq16(2). **Optional (in order):** posFlags(1), sats(1); then other attached fields (order TBD; same sentinel conventions). **posFlags** and **sats** are the canonical position-quality fields for Tail-1 (sample-attached); derivation of PositionQuality state is in [position_quality_v0.md](../policy/position_quality_v0.md). Receiver **MUST** apply payload only if **core_seq16** equals the last Core seq16 received from that node; otherwise **MUST** ignore. See [field_cadence_v0.md](../policy/field_cadence_v0.md) §2.

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| payloadVersion | uint8 | 1 | 0x00 = v0 |
| nodeId | uint48 | 6 | NodeID48 (6-byte LE MAC48); see [nodeid_policy_v0](../../../identity/nodeid_policy_v0.md). |
| core_seq16 | uint16 | 2 | seq16 of the Core sample this Tail-1 qualifies. Little-endian. |
| posFlags | uint8 | 1 | Position-quality flags for this sample; 0 = not present/unknown. Does not indicate no-fix; Tail-1 never revokes Core position. Optional. See [rx_semantics_v0](../policy/rx_semantics_v0.md) §4. |
| sats | uint8 | 1 | Satellite count when position valid; 0 = not present. Optional. |
| (optional attached fields) | — | TBD | hwProfileId, fwVersionId, uptimeSec, etc.; sentinel conventions as elsewhere. |

**Minimum size:** **9 bytes.** With posFlags+sats: 11 bytes.

### 4.3 BeaconTail-2

**MUST (minimal):** payloadVersion(1) | nodeId(6). **Optional (in defined order):** maxSilence10s(1), batteryPercent(1), hwProfileId(2), fwVersionId(2), uptimeSec(4); sentinel values 0xFF, 0xFFFF, 0xFFFFFFFF = not present. Fields may be omitted from the end.

**Normative:** **maxSilence10s** is **Tail-2 Informative**. It **MUST NOT** be included on every operational Tail-2 send unless its value changed. Operational Tail-2 sends (on change + at forced Core) may carry only version + nodeId + operational fields; informative fields are sent at informative cadence or on change (see [field_cadence_v0.md](../policy/field_cadence_v0.md) §2.1).

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| payloadVersion | uint8 | 1 | 0x00 = v0 |
| nodeId | uint48 | 6 | NodeID48 (6-byte LE MAC48); see [nodeid_policy_v0](../../../identity/nodeid_policy_v0.md). |
| maxSilence10s | uint8 | 1 | Informative. Max silence in 10s steps; clamp ≤ 90 (15 min). 0 = not specified. Omit unless changed or at informative cadence. |
| batteryPercent | uint8 | 1 | 0–100; 0xFF = not present |
| hwProfileId | uint16 | 2 | 0xFFFF = not present |
| fwVersionId | uint16 | 2 | 0xFFFF = not present |
| uptimeSec | uint32 | 4 | 0xFFFFFFFF = not present |

**Minimum size:** **7 bytes** (version + nodeId only). With maxSilence10s: **8 bytes.** Maximum (all optional present): 1+6+1+1+2+2+4 = **17 bytes.**

---

## 5) Examples

### 5.1 BeaconCore (15 B): with position (packed24)

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0x00 | `00` |
| nodeId48 | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | 1 | `01 00` |
| lat_u24 | 55.7558° → `round((55.7558+90)/180×16777215)` = 13585424 = 0xCF4C10 | `10 4C CF` |
| lon_u24 | 37.6173° → `round((37.6173+180)/360×16777215)` = 10141701 = 0x9AC005 | `05 C0 9A` |

**Full hex (15 bytes):** `00 FF EE DD CC BB AA 01 00 10 4C CF 05 C0 9A`

### 5.2 BeaconTail-1 (11 B): core_seq16 + posFlags + sats

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0000_AABB_CCDD_EEFF | FF EE DD CC BB AA |
| core_seq16 | 1 (matches last Core) | 01 00 |
| posFlags | 0x01 (position valid) | 01 |
| sats | 8 | 08 |

**Full hex:** `00 FF EE DD CC BB AA 01 00 01 08`

### 5.3 BeaconTail-2 (8 B): minimal

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0000_AABB_CCDD_EEFF | FF EE DD CC BB AA |
| maxSilence10s | 9 (90 s) | 09 |

**Full hex:** `00 FF EE DD CC BB AA 09`

### 5.4 BeaconTail-2 (9 B): with battery

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0000_AABB_CCDD_EEFF | FF EE DD CC BB AA |
| maxSilence10s | 6 (60 s) | 06 |
| batteryPercent | 85 | 55 |

**Full hex:** `00 FF EE DD CC BB AA 06 55`

---

## 6) Payload budgets per RadioProfile class

Payload size (in bytes) **MUST NOT** exceed the budget for the **RadioProfile class** in use for that beacon. Profile classes are defined in [registry_radio_profiles_v0.md](../../../radio/policy/registry_radio_profiles_v0.md) (Default / LongDist / Fast).

| Profile class | Max beacon payload (bytes) | Rationale |
|---------------|----------------------------|------------|
| **LongDist** | **24** | Maximize link margin; minimize ToA. Long range implies higher PER sensitivity to payload length; smallest budget. |
| **Default** | **32** | Balance range vs telemetry; reference size for v0. |
| **Fast** | **40** | Shorter range / higher rate; can tolerate more ToA for richer telemetry. Still bounded to avoid unbounded growth. |

**Rule:** If the chosen field set would exceed the budget for the current profile, the sender **MUST** either (a) omit optional fields until within budget, or (b) use a different packet type / cadence for the extra data. **MUST NOT** send a beacon over budget.

**Coupling to profile semantics:** LongDist = expected longer distance, lower density, less reliance on mesh; Fast = shorter distance or higher density/mesh; Default = middle. Budgets align: LongDist strictest, Fast most permissive.

---

## 7) Versioning / compat (v0 → v0.x)

- **Layer separation:** `msg_type` (in the 2-byte frame header; see [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0)) identifies the packet family (Core, Tail, Alive, …). `payloadVersion` (first byte of payload) determines the **entire payload layout** for that `msg_type`. These are independent versioning axes; do not conflate them.
- **Unknown `payloadVersion`:** If the first payload byte is not a known version for the given `msg_type` (e.g. not `0x00` for BeaconCore v0), the receiver **MUST** discard the payload for NodeTable update purposes (do not overwrite node-owned fields from that packet). Optionally log or pass to debug; no normative interpretation.
- **v0.x:** Reserved for backward-compatible extensions (e.g. new optional fields at end, or new version byte with same semantics for existing fields). Future doc will define v0.x if needed; until then, only `0x00` is valid.

---

## 8) Related

- **Alive packet (no-fix liveness):** [alive_packet_encoding_v0.md](alive_packet_encoding_v0.md) — Alive packet encoding; position-bearing vs alive-bearing in §3.1 above.
- **Field cadence (Core/Tail semantics):** [../policy/field_cadence_v0.md](../policy/field_cadence_v0.md) — §2 Beacon split definitions; receiver rule for Tail-1.
- **Minset (field semantics):** [link-telemetry-minset-v0.md](link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158)
- **RadioProfiles & ChannelPlan:** [../../../radio/policy/registry_radio_profiles_v0.md](../../../radio/policy/registry_radio_profiles_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- **NodeID policy (wire format, source, ShortId):** [../../../identity/nodeid_policy_v0.md](../../../identity/nodeid_policy_v0.md) — [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)
- **Geo encoding productization (packed24 decision):** [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301)
- **Packet header framing (2B 7+3+6, msg_type registry):** [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0) — [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)
