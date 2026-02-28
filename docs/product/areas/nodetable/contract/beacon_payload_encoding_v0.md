# NodeTable — Beacon payload encoding (Contract v0)

**Status:** Canon (contract).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) · **NodeID policy:** [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) · **Geo encoding productization:** [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301) · **Packet header framing:** [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304) · **Tail productization:** [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307)

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

### Common prefix (all Node_* packets)

Every `Node_*` payload begins with the same **9-byte Common prefix**. See [ootb_radio_v0.md §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants) for the full anatomy and `seq16` invariants.

| Offset | Field | Bytes | Notes |
|--------|-------|-------|-------|
| 0 | `payloadVersion` | 1 | Version byte; determines entire payload layout for this `msg_type`. |
| 1–6 | `nodeId48` | 6 | NodeID48 (6-byte LE MAC48); sender identity. |
| 7–8 | `seq16` | 2 | Global per-node sequence counter (uint16 LE). One counter shared across all `Node_*` packet types. Dedupe key: `(nodeId48, seq16)`. |

After the Common prefix, each packet type has its own **Useful payload** (bytes 9+):

| Canonical alias | Useful payload (bytes 9+) | Notes |
|---|---|---|
| `Node_OOTB_Core_Pos` | `lat_u24(3)` + `lon_u24(3)` | `seq16` in Common is the freshness counter |
| `Node_OOTB_I_Am_Alive` | optional `aliveStatus(1)` | `seq16` in Common is the freshness counter |
| `Node_OOTB_Core_Tail` | `ref_core_seq16(2)` + optional `posFlags(1)` + optional `sats(1)` | `ref_core_seq16` in payload is a Core linkage key (back-reference to Core_Pos `seq16`); distinct from `seq16` in Common |
| `Node_OOTB_Operational` | optional `batteryPercent(1)`, `uptimeSec(4)` | Operational/dynamic runtime fields |
| `Node_OOTB_Informative` | optional `maxSilence10s(1)`, `hwProfileId(2)`, `fwVersionId(2)` | Static/config fields |

**`ref_core_seq16` vs `seq16`:** `Node_OOTB_Core_Tail` carries both: `seq16` in Common (its own unique packet id, increments the global counter) and `ref_core_seq16` in Useful payload (back-reference to the `seq16` of the specific `Node_OOTB_Core_Pos` sample it qualifies). These are distinct fields; `seq16 ≠ ref_core_seq16`. See [rx_semantics_v0.md §1](../policy/rx_semantics_v0.md) for duplicate/OOO handling.

| Canonical alias | Legacy enum | Purpose | MUST fields | Optional fields | Notes |
|---|---|---|---|---|---|
| **`Node_OOTB_Core_Pos`** | `BEACON_CORE` | Minimal sample: WHO + WHERE + freshness. Strict cadence; MUST deliver. | payloadVersion(1), nodeId48(6), seq16(2), lat_u24(3), lon_u24(3) | — | **15 B fixed** (+ 2B header = 17 B on-air). lat/lon MUST be valid; Core MUST NOT be transmitted without valid fix (§3.1). |
| **`Node_OOTB_Core_Tail`** | `BEACON_TAIL1` | Core-attached extension; qualifies one Core sample. Optional, loss-tolerant. | payloadVersion(1), nodeId48(6), seq16(2), ref_core_seq16(2) | posFlags(1), sats(1) | Apply only if `ref_core_seq16 == lastCoreSeq`; else ignore. MUST NOT update position. Full contract: [tail1_packet_encoding_v0.md](tail1_packet_encoding_v0.md). |
| **`Node_OOTB_Operational`** | `BEACON_TAIL2` | Operational slow state; dynamic runtime fields. Optional, loss-tolerant. | payloadVersion(1), nodeId48(6), seq16(2) | batteryPercent(1), uptimeSec(4) | No CoreRef; apply unconditionally. MUST NOT update position. Full contract: [tail2_packet_encoding_v0.md](tail2_packet_encoding_v0.md). |
| **`Node_OOTB_Informative`** | `BEACON_INFO` | Static/config fields; rarely changes. Optional, loss-tolerant. | payloadVersion(1), nodeId48(6), seq16(2) | maxSilence10s(1), hwProfileId(2), fwVersionId(2) | No CoreRef; apply unconditionally. MUST NOT update position. Full contract: [info_packet_encoding_v0.md](info_packet_encoding_v0.md). |

- **Byte order:** Little-endian for all multi-byte integers.
- **Frame header:** Every on-air frame is preceded by a 2-byte header (7+3+6 bit layout) defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0). See registry in [ootb_radio_v0.md §3.2](../../../../protocols/ootb_radio_v0.md#32-msg_type-registry-v0). The header is **not part of this contract**; payload byte offsets below start at byte 0 of the payload body.
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

### 4.2 Node_OOTB_Core_Tail (BeaconTail-1)

**Dedicated contract:** [tail1_packet_encoding_v0.md](tail1_packet_encoding_v0.md) — full byte layout, field definitions, units, ranges, and normative RX rules. Canonized in [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307).

**Summary:**
- `msg_type = 0x03` (`BEACON_TAIL1`), `payloadVersion = 0x00`
- **Minimum (MUST):** `payloadVersion(1) | nodeId48(6) | seq16(2) | ref_core_seq16(2)` = **11 bytes**
- **Optional:** `posFlags(1)`, `sats(1)` — up to **13 bytes**
- **`seq16`:** In Common prefix — the sender's own global counter value for this packet (unique packet id).
- **`ref_core_seq16`:** In Useful payload — Core linkage key: the `seq16` of the `Node_OOTB_Core_Pos` sample this Tail qualifies. A back-reference; `ref_core_seq16 ≠ seq16`.
- **RX rule (normative):** Apply payload **only if** `ref_core_seq16 == last_core_seq16[N]` for that node; otherwise **MUST** ignore. See [rx_semantics_v0](../policy/rx_semantics_v0.md) §2 and §4.
- **Invariants:** Core_Tail MUST NOT update position. Optional and loss-tolerant; product MUST function with zero packets received.

### 4.3 Node_OOTB_Operational (BeaconTail-2)

**Dedicated contract:** [tail2_packet_encoding_v0.md](tail2_packet_encoding_v0.md) — full byte layout, field definitions, sentinel values, and normative RX rules. Canonized in [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307).

**Summary:**
- `msg_type = 0x04` (`BEACON_TAIL2`), `payloadVersion = 0x00`
- **Minimum (MUST):** `payloadVersion(1) | nodeId48(6) | seq16(2)` = **9 bytes**
- **Useful payload (optional, in order):** `batteryPercent(1)`, `uptimeSec(4)` — up to **14 bytes**
- **Sentinel values:** `batteryPercent=0xFF`, `uptimeSec=0xFFFFFFFF` = not present
- **Semantic:** Operational/dynamic runtime fields (change during normal node operation). Cadence: on Operational field change + at forced Core.
- **RX rule:** No CoreRef; apply unconditionally when version and length valid. See [rx_semantics_v0](../policy/rx_semantics_v0.md) §2.
- **Invariants:** MUST NOT update position. Optional and loss-tolerant; product MUST function with zero packets received.

### 4.4 Node_OOTB_Informative (BeaconInfo)

**Dedicated contract:** [info_packet_encoding_v0.md](info_packet_encoding_v0.md) — full byte layout, field definitions, sentinel values, and normative RX rules.

**Summary:**
- `msg_type = 0x05` (`BEACON_INFO`), `payloadVersion = 0x00`
- **Minimum (MUST):** `payloadVersion(1) | nodeId48(6) | seq16(2)` = **9 bytes**
- **Useful payload (optional, in order):** `maxSilence10s(1)`, `hwProfileId(2)`, `fwVersionId(2)` — up to **14 bytes**
- **Sentinel values:** `hwProfileId=0xFFFF`, `fwVersionId=0xFFFF` = not present; `maxSilence10s=0x00` = absent/unknown
- **Semantic:** Static/config fields (rarely change; set at manufacture or on OTA). Cadence: on static/config field change + fixed slow cadence (default 10 min). MUST NOT be sent on every Operational send.
- **RX rule:** No CoreRef; apply unconditionally when version and length valid.
- **Invariants:** MUST NOT update position. Optional and loss-tolerant; product MUST function with zero packets received.

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

### 5.2 Node_OOTB_Core_Tail (13 B): seq16 + ref_core_seq16 + posFlags + sats

| Field | Section | Value | Bytes (hex) |
|-------|---------|--------|-------------|
| payloadVersion | Common | 0 | `00` |
| nodeId | Common | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | Common | 5 (own packet id) | `05 00` |
| ref_core_seq16 | Useful payload | 1 (matches last Core) | `01 00` |
| posFlags | Useful payload | 0x01 (position valid) | `01` |
| sats | Useful payload | 8 | `08` |

**Full hex (13 bytes):** `00 FF EE DD CC BB AA 05 00 01 00 01 08`

### 5.3 Node_OOTB_Operational (14 B): with batteryPercent + uptimeSec

| Field | Section | Value | Bytes (hex) |
|-------|---------|--------|-------------|
| payloadVersion | Common | 0 | `00` |
| nodeId | Common | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | Common | 7 | `07 00` |
| batteryPercent | Useful payload | 85 | `55` |
| uptimeSec | Useful payload | 3600 | `10 0E 00 00` |

**Full hex (14 bytes):** `00 FF EE DD CC BB AA 07 00 55 10 0E 00 00`

### 5.4 Node_OOTB_Informative (14 B): with maxSilence10s + hwProfileId + fwVersionId

| Field | Section | Value | Bytes (hex) |
|-------|---------|--------|-------------|
| payloadVersion | Common | 0 | `00` |
| nodeId | Common | 0x0000_AABB_CCDD_EEFF | `FF EE DD CC BB AA` |
| seq16 | Common | 8 | `08 00` |
| maxSilence10s | Useful payload | 9 (90 s) | `09` |
| hwProfileId | Useful payload | 0x0001 | `01 00` |
| fwVersionId | Useful payload | 0x0042 | `42 00` |

**Full hex (14 bytes):** `00 FF EE DD CC BB AA 08 00 09 01 00 42 00`

For full examples, see [tail2_packet_encoding_v0.md §6](tail2_packet_encoding_v0.md) and [info_packet_encoding_v0.md §6](info_packet_encoding_v0.md).

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

- **Node_OOTB_Core_Tail contract (full layout + RX rules):** [tail1_packet_encoding_v0.md](tail1_packet_encoding_v0.md) — [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307)
- **Node_OOTB_Operational contract (full layout + RX rules):** [tail2_packet_encoding_v0.md](tail2_packet_encoding_v0.md) — [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307)
- **Node_OOTB_Informative contract (full layout + RX rules):** [info_packet_encoding_v0.md](info_packet_encoding_v0.md) — [#314](https://github.com/AlexanderTsarkov/naviga-app/issues/314)
- **Alive packet (no-fix liveness):** [alive_packet_encoding_v0.md](alive_packet_encoding_v0.md) — Alive packet encoding; position-bearing vs alive-bearing in §3.1 above.
- **Packet anatomy & naming:** [ootb_radio_v0.md §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants) — Common prefix invariants, seq16 rules, canonical alias table.
- **Field cadence (Core/Tail semantics):** [../policy/field_cadence_v0.md](../policy/field_cadence_v0.md) — §2 Beacon split definitions; Operational vs Informative packets.
- **RX semantics (CoreRef-lite, Tail apply rules):** [../policy/rx_semantics_v0.md](../policy/rx_semantics_v0.md) — §2 Tail acceptance; §4 no revocation of Core.
- **Minset (field semantics):** [link-telemetry-minset-v0.md](link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158)
- **RadioProfiles & ChannelPlan:** [../../../radio/policy/registry_radio_profiles_v0.md](../../../radio/policy/registry_radio_profiles_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- **NodeID policy (wire format, source, ShortId):** [../../../identity/nodeid_policy_v0.md](../../../identity/nodeid_policy_v0.md) — [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298)
- **Geo encoding productization (packed24 decision):** [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301)
- **Packet header framing (2B 7+3+6, msg_type registry):** [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0) — [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)
- **Tail productization:** [#307](https://github.com/AlexanderTsarkov/naviga-app/issues/307)
