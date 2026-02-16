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

**Allowed in v0 beacon:** Fixed-width fields only: version, node_id, uptimeSec, batteryPercent, hwProfileId, fwVersionId, position (fixed encoding). Optionality is expressed by sentinel values (see byte layout).

---

## 3) Byte layout (v0)

- **Byte order:** Little-endian for multi-byte integers.
- **Version tag:** First byte of payload = **payload format version**. v0 = `0x00`. Receivers **MUST** interpret unknown version per § Versioning/compat.

### 3.1 Field table

| Field | Type | Bytes | Encoding / scaling | Optionality |
|-------|------|-------|--------------------|-------------|
| **payloadVersion** | uint8 | 1 | 0x00 = v0 | MUST |
| **nodeId** | uint64 | 8 | DeviceId (e.g. ESP32-S3 MCU ID) | MUST |
| **uptimeSec** | uint32 | 4 | Seconds since boot; 0xFFFFFFFF = not present | Optional |
| **batteryPercent** | uint8 | 1 | 0–100; 0xFF = not present | Optional |
| **hwProfileId** | uint16 | 2 | Opaque id from HW registry ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)); 0xFFFF = not present | Optional |
| **fwVersionId** | uint16 | 2 | Opaque id; mapping to display string out of scope; 0xFFFF = not present | Optional |
| **positionLat** | int32 | 4 | Latitude × 1e7 (WGS84); 0x7FFFFFFF = not present | Optional |
| **positionLon** | int32 | 4 | Longitude × 1e7 (WGS84); 0x7FFFFFFF = not present | Optional |

**Field order:** Strict. Fields appear in the table order. Optional fields **MAY** be omitted from the end of the payload to save bytes (no trailing 0xFF/0xFFFF/0x7FFFFFFF required if field is omitted). If a field is present, all preceding optional fields are either present or encoded with their sentinel value.

**Omission rule:** Implementations **MAY** truncate the payload after the last field they need. Receiver: absent bytes = field not present; do not overwrite stored value (per minset “missing values do not erase”).

### 3.2 Minimum and maximum encoded size

- **Minimum (v0):** 9 bytes (payloadVersion + nodeId only).
- **Maximum (v0, all fields present):** 1+8+4+1+2+2+4+4 = **26 bytes**. Fits within all profile-class budgets below.

---

## 4) Examples

### 4.1 Minimal (9 bytes): version + node_id only

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |

**Full hex:** `00 EF CD AB 89 67 45 23 01`

### 4.2 With uptime and battery (14 bytes)

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| uptimeSec | 300 | E4 01 00 00 |
| batteryPercent | 85 | 55 |

**Full hex:** `00 EF CD AB 89 67 45 23 01 E4 01 00 00 55`

### 4.3 With hwProfileId and fwVersionId (18 bytes)

| Field | Value | Bytes (hex) |
|-------|--------|-------------|
| payloadVersion | 0 | 00 |
| nodeId | 0x0123_4567_89AB_CDEF | EF CD AB 89 67 45 23 01 |
| uptimeSec | 300 | E4 01 00 00 |
| batteryPercent | 85 | 55 |
| hwProfileId | 1 | 01 00 |
| fwVersionId | 2 | 02 00 |

**Full hex:** `00 EF CD AB 89 67 45 23 01 E4 01 00 00 55 01 00 02 00`

---

## 5) Payload budgets per RadioProfile class

Payload size (in bytes) **MUST NOT** exceed the budget for the **RadioProfile class** in use for that beacon. Profile classes are defined in [registry_radio_profiles_v0.md](../../radio/registry_radio_profiles_v0.md) (Default / LongDist / Fast).

| Profile class | Max beacon payload (bytes) | Rationale |
|---------------|----------------------------|------------|
| **LongDist** | **24** | Maximize link margin; minimize ToA. Long range implies higher PER sensitivity to payload length; smallest budget. |
| **Default** | **32** | Balance range vs telemetry; reference size for v0. |
| **Fast** | **40** | Shorter range / higher rate; can tolerate more ToA for richer telemetry. Still bounded to avoid unbounded growth. |

**Rule:** If the chosen field set would exceed the budget for the current profile, the sender **MUST** either (a) omit optional fields until within budget, or (b) use a different packet type / cadence for the extra data. **MUST NOT** send a beacon over budget.

**Coupling to profile semantics:** LongDist = expected longer distance, lower density, less reliance on mesh; Fast = shorter distance or higher density/mesh; Default = middle. Budgets align: LongDist strictest, Fast most permissive.

---

## 6) Versioning / compat (v0 → v0.x)

- **Unknown version byte:** If the first byte is not a known payload version (e.g. not 0x00 for v0), the receiver **MUST** discard the payload for NodeTable update purposes (do not overwrite node-owned fields from that packet). Optionally log or pass to debug; no normative interpretation.
- **v0.x:** Reserved for backward-compatible extensions (e.g. new optional fields at end, or new version byte with same semantics for existing fields). Future doc will define v0.x if needed; until then, only 0x00 is valid.

---

## 7) Related

- **Minset (field semantics):** [link-telemetry-minset-v0.md](link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158)
- **RadioProfiles & ChannelPlan:** [../../radio/registry_radio_profiles_v0.md](../../radio/registry_radio_profiles_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
