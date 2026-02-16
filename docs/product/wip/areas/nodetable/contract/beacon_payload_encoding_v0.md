# NodeTable — Beacon payload encoding (Contract v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173)

This contract defines the **v0 beacon payload**: byte layout, size budgets, and their coupling to RadioProfile classes (Default / LongDist / Fast). It implements the encoding deferred by [link-telemetry-minset-v0.md](link-telemetry-minset-v0.md). **Core/Tail split** (which fields are Core vs Tail) is driven by [field_cadence_v0](policy/field_cadence_v0.md) policy. Semantic truth is this contract and the minset; no reference to OOTB or UI as normative.

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

**Allowed in v0 beacon:** Fixed-width fields only; split by packet type (Core / Tail-1 / Tail-2) per §3. Optionality within a type is expressed by sentinel values (see byte layouts below).

---

## 3) Packet types (v0)

Policy and semantics for Core / Tail-1 / Tail-2 are in [field_cadence_v0.md](policy/field_cadence_v0.md) §2. This section defines the **v0 byte layouts** only.

| Type | Purpose | MUST fields | Optional fields | Notes |
|------|---------|-------------|-----------------|--------|
| **BeaconCore** | Minimal sample: WHO + WHERE + freshness. Strict cadence; MUST deliver. | version(1), nodeId(8), seq16(2), positionLat(4), positionLon(4) | — | Smallest possible; **19 B fixed**. Position may use sentinel 0x7FFFFFFF for "not present". |
| **BeaconTail-1** | Attached-to-Core extension; qualifies one Core sample. | version(1), nodeId(8), core_seq16(2) | posFlags(1), sats(1); then attached fields (hwProfileId, fwVersionId, uptimeSec, etc.) | Receiver applies only if **core_seq16 == lastCoreSeq**; else ignore. See [field_cadence_v0](policy/field_cadence_v0.md) §2. |
| **BeaconTail-2** | Uncoupled slow state; no CoreRef. | version(1), nodeId(8), maxSilence10s(1) | batteryPercent, hwProfileId, fwVersionId, uptimeSec (optional tail) | Minutes cadence; best-effort. maxSilence10s: uint8, 10s step, clamp ≤ 90 (15 min). Scheduling: **on change + at forced Core** (field_cadence §2). |

- **Byte order:** Little-endian for all multi-byte integers.
- **Version tag:** First byte = payload format version. v0 = `0x00`. Unknown version → discard (per §7).

---

## 4) Byte layouts by packet type

### 4.1 BeaconCore

**Strict order:** payloadVersion(1) | nodeId(8) | seq16(2) | positionLat(4) | positionLon(4).

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| payloadVersion | uint8 | 1 | 0x00 = v0 |
| nodeId | uint64 | 8 | DeviceId (e.g. ESP32-S3 MCU ID) |
| seq16 | uint16 | 2 | Freshness; monotonic per node. Little-endian. |
| positionLat | int32 | 4 | Latitude × 1e7 (WGS84); 0x7FFFFFFF = not present |
| positionLon | int32 | 4 | Longitude × 1e7 (WGS84); 0x7FFFFFFF = not present |

**Size:** **19 bytes** (fixed). Fits within LongDist (24), Default (32), Fast (40).

### 4.2 BeaconTail-1

**MUST (minimum):** payloadVersion(1) | nodeId(8) | core_seq16(2). **Optional (in order):** posFlags(1), sats(1); then other attached fields (order TBD; same sentinel conventions). Receiver **MUST** apply payload only if **core_seq16** equals the last Core seq16 received from that node; otherwise **MUST** ignore. See [field_cadence_v0.md](policy/field_cadence_v0.md) §2.

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| payloadVersion | uint8 | 1 | 0x00 = v0 |
| nodeId | uint64 | 8 | DeviceId |
| core_seq16 | uint16 | 2 | seq16 of the Core sample this Tail-1 qualifies. Little-endian. |
| posFlags | uint8 | 1 | Position-valid / quality flags; 0 = not present or no fix. Optional. |
| sats | uint8 | 1 | Satellite count when position valid; 0 = not present. Optional. |
| (optional attached fields) | — | TBD | hwProfileId, fwVersionId, uptimeSec, etc.; sentinel conventions as elsewhere. |

**Minimum size:** **11 bytes.** With posFlags+sats: 13 bytes.

### 4.3 BeaconTail-2

**MUST (minimal):** payloadVersion(1) | nodeId(8) | maxSilence10s(1). **Optional (in defined order):** batteryPercent(1), hwProfileId(2), fwVersionId(2), uptimeSec(4); sentinel values 0xFF, 0xFFFF, 0xFFFFFFFF = not present. Fields may be omitted from the end.

| Field | Type | Bytes | Encoding |
|-------|------|-------|----------|
| payloadVersion | uint8 | 1 | 0x00 = v0 |
| nodeId | uint64 | 8 | DeviceId |
| maxSilence10s | uint8 | 1 | Max silence in 10s steps; clamp ≤ 90 (15 min). 0 = not specified. |
| batteryPercent | uint8 | 1 | 0–100; 0xFF = not present |
| hwProfileId | uint16 | 2 | 0xFFFF = not present |
| fwVersionId | uint16 | 2 | 0xFFFF = not present |
| uptimeSec | uint32 | 4 | 0xFFFFFFFF = not present |

**Minimum size:** **10 bytes.** Maximum (all optional present): 1+8+1+1+2+2+4 = **19 bytes.**

---

## 5) Examples

### 5.1 BeaconCore (19 B): with position

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| seq16 | 1 | 01 00 |
| positionLat | 55.7558° × 1e7 = 557558000 | F0 A8 3B 21 |
| positionLon | 37.6173° × 1e7 = 376173000 | C8 F1 6B 16 |

**Full hex:** `00 EF CD AB 89 67 45 23 01 01 00 F0 A8 3B 21 C8 F1 6B 16`

### 5.2 BeaconCore (19 B): position not present

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| seq16 | 42 | 2A 00 |
| positionLat | not present (0x7FFFFFFF) | FF FF FF 7F |
| positionLon | not present (0x7FFFFFFF) | FF FF FF 7F |

**Full hex:** `00 EF CD AB 89 67 45 23 01 2A 00 FF FF FF 7F FF FF FF 7F`

### 5.3 BeaconTail-1 (13 B): core_seq16 + posFlags + sats

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| core_seq16 | 1 (matches last Core) | 01 00 |
| posFlags | 0x01 (position valid) | 01 |
| sats | 8 | 08 |

**Full hex:** `00 EF CD AB 89 67 45 23 01 01 00 01 08`

### 5.4 BeaconTail-2 (10 B): minimal

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| maxSilence10s | 9 (90 s) | 09 |

**Full hex:** `00 EF CD AB 89 67 45 23 01 09`

### 5.5 BeaconTail-2 (14 B): with battery

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| maxSilence10s | 6 (60 s) | 06 |
| batteryPercent | 85 | 55 |

**Full hex:** `00 EF CD AB 89 67 45 23 01 06 55`

---

## 6) Payload budgets per RadioProfile class

Payload size (in bytes) **MUST NOT** exceed the budget for the **RadioProfile class** in use for that beacon. Profile classes are defined in [registry_radio_profiles_v0.md](../../radio/registry_radio_profiles_v0.md) (Default / LongDist / Fast).

| Profile class | Max beacon payload (bytes) | Rationale |
|---------------|----------------------------|------------|
| **LongDist** | **24** | Maximize link margin; minimize ToA. Long range implies higher PER sensitivity to payload length; smallest budget. |
| **Default** | **32** | Balance range vs telemetry; reference size for v0. |
| **Fast** | **40** | Shorter range / higher rate; can tolerate more ToA for richer telemetry. Still bounded to avoid unbounded growth. |

**Rule:** If the chosen field set would exceed the budget for the current profile, the sender **MUST** either (a) omit optional fields until within budget, or (b) use a different packet type / cadence for the extra data. **MUST NOT** send a beacon over budget.

**Coupling to profile semantics:** LongDist = expected longer distance, lower density, less reliance on mesh; Fast = shorter distance or higher density/mesh; Default = middle. Budgets align: LongDist strictest, Fast most permissive.

---

## 7) Versioning / compat (v0 → v0.x)

- **Unknown version byte:** If the first byte is not a known payload version (e.g. not 0x00 for v0), the receiver **MUST** discard the payload for NodeTable update purposes (do not overwrite node-owned fields from that packet). Optionally log or pass to debug; no normative interpretation.
- **v0.x:** Reserved for backward-compatible extensions (e.g. new optional fields at end, or new version byte with same semantics for existing fields). Future doc will define v0.x if needed; until then, only 0x00 is valid.

---

## 8) Related

- **Field cadence (Core/Tail semantics):** [policy/field_cadence_v0.md](policy/field_cadence_v0.md) — §2 Beacon split definitions; receiver rule for Tail-1.
- **Minset (field semantics):** [link-telemetry-minset-v0.md](link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158)
- **RadioProfiles & ChannelPlan:** [../../radio/registry_radio_profiles_v0.md](../../radio/registry_radio_profiles_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
