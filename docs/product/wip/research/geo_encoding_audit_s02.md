# Geo Encoding Audit — S02

**Status:** Research / WIP (not canon). **Superseded by decisions in [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301) and [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298).**
**Iteration:** S02__2026-03__docs_promotion_and_arch_audit
**Work Area:** Docs (audit)
**Date:** 2026-02-26
**Non-goal:** No code changes, no semantic changes, no mesh/JOIN changes.
**OOTB policy:** OOTB docs referenced only as examples/indicators; canon = contracts/policy/spec under `docs/product/areas/`.

> **⚠️ Decision of record:** The gaps and options described in this audit have been resolved. See:
> - [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) — NodeID48 (6-byte LE on-air); PR #299 (docs) and PR #300 (code) merged.
> - [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301) — BeaconCore packed24 v0: `payloadVersion(1) | nodeId48(6) | seq16(2) | lat_u24(3) | lon_u24(3)` = 15 bytes payload.
>
> The layouts described in §1.1–§1.3 below are **historical / as-implemented at audit time** and are **non-canon**. The current canon is in [`beacon_payload_encoding_v0.md §4.1`](../../areas/nodetable/contract/beacon_payload_encoding_v0.md).

---

## 1. As-Is Summary

### 1.1 On-air geo encoding — as-implemented at audit time (historical)

> **Note:** This section describes the codec at audit time (2026-02-26). The canon target is packed24 v0 per [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301).

The only implemented on-air geo encoding is in `firmware/protocol/geo_beacon_codec.cpp`. It uses **absolute WGS84 coordinates encoded as int32 × 1e7**, with no delta or sector compression.

**BeaconCore payload (19 bytes, fixed — historical):**

| Byte offset | Field | Type | Size | Encoding |
|-------------|-------|------|------|----------|
| 0 | `payloadVersion` | uint8 | 1 | `0x00` = v0 |
| 1–8 | `nodeId` | uint64 LE | 8 | Device ID |
| 9–10 | `seq` | uint16 LE | 2 | Freshness counter |
| 11–14 | `lat_e7` | int32 LE | 4 | Latitude × 1e7 (WGS84); range −900 000 000 .. +900 000 000 |
| 15–18 | `lon_e7` | int32 LE | 4 | Longitude × 1e7 (WGS84); range −1 800 000 000 .. +1 800 000 000 |

**Total: 19 bytes.** No `pos_valid` flag in the payload itself — the packet type (Core vs Alive) encodes validity implicitly (Core = valid fix, Alive = no fix).

> **Note on codec vs canon mismatch:** The current codec (`geo_beacon_codec.h/cpp`) uses a different layout than the canon contract (`beacon_payload_encoding_v0.md`). See §3 Gaps.

**Alive payload (11 bytes, fixed):**

| Byte offset | Field | Type | Size |
|-------------|-------|------|------|
| 0 | `payloadVersion` | uint8 | 1 |
| 1–8 | `nodeId` | uint64 LE | 8 |
| 9–10 | `seq` | uint16 LE | 2 |

No position fields. Alive = no-fix liveness signal only.

---

### 1.2 Canon contract geo encoding at audit time (`beacon_payload_encoding_v0.md`) — historical

The canon contract defines the same absolute WGS84 × 1e7 encoding but with a slightly different field order:

**BeaconCore (canon, 19 bytes):**

| Byte offset | Field | Type | Size | Encoding |
|-------------|-------|------|------|----------|
| 0 | `payloadVersion` | uint8 | 1 | `0x00` |
| 1–8 | `nodeId` | uint64 LE | 8 | |
| 9–10 | `seq16` | uint16 LE | 2 | |
| 11–14 | `positionLat` | int32 LE | 4 | Latitude × 1e7 (WGS84) |
| 15–18 | `positionLon` | int32 LE | 4 | Longitude × 1e7 (WGS84) |

**Encoding semantics (canon):**
- `positionLat` = latitude in degrees × 10^7, signed int32. Range: −90° to +90° → −900 000 000 to +900 000 000.
- `positionLon` = longitude in degrees × 10^7, signed int32. Range: −180° to +180° → −1 800 000 000 to +1 800 000 000.
- Precision: 1 unit = 0.0000001° ≈ 1.1 cm at equator. Sufficient for all V1-A use cases.
- Little-endian byte order for all multi-byte fields.
- BeaconCore MUST NOT be transmitted without a valid GNSS fix.

**Source:** `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md §4.1`

---

### 1.3 Firmware codec geo encoding (`geo_beacon_codec.cpp`) — **legacy / as-implemented at audit time**

> **Note:** This section describes the codec layout **at the time of the audit (2026-02-26)**. Since then: PR #300 reduced nodeId to 6 bytes (NodeID48); PR-B for #301 will replace this layout with packed24 v0. This section is retained for historical context only.

The firmware codec (`firmware/protocol/geo_beacon_codec.cpp`) encodes the same fields but with a **different byte layout**:

**Codec layout (24 bytes, legacy — as-implemented at audit time):**

| Byte offset | Field | Type | Size | Notes |
|-------------|-------|------|------|-------|
| 0 | `kGeoBeaconMsgType` = `0x01` | uint8 | 1 | Written as byte 0 |
| 1 | `kGeoBeaconVersion` = `1` | uint8 | 1 | Written as byte 1 |
| 2 | reserved / `0x00` | uint8 | 1 | Padding |
| 3–10 | `node_id` | uint64 LE | 8 | |
| 11 | `pos_valid` | uint8 | 1 | 0 = no fix, non-zero = valid |
| 12–15 | `lat_e7` | int32 LE | 4 | Latitude × 1e7 |
| 16–19 | `lon_e7` | int32 LE | 4 | Longitude × 1e7 |
| 20–21 | `pos_age_s` | uint16 LE | 2 | Position age in seconds |
| 22–23 | `seq` | uint16 LE | 2 | Sequence counter |

**Total: 24 bytes (`kGeoBeaconSize = 24`).** Constant `kGeoBeaconVersion = 1` (not `0x00`).

**Source:** `firmware/protocol/geo_beacon_codec.h` lines 9–16, 36–38; `firmware/protocol/geo_beacon_codec.cpp` lines 73–89.

---

### 1.4 NodeTable domain geo fields

The domain `NodeEntry` struct stores geo fields as received from the codec:

| Field | Type | Meaning |
|-------|------|---------|
| `pos_valid` | bool | Whether lat/lon are valid |
| `lat_e7` | int32_t | Latitude × 1e7 |
| `lon_e7` | int32_t | Longitude × 1e7 |
| `pos_age_s` | uint16_t | Age of position in seconds |

**Source:** `firmware/src/domain/node_table.h` lines 14–18.

These are populated by `beacon_logic.cpp:on_rx()` → `table.upsert_remote(node_id, pos_valid, lat_e7, lon_e7, pos_age_s, rssi_dbm, seq, now_ms)`.

---

### 1.5 BLE export geo fields

The 26-byte BLE NodeTableSnapshot record exports geo fields at fixed offsets:

| Offset | Field | Type | Size |
|--------|-------|------|------|
| 13–16 | `lat_e7` | uint32 LE (cast from int32) | 4 |
| 17–20 | `lon_e7` | uint32 LE (cast from int32) | 4 |
| 21–22 | `pos_age_s` | uint16 LE | 2 |

**Source:** `firmware/src/domain/node_table.cpp` `get_page()` lines 272–274.

---

### 1.6 Mesh concept geo encoding

The mesh concept (`naviga_mesh_protocol_concept_v1_4.md §5`) references `origin_pos` as a field in the geo packet, described as "position of origin in compressed format." No byte layout is defined — the concept explicitly defers encoding to a future spec.

Key quote (§5): `origin_pos — позиция origin в сжатом формате.`

The concept also mentions `bridge_hop_pos` (position of the current relay hop) as a separate field. Neither field has a defined byte layout in any current doc.

**Estimated geo packet size (concept):** ~40 bytes total including `origin_pos`, `covered_mask` (8 bytes), `origin_id`, `bridge_hop_id`, `bridge_hop_pos`, `seq`, `hop_count`, `TTL`. Source: `naviga_mesh_protocol_concept_v1_4.md §3.4` (line 197, 371).

---

## 2. Where Defined (file + section)

| Artefact | File | Section / line |
|----------|------|---------------|
| Canon geo encoding (lat/lon × 1e7, LE, int32) | `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | §4.1 BeaconCore table |
| Canon range constraints (lat/lon valid range) | `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | §4.1 row "positionLat" / "positionLon" |
| Canon: Core only with valid fix | `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | §3.1 "Position-bearing vs Alive-bearing" |
| Firmware codec struct (`GeoBeaconFields`) | `firmware/protocol/geo_beacon_codec.h` | Lines 9–16 |
| Firmware codec constants (`kGeoBeaconSize=24`, `kGeoBeaconVersion=1`) | `firmware/protocol/geo_beacon_codec.h` | Lines 36–38 |
| Firmware codec encode/decode | `firmware/protocol/geo_beacon_codec.cpp` | Lines 73–119 |
| Range validation in codec | `firmware/protocol/geo_beacon_codec.cpp` | Lines 8–11 (`kLatMin/Max`, `kLonMin/Max`); lines 111–116 |
| Domain `NodeEntry` geo fields | `firmware/src/domain/node_table.h` | Lines 14–18 |
| Domain upsert (pos_valid gating) | `firmware/src/domain/node_table.cpp` | Lines 166–169, 205–208 |
| BLE record geo offsets | `firmware/src/domain/node_table.cpp` | Lines 272–274 (`get_page`) |
| Mobile geo fields (`latE7`, `lonE7`, `posAgeS`) | `app/lib/features/connect/connect_controller.dart` | `NodeRecordV1` model; `BleNodeTableParser.parsePage` |
| Mesh concept `origin_pos` (compressed, TBD) | `docs/product/naviga_mesh_protocol_concept_v1_4.md` | §5 (line 583); §3.4 (size estimate ~40B) |
| OOTB-era geo field spec (TBD encoding) | `docs/protocols/ootb_radio_v0.md` | §3.2 GEO_BEACON "position (lat/lon fixed-point TBD)" |
| `pos_age_s` definition | `docs/firmware/ootb_node_table_v0.md` | §3 row "pos_age_s" (OOTB-era; non-canon) |

---

## 3. Gaps & Risks

| # | Gap / Risk | Severity | Details |
|---|-----------|----------|---------|
| G1 | **Codec layout diverges from canon contract** _(resolved — see note)_ | ~~High~~ **Closed** | _(At audit time)_ Canon (`beacon_payload_encoding_v0.md §4.1`) defined ~~19-byte~~ BeaconCore with layout: `payloadVersion(1) \| nodeId(8) \| seq16(2) \| positionLat(4) \| positionLon(4)`. Firmware codec used 24-byte layout with `lat_e7/lon_e7`. **Resolution:** #298 (nodeId48 6 B) + #301/#303 (packed24 3+3 B) + #304/#306 (2B header). Current canon: **15-byte payload** `payloadVersion(1) \| nodeId48(6) \| seq16(2) \| lat_u24(3) \| lon_u24(3)` + 2B header = **17 B on-air**. See [`beacon_payload_encoding_v0.md §4.1`](../../areas/nodetable/contract/beacon_payload_encoding_v0.md). |
| G2 | **`kGeoBeaconVersion = 1` vs canon `payloadVersion = 0x00`** | High | Codec uses version byte `1`; canon contract defines `payloadVersion = 0x00` for v0. These are different versioning layers (frame header `ver` vs payload `payloadVersion`) but the naming and values conflict. See also `_working/protocol_research/2026-02-25_on_air_framing_ootb_report.md §2.1` for full analysis. |
| G3 | **`pos_valid` byte in codec not in canon payload** | Medium | The codec encodes `pos_valid` as byte 11 of the 24-byte packet. The canon contract does NOT include a `pos_valid` field in the BeaconCore payload — instead, the Core/Alive packet type distinction carries the validity semantics. This is a structural difference: codec uses a single packet type with a validity flag; canon uses two packet types (Core = valid, Alive = no-fix). |
| G4 | **`pos_age_s` in codec not in canon BeaconCore** | Medium | The codec encodes `pos_age_s` (uint16, bytes 20–21). The canon BeaconCore does not include `pos_age_s` — it is a Tail-1/Tail-2 field in the canon model. The OOTB-era spec (`ootb_radio_v0.md §3.2`) lists `pos_age_s` as a "required core freshness indicator" (TBD). This field exists in the codec and domain but has no canon placement in BeaconCore. |
| G5 | **Mesh `origin_pos` encoding is undefined** | Medium | The mesh concept says `origin_pos` is in "compressed format" but provides no byte layout. No doc defines delta coding, sector encoding, or any compact geo scheme for mesh packets. The ~40-byte mesh packet estimate assumes some compression but the method is TBD. |
| G6 | **No canon doc for `pos_age_s` semantics** | Low | `pos_age_s` is used throughout firmware and BLE export but its canon definition is only in the OOTB-era `ootb_node_table_v0.md §3` (non-canon). The canon `beacon_payload_encoding_v0.md` does not define `pos_age_s` for BeaconCore (it appears only as an optional Tail field). |
| G7 | **`pos_valid` gating in domain is correct but undocumented** | Low | `node_table.cpp` correctly gates lat/lon storage on `pos_valid` (lines 166–169, 205–208): if `!pos_valid`, lat/lon stored as 0. This is correct behaviour but no canon policy doc explicitly states "receiver MUST zero lat/lon when pos_valid=false." |
| G8 | **No delta / compact encoding defined or implemented** | Low (V1-A) | The mesh concept requires compact geo encoding (~40B total packet). No delta coding, sector encoding, or fixed-point compression scheme is defined anywhere in docs or code. For V1-A OOTB (no mesh), this is not blocking; for mesh, it will need a spec. |

---

## 4. Candidate Options

> **Historical note (added [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304)):** Options B and C below referenced a "3-byte OOTB frame header (msg_type | ver | flags)". That 3-byte draft was never implemented and is superseded by the canonical **2-byte 7+3+6 header** defined in [ootb_radio_v0.md §3](../../../../protocols/ootb_radio_v0.md#3-radio-frame-format-v0). The `ver` and `flags` fields are retired. Options below are preserved as historical record; the chosen path was Option B (canon 15-byte BeaconCore packed24 v0, per #301/#303) with the 2-byte header (per #304).

### Option A: Align canon contract with firmware codec (docs-only)

Update `beacon_payload_encoding_v0.md` to match the actual 24-byte codec layout, adding `pos_valid` and `pos_age_s` to BeaconCore, and clarifying the frame header bytes (`msg_type`, `ver`, `reserved`).

**Pros:** Closes G1, G2, G3, G4 without code changes.
**Cons:** Changes the canon contract (requires review); the 24-byte layout is larger than the canon's 19-byte budget for LongDist profile (budget = 24B, so it just fits); adds `pos_valid` byte which the canon model intentionally avoids (Core = valid by definition).

### Option B: Align firmware codec with canon contract (code change)

Update the codec to implement the 19-byte canon layout: remove `msg_type`, `ver`, `reserved`, `pos_valid` bytes; move `pos_age_s` out of BeaconCore (into Tail-1 or Tail-2); use `payloadVersion = 0x00`.

**Pros:** Firmware matches canon; smaller packet (19B vs 24B); consistent with the Core/Alive type-based validity model.
**Cons:** Breaking change (all devices must update simultaneously); requires implementing the OOTB 3-byte frame header separately (per `ootb_radio_v0.md §3.1`) to preserve `msg_type` dispatch.

### Option C: Hybrid — add frame header, keep payload structure close to codec

Implement the 3-byte OOTB frame header (`msg_type=0x01 | ver=0x01 | flags=0x00`) prepended to the current codec payload, but remove the redundant `pos_valid` byte from the payload and rely on packet type (Core vs Alive) for validity. BeaconCore payload becomes: `payloadVersion(1) | nodeId(8) | seq16(2) | lat_e7(4) | lon_e7(4) | pos_age_s(2)` = 21 bytes + 3-byte header = 24 bytes total.

**Pros:** Aligns with OOTB frame spec; keeps `pos_age_s` in Core (useful for receivers); removes `pos_valid` redundancy; total size unchanged (24B).
**Cons:** Still diverges from canon 19-byte BeaconCore; requires code + doc changes.

### Option D: Status quo + document the divergence (docs-only, minimal)

Add a note to `beacon_payload_encoding_v0.md` or a new WIP doc explicitly stating: "Current firmware implements a 24-byte variant; the 19-byte canon layout is the target for a future migration." No code changes.

**Pros:** Zero risk; unblocks other work; honest about current state.
**Cons:** Leaves the divergence unresolved; risk of new code being written against either layout.

---

## 5. Recommendation for Next PRs

> **Status (updated):** Decisions have been made. See [#298](https://github.com/AlexanderTsarkov/naviga-app/issues/298) (NodeID48) and [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301) (packed24). The recommendations below are retained for historical context; they have been superseded.

### PR 1 (docs-only, recommended immediately)

**Scope:** Create a WIP doc `docs/product/wip/areas/nodetable/research/geo_encoding_delta_s02.md` (or similar) that:
1. Documents the current 24-byte codec layout as "as-implemented" (non-canon).
2. Documents the canon 19-byte BeaconCore layout as "target."
3. Explicitly lists the 5 field-level differences (G1–G4).
4. Recommends Option B or C as the resolution path, pending owner decision.
5. Notes that `pos_age_s` needs a canon placement decision (BeaconCore vs Tail-1).

**Files changed:** 1 new WIP doc. No code changes.

### PR 2 (docs-only, follow-up)

**Scope:** Update `beacon_payload_encoding_v0.md` to either:
- (if Option B chosen) Confirm the 19-byte canon layout is the target and add a migration note.
- (if Option C chosen) Update the layout table to reflect the hybrid 24-byte layout with frame header.

**Files changed:** `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md`.

### PR 3 (code, after PR 2 decision)

**Scope:** Update `firmware/protocol/geo_beacon_codec.cpp` to implement the chosen layout. Coordinate with all device firmware updates (breaking change).

---

## 6. File Reference Index

| File | Role | Key lines |
|------|------|-----------|
| `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | Canon: BeaconCore geo layout | §4.1 (19B layout table); §3.1 (Core only with valid fix) |
| `docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md` | Canon: Alive (no geo) | §3.1 (11B layout) |
| `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md` | Canon: positionLat/Lon rows | Rows "positionLat", "positionLon", "posFlags", "sats" |
| `docs/protocols/ootb_radio_v0.md` | OOTB-era frame spec | §3.2 GEO_BEACON v1 fields; "position (lat/lon fixed-point TBD)" |
| `docs/firmware/ootb_node_table_v0.md` | OOTB-era: pos_age_s | §3 row "pos_age_s" |
| `docs/product/naviga_mesh_protocol_concept_v1_4.md` | Concept: origin_pos (compressed, TBD) | §5 (line 583); §3.4 (size ~40B) |
| `firmware/protocol/geo_beacon_codec.h` | Impl: GeoBeaconFields struct, constants | Lines 9–16 (struct); 36–38 (kGeoBeaconSize=24, kGeoBeaconMsgType=0x01, kGeoBeaconVersion=1) |
| `firmware/protocol/geo_beacon_codec.cpp` | Impl: encode/decode | Lines 73–89 (encode); 91–119 (decode); 8–11 (range constants) |
| `firmware/src/domain/node_table.h` | Domain: NodeEntry geo fields | Lines 14–18 |
| `firmware/src/domain/node_table.cpp` | Domain: pos_valid gating, BLE export | Lines 166–169, 205–208 (upsert); 272–274 (get_page) |
| `firmware/src/domain/beacon_logic.cpp` | Domain: on_rx geo flow | Lines 80–108 |
| `firmware/src/app/m1_runtime.cpp` | Wiring: set_self_position | Lines 80–113 |
| `_working/protocol_research/2026-02-25_on_air_framing_ootb_report.md` | Research: frame header vs payload version analysis | §2.1 (ootb_radio_v0 spec); §3.1 (codec constants); §6.1 (gap table) |
