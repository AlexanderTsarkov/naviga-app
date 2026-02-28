# Tail-2 Split Cutover Report (PR0 Research Map)

**Status:** WIP — Research only. No behavior changes.
**Work Area:** Product Specs WIP · **Iteration:** S02__2026-03__docs_promotion_and_arch_audit
**Drives:** [#314](https://github.com/AlexanderTsarkov/naviga-app/issues/314) · [#315](https://github.com/AlexanderTsarkov/naviga-app/issues/315) · [#316](https://github.com/AlexanderTsarkov/naviga-app/issues/316) · [#317](https://github.com/AlexanderTsarkov/naviga-app/issues/317)
**Umbrella:** [#318](https://github.com/AlexanderTsarkov/naviga-app/issues/318) · **This issue:** [#313](https://github.com/AlexanderTsarkov/naviga-app/issues/313)

---

## 1) Scope & Intent

This report is the PR0 research map for the Tail-2 split. Its sole purpose is to inventory the current state of canon docs, WIP docs, and firmware touchpoints so that the follow-up issues (#314–#317) have exact file paths and change descriptions. No doc edits and no code changes are made here. The split introduces `msg_type=0x05` (`Node_OOTB_Informative` / legacy wire: `BEACON_INFO`) as an independent Informative-only packet, and narrows `msg_type=0x04` (`Node_OOTB_Operational` / legacy: `BEACON_TAIL2`) to Operational fields only.

---

## 2) Canonical Alias Table (0x01–0x05)

| `msg_type` | Canonical alias (`<Originator>_<Scope>_<Intent>`) | Legacy wire enum | Status |
|---|---|---|---|
| `0x01` | `Node_OOTB_Core_Pos` | `BEACON_CORE` | Defined |
| `0x02` | `Node_OOTB_I_Am_Alive` | `BEACON_ALIVE` | Defined |
| `0x03` | `Node_OOTB_Core_Tail` | `BEACON_TAIL1` | Defined |
| `0x04` | `Node_OOTB_Operational` | `BEACON_TAIL2` | Defined; scope narrows to Operational-only |
| `0x05` | `Node_OOTB_Informative` | `BEACON_INFO` | **New** — currently reserved/drop in registry and firmware |

**Rule:** Canon docs and policy MUST use the `<Originator>_<Scope>_<Intent>` alias as the primary name. Legacy `BEACON_*` names appear parenthetically for firmware/wire traceability only. Do not invent new `BEACON_*` names.

---

## 3) Current Canon State

### 3.1 msg_type Registry

**File:** `docs/protocols/ootb_radio_v0.md`
**Section:** `§3.2 msg_type registry (v0)`

Current table lists `0x01`–`0x04` as defined; `0x05–0x7F` as `(reserved) → drop`. The firmware mirrors this exactly: `decode_header` in `firmware/protocol/packet_header.h` (line 97) drops any `msg_type > BeaconTail2` (i.e., `0x05+`).

**Gap:** `0x05` must be promoted from reserved to a named entry (`BEACON_INFO`). Both the doc registry and the firmware drop guard must be updated.

### 3.2 Tail-2 Contract and Operational/Informative Split Language

**File:** `docs/product/areas/nodetable/contract/tail2_packet_encoding_v0.md`
**Sections:** `§1 Role and invariants` (scheduling classes reference), `§3 Byte layout (v0)`

The contract defines a single `msg_type=0x04` packet carrying all fields — including `maxSilence10s` at byte offset 7 — and acknowledges the two scheduling classes via a cross-reference to `field_cadence_v0.md §2.2`. The byte layout is:

| Offset | Field | Class |
|---|---|---|
| 0 | `payloadVersion` | — |
| 1–6 | `nodeId48` | — |
| 7 | `maxSilence10s` | **Informative** |
| 8 | `batteryPercent` | Operational |
| 9–10 | `hwProfileId` | Operational |
| 11–12 | `fwVersionId` | Operational |
| 13–16 | `uptimeSec` | Operational |

**Gap:** After the split, `maxSilence10s` moves to `0x05`; the `0x04` layout must be updated to remove it. A new contract doc for `0x05` must be created.

### 3.3 Field Cadence Policy

**File:** `docs/product/areas/nodetable/policy/field_cadence_v0.md`
**Section:** `§2.2 Tail-2 scheduling classes (v0)`

Normative text defines two classes within a single `msg_type=0x04`:
- **Operational:** on-change + at forced Core.
- **Informative:** on-change + fixed slow cadence (default 10 min). `maxSilence10s` is explicitly Informative; MUST NOT be sent on every operational Tail-2 send.

**Gap:** After the split, §2.2 must describe two separate packets, not two scheduling classes within one packet. The field table must be updated to assign each field to its packet.

### 3.4 Other Canon Docs with Tail-2 References

| File | Section | Relevance |
|---|---|---|
| `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | `§3 Packet types table`, `§4.3 BeaconTail-2` | Lists `0x04` with "Operational and Informative" scheduling note; must add `0x05` row |
| `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md` | `§2 Main table` | `maxSilence10s` row lists packet placement as `BeaconTail-2`; must update to `Node_OOTB_Informative (0x05)` |
| `docs/product/areas/nodetable/policy/rx_semantics_v0.md` | `§2 BeaconTail-2 acceptance` | References `tail2_packet_encoding_v0.md §4.1`; must add `0x05` acceptance rules |
| `docs/product/areas/nodetable/policy/activity_state_v0.md` | `maxSilence10s` references | References `maxSilence10s` from Tail-2; packet source changes to `0x05` |
| `docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md` | `§8 Related` | References Tail-2 by contrast; no semantic change; may need alias update |
| `docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md` | `§8 Related` | References `maxSilence10s` as carried by Tail-2; must update to `0x05` |
| `docs/product/areas/nodetable/index.md` | Packet family summary | Lists Tail-2; must add `0x05` entry |
| `docs/product/areas/domain/policy/role_profiles_policy_v0.md` | `maxSilence10s` references | References the field; packet source changes to `0x05` |

---

## 4) Current WIP State

| File | Relevance |
|---|---|
| `docs/product/wip/spec_map_v0.md` | Inventory row for `tail2_packet_encoding_v0.md` (Promoted, V1-A IN); changelog records canonization. Must add `0x05` contract row when created. |
| `docs/product/wip/areas/nodetable/policy/field_cadence_v0.md` | Redirect stub only — points to canon. No conflict. |
| `docs/product/wip/research/seq_audit_s02.md` | Mentions Tail-2 as one of four packet types sharing the per-node seq16 counter. No conflict; `0x05` will also share the counter. |
| `docs/product/wip/research/geo_encoding_audit_s02.md` | Mentions Tail-2 as correct placement for `pos_age_s`. No conflict with split. |
| `docs/product/wip/research/nodeid_audit_s02.md` | Lists Tail-2 as a packet type carrying `nodeId48`. `0x05` will also carry `nodeId48`; no conflict. |
| `docs/product/wip/areas/radio/policy/traffic_model_v0.md` | References BeaconTail as best-effort packet class; `ProductMaxFrameBytes=96`. `0x05` max payload (8 B + header = 10 B on-air) fits all profile budgets. No conflict. |

---

## 5) Current Code State (Firmware Touchpoints)

### 5.1 MsgType Enum

**File:** `firmware/protocol/packet_header.h`
**Lines:** 37–43

```cpp
enum class MsgType : uint8_t {
  Reserved    = 0x00,
  BeaconCore  = 0x01,
  BeaconAlive = 0x02,
  BeaconTail1 = 0x03,
  BeaconTail2 = 0x04,
  // 0x05 is absent — treated as unknown → drop by decode_header
};
```

**Change needed (#315):** Add `BeaconInfo = 0x05` (canonical alias: `Node_OOTB_Informative`).

### 5.2 Header Decode Drop Guard

**File:** `firmware/protocol/packet_header.h`
**Lines:** 96–98 (`decode_header`)

```cpp
if (mt == static_cast<uint8_t>(MsgType::Reserved) ||
    mt > static_cast<uint8_t>(MsgType::BeaconTail2)) {
  return false;  // drops 0x05 today
}
```

**Change needed (#315):** Upper bound must be raised to `BeaconInfo` (`0x05`) once the enum entry is added.

### 5.3 RX Dispatch / NodeTable Apply

**File:** `firmware/src/domain/beacon_logic.cpp`
**Lines:** 188–209

The `on_rx` switch handles `MsgType::BeaconTail2` and calls `decode_tail2_payload` + `table.apply_tail2(...)`. The `apply_tail2` signature (in `firmware/src/domain/node_table.h`, line 120) accepts `has_max_silence` and `max_silence_10s` alongside the Operational fields.

**Changes needed (#315):**
- Add `MsgType::BeaconInfo` case in `on_rx` switch (`beacon_logic.cpp`).
- New `decode_info_payload` function (new codec header, e.g. `firmware/protocol/info_codec.h`).
- New `table.apply_info(node_id, has_max_silence, max_silence_10s, rssi_dbm, now_ms)` method in `node_table.h` / `node_table.cpp`.
- Remove `has_max_silence` / `max_silence_10s` parameters from `apply_tail2` (they move to `apply_info`).
- Add `PacketLogType::INFO` to the enum in `beacon_logic.h` (line 16).

### 5.4 TX Scheduling / Queueing

**File:** `firmware/src/domain/beacon_logic.cpp`
**Lines:** 18–86 (`build_tx`)

`build_tx` currently produces only `BeaconCore` (`0x01`) or `BeaconAlive` (`0x02`). **Tail-2 TX is unreachable** — `encode_tail2_frame` is never called from `build_tx`. This is confirmed by `_working/protocol_research/2026-02-25_tail_scheduling_prep.md §2.3`.

**Changes needed (#316):**
- Add independent `build_tail2_tx` (Operational path): triggers on Operational field change + at forced Core.
- Add independent `build_info_tx` (Informative path): triggers on `maxSilence10s` change OR slow cadence expiry (default 10 min).
- Add replaced/expired counter for pending `0x05` frames (observability).
- Neither path gates on the other.

### 5.5 Existing Tests

**File:** `firmware/test/test_beacon_logic/test_beacon_logic.cpp`

Existing Tail-2 tests (lines 400–704): `test_tail2_codec_base_round_trip`, `test_tail2_codec_with_max_silence_round_trip`, `test_tail2_decode_bad_version_dropped`, `test_tail2_rx_applies_max_silence`, `test_tail2_rx_no_prior_core_creates_entry`, `test_tail2_rx_bad_version_dropped`.

After the split, `test_tail2_codec_with_max_silence_round_trip` and `test_tail2_rx_applies_max_silence` must be updated (or replaced) because `maxSilence10s` will no longer be in `0x04` frames. New tests for `0x05` are tracked in #317.

### 5.6 Touchpoints Not Found

- No mobile app (Swift/Kotlin) files found that directly reference `BEACON_TAIL2`, `MsgType`, or `tail2_codec`. Mobile RX path is **not found** in this repo — either not present or in a separate repo. Scope of #315 is firmware only.
- No separate TX queue / scheduler file found beyond `beacon_logic.cpp`. TX scheduling for Tail-2 and Informative is entirely new work (#316).

---

## 6) Target State

### Node_OOTB_Operational (`msg_type=0x04`, legacy `BEACON_TAIL2`)

**Carries:** `payloadVersion` + `nodeId48` + Operational optional fields only.

| Offset | Field | Encoding |
|---|---|---|
| 0 | `payloadVersion` | `0x00` |
| 1–6 | `nodeId48` | uint48 LE |
| 7 | `batteryPercent` | uint8; `0xFF` = absent |
| 8–9 | `hwProfileId` | uint16 LE; `0xFFFF` = absent |
| 10–11 | `fwVersionId` | uint16 LE; `0xFFFF` = absent |
| 12–15 | `uptimeSec` | uint32 LE; `0xFFFFFFFF` = absent |

Min payload: 7 B. Max payload: 16 B. On-air: 9–18 B. Fits LongDist budget (24 B).

**Cadence:** On Operational field change + at forced Core (every N Core beacons; N implementation-defined).

### Node_OOTB_Informative (`msg_type=0x05`, legacy `BEACON_INFO`)

**Carries:** `payloadVersion` + `nodeId48` + Informative optional fields only.

| Offset | Field | Encoding |
|---|---|---|
| 0 | `payloadVersion` | `0x00` |
| 1–6 | `nodeId48` | uint48 LE |
| 7 | `maxSilence10s` | uint8; `0x00` = absent; unit = 10 s |

Min payload: 7 B. Current max payload: 8 B (extensible). On-air: 9–10 B. Fits all profile budgets.

**Cadence:** On `maxSilence10s` change + fixed slow cadence (default 10 min). MUST NOT be sent on every Operational send.

**Loss-tolerance invariant (inherited):** Product MUST function correctly with zero `0x05` packets received. `0x05` absence is the normal operating condition.

---

## 7) Field Migration Table

| Field | Current packet | Target packet | Class | Notes |
|---|---|---|---|---|
| `payloadVersion` | `0x04` | Both `0x04` and `0x05` | — | First byte of every payload |
| `nodeId48` | `0x04` | Both `0x04` and `0x05` | — | Bytes 1–6 of every payload |
| `maxSilence10s` | `0x04` (byte 7) | **`0x05`** (byte 7) | **Informative** | Moves entirely to `0x05` |
| `batteryPercent` | `0x04` (byte 8) | `0x04` (byte 7) | Operational | Offset shifts by −1 after `maxSilence10s` removed |
| `hwProfileId` | `0x04` (bytes 9–10) | `0x04` (bytes 8–9) | Operational | Offset shifts by −1 |
| `fwVersionId` | `0x04` (bytes 11–12) | `0x04` (bytes 10–11) | Operational | Offset shifts by −1 |
| `uptimeSec` | `0x04` (bytes 13–16) | `0x04` (bytes 12–15) | Operational | Offset shifts by −1 |

> **Note on offset shift:** Removing `maxSilence10s` from `0x04` shifts all subsequent optional field offsets by −1. The `0x04` codec (`tail2_codec.h`) and all offset constants (`kTail2OffsetBattery`, etc.) must be updated. Existing golden vectors for `0x04` will change.

---

## 8) Follow-up PR Touch List

### #314 — Docs changes

- `docs/protocols/ootb_radio_v0.md §3.2` — Add `0x05` (`BEACON_INFO`) row to registry; remove from reserved range.
- `docs/product/areas/nodetable/contract/tail2_packet_encoding_v0.md §1, §3` — Remove `maxSilence10s` from layout; update role description to "Operational-only"; update byte offsets for remaining fields.
- **New file** `docs/product/areas/nodetable/contract/tail2_info_packet_encoding_v0.md` — Full `0x05` contract: byte layout, `maxSilence10s`, RX rules, loss-tolerance invariant, examples.
- `docs/product/areas/nodetable/policy/field_cadence_v0.md §2.2` — Replace "two scheduling classes in one packet" with "two separate packets"; update field table.
- `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md §3, §4.3` — Add `0x05` row to packet-types table; update §4.3 to Operational-only; add new §4.4 for `0x05`.
- `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md §2` — Update `maxSilence10s` row: packet placement → `Node_OOTB_Informative (0x05)`.
- `docs/product/areas/nodetable/policy/rx_semantics_v0.md` — Add `0x05` acceptance rules (parallel to existing `0x04` rules).
- `docs/product/areas/nodetable/policy/activity_state_v0.md` — Update `maxSilence10s` source reference from `0x04` to `0x05`.
- `docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md §8 Related` — Update `maxSilence10s` source to `0x05`.
- `docs/product/areas/nodetable/index.md` — Add `0x05` to packet family summary.
- `docs/product/areas/domain/policy/role_profiles_policy_v0.md` — Update `maxSilence10s` source to `0x05`.
- `docs/product/wip/spec_map_v0.md` — Add inventory row for new `tail2_info_packet_encoding_v0.md`.
- **Naming memo** — Add canonical alias table (`0x01`–`0x05`) to `ootb_radio_v0.md §3.2` or as a standalone `docs/product/areas/nodetable/contract/naming_aliases_v0.md`.

### #315 — Firmware changes (RX + dispatch)

- `firmware/protocol/packet_header.h` (line 42) — Add `BeaconInfo = 0x05` to `MsgType` enum.
- `firmware/protocol/packet_header.h` (line 97) — Raise drop guard upper bound from `BeaconTail2` to `BeaconInfo`.
- **New file** `firmware/protocol/info_codec.h` — `InfoFields` struct (`node_id`, `has_max_silence`, `max_silence_10s`), `encode_info_frame`, `decode_info_payload`, offset constants, `InfoDecodeError` enum.
- `firmware/src/domain/beacon_logic.h` (line 16) — Add `PacketLogType::INFO` to enum.
- `firmware/src/domain/beacon_logic.cpp` (after line 209) — Add `MsgType::BeaconInfo` case in `on_rx`; call `decode_info_payload` + `table.apply_info(...)`.
- `firmware/src/domain/node_table.h` (line 120) — Remove `has_max_silence`/`max_silence_10s` from `apply_tail2` signature; add new `apply_info(node_id, has_max_silence, max_silence_10s, rssi_dbm, now_ms)` declaration.
- `firmware/src/domain/node_table.cpp` — Implement `apply_info`; update `apply_tail2` to remove max_silence handling.
- `firmware/src/app/m1_runtime.cpp` (line 22) — Add `PacketLogType::INFO → "INFO"` to `packet_log_type_str()`; update `is_tail_type()` if needed.
- `firmware/protocol/tail2_codec.h` — Update byte offsets (`kTail2OffsetBattery` → 7, `kTail2OffsetHwProfile` → 8, etc.) after removing `maxSilence10s` from `0x04` layout.

### #316 — TX policy / queue fairness

- `firmware/src/domain/beacon_logic.h` — Add `build_tail2_tx` and `build_info_tx` method declarations; add replaced/expired counter member.
- `firmware/src/domain/beacon_logic.cpp` — Implement `build_tail2_tx` (Operational: on-change + forced Core) and `build_info_tx` (Informative: on-change + 10 min cadence); neither path gates on the other.
- `firmware/src/app/m1_runtime.cpp` — Wire `build_tail2_tx` and `build_info_tx` into the TX loop.

### #317 — Tests / evidence

- `firmware/test/test_beacon_logic/test_beacon_logic.cpp` — Update `test_tail2_codec_with_max_silence_round_trip` and `test_tail2_rx_applies_max_silence` (maxSilence10s no longer in `0x04`); add new golden vectors for `0x04` (updated offsets) and `0x05`.
- **New test** `firmware/test/test_beacon_logic/test_info_codec.cpp` (or inline) — `test_info_codec_base_round_trip`, `test_info_codec_with_max_silence_round_trip`, `test_info_decode_bad_version_dropped`, `test_info_rx_applies_max_silence`.
- **TX independence test** — Verify `0x04` TX never includes `maxSilence10s`; `0x05` TX fires independently.
- **Cadence/fairness test** — Verify replaced/expired counter increments under simulated congestion.

---

## 9) Exit Criteria Checklist

- [x] Migration table present (§7)
- [x] Touch list present, grouped by #314–#317 (§8)
- [x] Canon inventory present (§3)
- [x] WIP inventory present (§4)
- [x] Code inventory present (§5)
- [x] PR references #313 and #318 (see header)
