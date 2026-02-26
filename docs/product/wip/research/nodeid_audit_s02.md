# NodeID Audit — S02

**Status:** Research / WIP (not canon).
**Iteration:** S02__2026-03__docs_promotion_and_arch_audit
**Work Area:** Docs (audit)
**Date:** 2026-02-26
**Non-goal:** No code changes, no semantic changes, no mesh/JOIN changes.
**OOTB policy:** OOTB docs referenced only as examples/indicators; canon = contracts/policy/spec under `docs/product/areas/`.

---

## 1. As-Is Summary

### 1.1 Concept map

| Concept | Name in docs | Name in code | Size | Where defined |
|---------|-------------|-------------|------|---------------|
| Primary node key | `nodeId` / `node_id` | `node_id` (uint64_t) | 8 bytes | `beacon_payload_encoding_v0.md §4.1`; `alive_packet_encoding_v0.md §3.1`; `firmware/src/domain/node_table.h` |
| Display / short key | `ShortId` / `short_id` | `short_id` (uint16_t) | 2 bytes | `nodetable_fields_inventory_v0.md` row "ShortId"; `firmware/src/domain/node_table.h` |
| Mesh session key | `short_id` (0..63) | — (not implemented) | 6 bits | `naviga_mesh_protocol_concept_v1_4.md §3.2` |
| Hardware source | ESP32 eFuse MAC | `esp_efuse_mac_get_default` | 6 bytes raw | `firmware/src/platform/esp_device_id_provider.cpp` |
| Wire encoding of nodeId | uint64 LE | `write_u64_le` | 8 bytes | `firmware/protocol/geo_beacon_codec.cpp`; `firmware/src/domain/node_table.cpp` |

---

### 1.2 NodeID formation chain (firmware)

```
esp_efuse_mac_get_default(base_mac[6])          // esp_device_id_provider.cpp:16
  → DeviceId{bytes[6]}                           // naviga/platform/device_id.h
  → full_id_from_mac(device_id.bytes)            // platform/device_id.cpp:68
      id = 0x0000<mac[0]..mac[5]>               // big-endian packing into lower 6 bytes
      → uint64_t (upper 2 bytes = 0x0000)
  → domain::NodeTable::compute_short_id(full_id) // domain/node_table.cpp:62
      → CRC16-CCITT over 8 bytes (LE of full_id)
```

**Wiring entry point:** `firmware/src/app/app_services.cpp` line 147–151.

---

### 1.3 Where nodeId appears as a key

| Role | Location | Notes |
|------|----------|-------|
| NodeTable primary key (lookup) | `firmware/src/domain/node_table.cpp` `find_entry_index()` | `node_id == entry.node_id` |
| NodeTable primary key (init) | `node_table.cpp` `init_self()`, `upsert_remote()` | Stored in `NodeEntry.node_id` |
| On-air beacon (Core/Tail-1/Tail-2/Alive) | `firmware/protocol/geo_beacon_codec.cpp` | Bytes 1–8 of payload (after payloadVersion byte) in current codec. _(Historical note: earlier audit referenced "bytes 3–10 after msg_type+ver+flags 3-byte header"; that 3-byte header was never implemented and is superseded by the 2-byte 7+3+6 header per [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304). With the 2-byte header: payload bytes 1–8 remain at the same payload-relative offsets.)_ |
| BLE NodeTableSnapshot record | `firmware/src/domain/node_table.cpp` `get_page()` offset 0 | 8 bytes LE |
| Mobile model | `app/lib/features/connect/connect_controller.dart` `NodeRecordV1.nodeId` | Parsed from BLE record |
| Dedup / seq ordering key | `node_table.cpp` `upsert_remote()` seq16_order | Per-node seq16 counter keyed by node_id |
| Mesh dedup key | `naviga_mesh_protocol_concept_v1_4.md §5` | `(origin_id, seq)` pair; `origin_id` = mesh `short_id` (0..63), not uint64 |

---

### 1.4 short_id derivation — two implementations

| Location | Input | Algorithm | Output |
|----------|-------|-----------|--------|
| `firmware/src/domain/node_table.cpp` `compute_short_id()` line 62 | `uint64_t node_id` serialised as 8 bytes **LE** | CRC16-CCITT-FALSE (`init=0xFFFF`, `poly=0x1021`) | `uint16_t` |
| `firmware/src/platform/device_id.cpp` `short_id_from_mac()` line 81 | `uint8_t mac[6]` (6 bytes, MAC order) | CRC16-CCITT-FALSE (`init=0xFFFF`, `poly=0x1021`) | `uint16_t` |

**Gap:** These two functions operate on different inputs (8-byte LE of uint64 vs 6-byte MAC). Because `full_id_from_mac` packs MAC bytes big-endian into the lower 6 bytes of a uint64, and `compute_short_id` serialises that uint64 back as 8 bytes LE, the byte sequences fed to CRC differ:

- `short_id_from_mac(mac[6])` → CRC16 over `[mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]]`
- `compute_short_id(full_id_from_mac(mac))` → CRC16 over `[mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], 0x00, 0x00]`

These produce **different CRC16 values** for the same device. In practice `app_services.cpp` uses `compute_short_id(full_id)` (line 151), so the domain path is consistent; `short_id_from_mac` in `device_id.cpp` is a standalone utility that is not called from the main wiring path.

---

### 1.5 Mesh short_id (session key) vs NodeTable short_id (display key)

| | NodeTable `short_id` | Mesh `short_id` |
|--|---------------------|-----------------|
| Range | uint16 (0..65535) | 0..63 (6 bits) |
| Purpose | Display / UI label; collision detection | `covered_mask` bit index; `origin_id`; `bridge_hop_id` in mesh packet |
| Assignment | Derived: CRC16 of node_id | Session-assigned (mechanism TBD per mesh concept) |
| Canon source | `nodetable_fields_inventory_v0.md` row "ShortId"; `ootb_node_table_v0.md §1a` | `naviga_mesh_protocol_concept_v1_4.md §3.2` |
| Implemented? | Yes | No (mesh not implemented) |

These are **two distinct concepts sharing the same name** — a naming collision risk.

---

## 2. Where Defined (file + section)

| Artefact | File | Section / line |
|----------|------|---------------|
| `nodeId` on-air layout (uint64 LE, bytes 1–8 of payload) | `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | §4.1 BeaconCore; §4.2 Tail-1; §4.3 Tail-2 |
| `nodeId` in Alive packet | `docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md` | §3.1 Minimum payload |
| `nodeId` semantics (DeviceId, primary key) | `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | §4.1 row "nodeId" |
| `ShortId` definition (derived, display only) | `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md` | Row "ShortId" |
| `short_id` collision handling | `docs/firmware/ootb_node_table_v0.md` | §1a (OOTB-era, non-canon; still the most detailed collision spec) |
| NodeTable primary key semantics | `docs/product/areas/nodetable/index.md` | §3 Contracts |
| Hardware source (eFuse MAC) | `firmware/src/platform/esp_device_id_provider.cpp` | Line 16 (`esp_efuse_mac_get_default`) |
| `IDeviceIdProvider` interface | `firmware/lib/NavigaCore/include/naviga/platform/device_id.h` | `struct DeviceId{bytes[6]}`; `struct IDeviceIdProvider` |
| `full_id_from_mac` (MAC → uint64) | `firmware/src/platform/device_id.cpp` | Lines 68–79 |
| `compute_short_id` (uint64 → CRC16) | `firmware/src/domain/node_table.cpp` | Lines 62–66 |
| `short_id_from_mac` (MAC → CRC16, standalone) | `firmware/src/platform/device_id.cpp` | Lines 81–87 |
| Mesh `short_id` (0..63, session key) | `docs/product/naviga_mesh_protocol_concept_v1_4.md` | §3.2 (short_id ∈ [0..63]); §5 (origin_id, bridge_hop_id) |
| seq16 scope (single per-node counter) | `docs/product/areas/nodetable/index.md` | §2 V1-A invariants |
| seq16 dedup / ordering | `firmware/src/domain/node_table.cpp` | Lines 17–23 (`seq16_order`) |

---

## 3. Gaps & Risks

| # | Gap / Risk | Severity | Details |
|---|-----------|----------|---------|
| G1 | **Two `short_id` concepts share the same name** | Medium | NodeTable `short_id` (uint16, CRC16 display key) vs Mesh `short_id` (0..63, session bit-index). Mesh concept uses `short_id` as `origin_id` and in `covered_mask`; NodeTable uses it for UI only. When mesh is implemented, the naming collision will cause confusion. No canon doc disambiguates them. |
| G2 | **`short_id_from_mac` and `compute_short_id` produce different values** | Low (currently) | `device_id.cpp:short_id_from_mac` takes 6 MAC bytes; `node_table.cpp:compute_short_id` takes 8-byte LE of uint64. The main wiring path (`app_services.cpp`) uses `compute_short_id`, so runtime is consistent. But `short_id_from_mac` exists as a public API and could be called by mistake, producing a different short_id for the same device. |
| G3 | **No canon doc for `short_id` collision handling** | Low | `ootb_node_table_v0.md §1a` is the most detailed collision spec (reserved values 0x0000/0xFFFF, disambiguation display). This is an OOTB-era doc, not promoted to canon. `nodetable_fields_inventory_v0.md` row "ShortId" only says "may collide". |
| G4 | **NodeID source (eFuse MAC) not documented in canon** | Low | Canon contracts say `nodeId = DeviceId (e.g. ESP32-S3 MCU ID)` but do not specify the exact hardware source. The eFuse MAC path is only in firmware code. No canon policy doc covers factory-reset behaviour, MAC type selection order (`ESP_MAC_BLE` → `ESP_MAC_BT` → `esp_efuse_mac_get_default`). |
| G5 | **Mesh `short_id` assignment mechanism is TBD** | Medium | `naviga_mesh_protocol_concept_v1_4.md §3.2` says "Mechanism for assigning short_id is part of session management and is considered separately from the mesh protocol." No doc defines this mechanism. |
| G6 | **Upper 2 bytes of nodeId are always 0x0000** | Low | `full_id_from_mac` packs 6 MAC bytes into the lower 6 bytes of uint64, leaving upper 2 bytes zero. This is not documented as a constraint in any canon doc. Future ID sources (e.g. non-MAC) could use the full 64 bits; the current assumption is implicit. |
| G7 | **`long_id` term appears in OOTB-era docs but not in canon** | Low | `ootb_node_table_v0.md §1` uses "64-bit integer" without naming it `long_id`. Some older research notes use `long_id`. Canon contracts use `nodeId`. No explicit mapping doc. |

---

## 4. Candidate Options

### Option A: Status quo + disambiguation doc (docs-only)

Add a short canon policy doc (e.g. `docs/product/areas/identity/nodeid_policy_v0.md`) that:
- Defines `nodeId` = uint64, source = eFuse MAC via `full_id_from_mac`, upper 2 bytes reserved = 0x0000.
- Defines `ShortId` = CRC16-CCITT-FALSE over 8-byte LE of nodeId; display only; not a protocol key.
- Explicitly names the two `short_id` concepts and disambiguates: "NodeTable ShortId" vs "Mesh SessionId (0..63)".
- Documents collision handling (reserved values, display format).

**Pros:** No code change; resolves G1, G3, G4, G7.
**Cons:** Does not fix G2 (two functions producing different values).

### Option B: Option A + deprecate `short_id_from_mac`

Same as A, plus mark `firmware/src/platform/device_id.cpp:short_id_from_mac` as deprecated (comment or remove), since the canonical path is `compute_short_id(full_id_from_mac(mac))`.

**Pros:** Closes G2; reduces confusion.
**Cons:** Small code change (comment or removal); must verify no callers outside main path.

### Option C: Rename mesh session key in concept doc

In `naviga_mesh_protocol_concept_v1_4.md`, rename the 0..63 session identifier to `session_id` or `mesh_slot_id` to avoid collision with NodeTable `short_id`.

**Pros:** Eliminates G1 at the concept level before mesh is implemented.
**Cons:** Mesh concept is a concept doc (not canon); renaming now may diverge from future mesh spec.

---

## 5. Recommendation for Next PRs

### PR 1 (docs-only, recommended immediately)

**Scope:** Create `docs/product/areas/identity/nodeid_policy_v0.md` (or equivalent) covering:
1. `nodeId` = uint64; source = eFuse MAC (`esp_efuse_mac_get_default`); packing = `full_id_from_mac` (big-endian MAC bytes into lower 6 bytes of uint64, upper 2 bytes = 0x0000).
2. `ShortId` = CRC16-CCITT-FALSE over 8-byte LE of nodeId; display only; MUST NOT be used as protocol key.
3. Disambiguation: "NodeTable ShortId (uint16)" vs "Mesh Session Slot (0..63, TBD)".
4. Collision handling: reserved values (0x0000, 0xFFFF), display format, `short_id_collision` flag.
5. Factory-reset policy: nodeId does not change on reset (per `ootb_node_table_v0.md §1`, promote to canon).

**Files changed:** 1 new doc. No code changes.

### PR 2 (optional, code-only, low priority)

**Scope:** Deprecate or remove `short_id_from_mac` in `firmware/src/platform/device_id.cpp` (or add a comment explaining it produces a different value than `compute_short_id`). Verify no callers outside test files.

**Files changed:** `firmware/src/platform/device_id.cpp`, `firmware/src/platform/device_id.h`, possibly test files.

---

## 6. File Reference Index

| File | Role | Key lines |
|------|------|-----------|
| `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | Canon: nodeId on-air layout | §4.1 (Core), §4.2 (Tail-1), §4.3 (Tail-2) |
| `docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md` | Canon: nodeId in Alive | §3.1 |
| `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md` | Canon: ShortId row | Row "ShortId" |
| `docs/product/areas/nodetable/index.md` | Canon hub | §2 (seq16 scope), §3 (contracts) |
| `docs/firmware/ootb_node_table_v0.md` | OOTB-era (non-canon): collision spec | §1 (NodeID source), §1a (ShortId, collision) |
| `docs/product/naviga_mesh_protocol_concept_v1_4.md` | Concept: mesh short_id | §3.2 (0..63 range, covered_mask), §5 (origin_id) |
| `firmware/lib/NavigaCore/include/naviga/platform/device_id.h` | Interface: DeviceId, IDeviceIdProvider | struct definitions |
| `firmware/src/platform/esp_device_id_provider.cpp` | Impl: eFuse MAC source | Line 16 |
| `firmware/src/platform/device_id.cpp` | Impl: full_id_from_mac, short_id_from_mac | Lines 68–87 |
| `firmware/src/domain/node_table.cpp` | Impl: compute_short_id, find_entry_index | Lines 62–66, 387–394 |
| `firmware/src/app/app_services.cpp` | Wiring: ID formation entry point | Lines 147–151 |
| `docs/product/wip/areas/nodetable/research/naviga-current-state.md` | Research: full field flow | §"Where NodeTable lives" |
