# First firmware implementation for #435 (v0.2 packet family)

**Issue:** [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435) — Post-P0: v0.2 packet redesign (Path A).  
**Design:** PR #436 (merged) — packet_truth_table_v02, packet_migration_v01_v02.

This is the **first implementation PR** for #435 after the design/truth-table PR #436. It introduces the v0.2 packet family in code with the documented staged migration policy.

## What this PR does

- **TX:** Sends **v0.2 only** — Node_Pos_Full (msg_type 0x06), Node_Status (0x07), Alive (0x02). No Core, Tail1, Tail2, or Info on the canonical send path.
- **RX:** Accepts **v0.1 + v0.2** during transition. Compatibility logic in `BeaconLogic::on_rx()`: 0x01–0x05 (v0.1) and 0x06–0x07 (v0.2) decode and apply to the same NodeTable; v0.1 branches commented as temporary.
- **Encoders:** `pos_full_codec` (17 B payload, Pos_Quality 2 B LE), `status_codec` (19 B, hw/fw uint16).
- **NodeTable:** `apply_pos_full()` (position + Pos_Quality, one step), `apply_status()` (full snapshot). Pos_Quality packed into existing `pos_flags`/`sats`; no new NodeEntry fields.
- **Slots:** 3 TX slots (PosFull, Alive, Status). Node_Status lifecycle unchanged (min_status_interval_ms, T_status_max, bootstrap, no hitchhiking).
- **Tests:** beacon_logic and ootb e2e updated for v0.2 TX/RX; new tests for PosFull and Status RX apply; compat tests for v0.1 RX unchanged.

## Migration policy (in code)

| Phase       | TX sends              | RX accepts                          |
|------------|------------------------|-------------------------------------|
| Transition | v0.2 only (0x06,0x07,0x02) | v0.2 **and** v0.1 (0x01–0x05) |
| Post-cutover (later PR) | v0.2 only | v0.2 only; v0.1 optionally log & drop |

- **No dual TX path.** We do not send both v0.1 and v0.2.
- **Core_Tail:** Removed from TX; no Tail enqueue. v0.1 RX still accepts Tail1 for compatibility.

## Constraints preserved

- NodeTable remains normalized product truth (#419).
- `hw_profile_id` / `fw_version_id` remain uint16.
- `uptime_10m` at canon layer (wire `uptime10m` in Status payload).
- M1Runtime single composition; domain radio-agnostic.

## Pre-coding note

Implementation note (wire IDs, compat policy, test plan) is in `_working/435_implementation_note.md` (commit 1).

## Validation

- `pio test -e test_native_nodetable -f test_beacon_logic` — 75 tests passed.
- `pio test -e test_native_nodetable -f test_ootb_e2e_native` — passed.
- `pio run -e devkit_e220_oled` — build succeeded.

Addresses #435 (first implementation step; compatibility cleanup remains for a later PR).
