# #435 v0.2 packet family — implementation note (pre-coding)

**Issue:** [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435). **Design:** PR #436 (merged); [packet_truth_table_v02](../docs/product/areas/nodetable/policy/packet_truth_table_v02.md), [packet_migration_v01_v02](../docs/product/areas/nodetable/policy/packet_migration_v01_v02.md).

This note is the **authoritative pre-coding spec** for the first firmware implementation PR. No coding until this is written and agreed.

---

## 1) Packet / wire identifiers

| Packet          | msg_type (wire) | Notes |
|-----------------|------------------|--------|
| **Node_Pos_Full** | **0x06**         | New; v0.2 canonical. Replaces Core (0x01) + Tail1 (0x03) on TX. |
| **Node_Status**   | **0x07**         | New; v0.2 canonical. Replaces Tail2 (0x04) + Info (0x05) on TX. |
| **Alive**         | **0x02**         | Unchanged; retained in v0.2 family. |

- **TX:** We send only 0x06, 0x07, 0x02 (Node_Pos_Full, Node_Status, Alive).
- **RX:** We accept 0x01–0x07 during transition: 0x01 (Core), 0x02 (Alive), 0x03 (Tail1), 0x04 (Tail2), 0x05 (Info), 0x06 (Node_Pos_Full), 0x07 (Node_Status). v0.1 types (0x01, 0x03, 0x04, 0x05) are **compatibility-only**; dispatch remains in the same RX path, apply to same NodeTable.
- **Header decode:** Extend `decode_header()` to accept `mt <= 0x07` (or explicitly allow 0x06 and 0x07). Reject only Reserved (0x00) and unknown (> 0x07). This keeps compatibility in one place: the header validator allows 0x01–0x07; dispatch then routes to v0.1 or v0.2 handlers.

---

## 2) Compatibility policy in code

| Aspect | Policy |
|--------|--------|
| **TX** | Send **only** v0.2: Node_Pos_Full (0x06), Node_Status (0x07), Alive (0x02). Do **not** enqueue or send Core (0x01), Tail1 (0x03), Tail2 (0x04), Info (0x05). |
| **RX** | Accept **both** v0.1 (0x01, 0x03, 0x04, 0x05) and v0.2 (0x06, 0x07, 0x02). Decode and apply to the **same** NodeTable normalized fields. |
| **Where compatibility lives** | **RX only.** In `BeaconLogic::on_rx()`: after `decode_header()`, branch on `hdr.msg_type`. If 0x06 → decode Node_Pos_Full payload, call `NodeTable::apply_pos_full()`. If 0x07 → decode Node_Status payload, call `NodeTable::apply_status()`. If 0x01, 0x03, 0x04, 0x05 → existing v0.1 decode/apply (unchanged). If 0x02 → existing Alive. Compatibility is **explicit and narrow**: one switch on msg_type; v0.1 branches are commented as "v0.1 compatibility; remove in cleanup PR". |
| **Dual TX** | We do **not** send both v0.1 and v0.2. TX path uses only v0.2 encoders and slot set (PosFull, Status, Alive). |

---

## 3) Core_Tail and TX path

- **This PR fully removes Core and Tail from the canonical TX path.** We no longer enqueue Core_Pos (0x01) or Core_Tail (0x03). We enqueue **Node_Pos_Full (0x06)** when position is valid (same trigger as before: min_interval + allow_core, or max_silence). One seq16 per position update; no Tail, no ref_core_seq16.
- **Slot set:** Replace the five v0.1 slots (Core, Alive, Tail1, Tail2, Info) with **three v0.2 slots**: PosFull, Alive, Status. Slot indices: 0 = PosFull, 1 = Alive, 2 = Status. Selection order remains P0 (PosFull, Alive) > P3 (Status); Status uses same lifecycle (min_status_interval_ms, T_status_max, bootstrap).
- **v0.1 RX acceptance:** We **keep** the existing RX handlers for 0x01, 0x03, 0x04, 0x05 so that during transition we still accept old packets from other nodes. No change to v0.1 decode/apply logic; we only **add** handlers for 0x06 and 0x07.

---

## 4) Test plan

| Area | Tests |
|------|--------|
| **Wire IDs** | Header encode/decode accepts 0x06 and 0x07; rejects 0x00 and > 0x07 (or extended range per decision). |
| **Node_Pos_Full** | Pack: common prefix (9 B) + lat_u24, lon_u24, Pos_Quality (2 B). Unpack round-trip. Pos_Quality bit layout: fix_type (3 b), pos_sats (6 b), pos_accuracy_bucket (3 b), pos_flags_small (4 b) LE. |
| **Node_Status** | Pack: common prefix (9 B) + full snapshot (batteryPercent, battery_Est_Rem_Time, TX_power_Ch_throttle, uptime10m, role_id, maxSilence10s, hwProfileId, fwVersionId). Unpack round-trip. hw_profile_id / fw_version_id uint16 LE. |
| **TX formation** | With pos_valid and allow_core: only PosFull (0x06) enqueued, not Core or Tail1. With !pos_valid and time_for_silence: only Alive (0x02) enqueued. Status (0x07) enqueued per lifecycle (bootstrap, min_status_interval, T_status_max); not enqueued in same pass as PosFull (no hitchhiking). |
| **RX apply** | Node_Pos_Full (0x06): apply_pos_full() updates position, last_core_seq16 := seq16, Pos_Quality fields; no Tail ref. Node_Status (0x07): apply_status() updates full status snapshot (battery, uptime_10m, tx_power/channel_throttle, role_id, max_silence_10s, hw_profile_id, fw_version_id). Alive (0x02): unchanged. |
| **Compatibility** | RX accepts 0x01 (Core) and applies via existing upsert_remote. RX accepts 0x03 (Tail1) and applies via apply_tail1. RX accepts 0x04 (Tail2) and 0x05 (Info) and applies via apply_tail2 / apply_info. All apply to same NodeTable; no dual truth. |
| **Widths** | Node_Status encode/decode and apply use hw_profile_id and fw_version_id as uint16; unit tests assert no truncation. |
| **Packet size** | Node_Pos_Full payload 17 B; Node_Status payload ≤ 32 B (Default budget). Unit or compile-time assertions. |
| **Devkit** | Build passes; existing tests updated or extended; new tests for PosFull and Status pack/unpack and apply. |

---

## 5) File / module ownership

| Component | Location | Change |
|-----------|----------|--------|
| msg_type registry | `protocol/packet_header.h` | Add BeaconPosFull = 0x06, BeaconStatus = 0x07; extend decode_header to accept 0x06–0x07. |
| Node_Pos_Full codec | `protocol/pos_full_codec.h` (new) | Encode/decode 17 B payload; Pos_Quality 2 B LE (fix_type 3b, pos_sats 6b, pos_accuracy_bucket 3b, pos_flags_small 4b). |
| Node_Status codec | `protocol/status_codec.h` (new) | Encode/decode full snapshot; field order and sentinels per truth table; hw/fw uint16. |
| TX formation | `domain/beacon_logic.cpp` | Switch to 3 slots: PosFull, Alive, Status. Enqueue PosFull (not Core+Tail), Status (not Tail2+Info). Same triggers and lifecycle. |
| RX dispatch | `domain/beacon_logic.cpp` | Add branches for 0x06 (decode PosFull, apply_pos_full), 0x07 (decode Status, apply_status). Keep 0x01, 0x03, 0x04, 0x05 for compatibility. |
| NodeTable apply | `domain/node_table.h/.cpp` | Add apply_pos_full(), apply_status(). apply_pos_full: position + Pos_Quality + last_core_seq16 := seq16. apply_status: full status snapshot; no position update. |
| Tests | `test/test_beacon_logic`, `test/test_*_codec` | New tests for PosFull/Status encode/decode; TX enqueue only v0.2; RX apply; compatibility (accept 0x01–0x05). |

---

## 6) Node_Status payload layout (fixed for implementation)

- Common prefix: 9 B (payloadVersion, nodeId48, seq16).
- Useful payload (fixed order, all present for simplicity; optionality TBD in contract):
  - batteryPercent 1 B
  - battery_Est_Rem_Time 1 B (e.g. 10-minute units; 0xFF = N/A)
  - TX_power_Ch_throttle 1 B (4+4 bits or single byte)
  - uptime10m 1 B
  - role_id 1 B
  - maxSilence10s 1 B
  - hwProfileId 2 B LE
  - fwVersionId 2 B LE
- Total payload: 9 + 10 = **19 B**. On-air 21 B. Fits Default (32 B).

---

## 7) Pos_Quality bit layout (2 B LE)

- Bit layout (LSB first in first byte): fix_type [2:0], pos_sats [8:3], pos_accuracy_bucket [11:9], pos_flags_small [15:12]. So: byte0 = fix_type | (pos_sats << 3); byte1 = (pos_sats >> 5) | (pos_accuracy_bucket << 1) | (pos_flags_small << 4). Actually per gnss_tail: fix_type 3b, pos_sats 6b, pos_accuracy_bucket 3b, pos_flags_small 4b = 16 bits. LE: low byte = bits 0–7, high byte = bits 8–15. So: bits 0–2 = fix_type, 3–8 = pos_sats (6 bits), 9–11 = pos_accuracy_bucket, 12–15 = pos_flags_small. Byte0 = fix_type | (pos_sats << 3). Byte1 = (pos_sats >> 5) | (pos_accuracy_bucket << 3) | (pos_flags_small << 6). Wait, pos_sats is 6 bits: so bits 3–8 in the 16-bit word. Byte0 bits 3–7 = pos_sats bits 0–4, byte1 bit 0 = pos_sats bit 5. So byte1 = (pos_sats >> 5) | (pos_accuracy_bucket << 1) | (pos_flags_small << 4). Good.
- Decode: fix_type = word & 0x07; pos_sats = (word >> 3) & 0x3F; pos_accuracy_bucket = (word >> 9) & 0x07; pos_flags_small = (word >> 12) & 0x0F.

---

## 8) What the later compatibility-cleanup PR will do

- Remove RX branches for 0x01, 0x03, 0x04, 0x05 (or gate behind a build/config flag).
- Remove or confine last_applied_tail_ref_core_seq16 / apply_tail1 to legacy-only path.
- Update seq_ref policy docs to v0.2-only.

This PR does **not** remove v0.1 RX; it only adds v0.2 TX and RX and keeps v0.1 RX for transition.
