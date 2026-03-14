## Summary

This PR completes **#438**: final v0.2 protocol cutover. It removes the temporary v0.1 RX compatibility layer and finalizes protocol truth as **TX = v0.2 only**, **RX = v0.2 only**.

## What changed

- **`decode_header`** (packet_header.h): Rejects v0.1 msg_types 0x01, 0x03, 0x04, 0x05; accepts only 0x02 (Alive), 0x06 (Pos_Full), 0x07 (Status).
- **BeaconLogic RX**: Removed RX branches for BeaconCore, BeaconTail1, BeaconTail2, BeaconInfo; removed tail1/tail2/info codec includes.
- **NodeTable**: Removed `apply_tail1`, `apply_tail2`, `apply_info`; removed `last_applied_tail_ref_core_seq16` and `has_applied_tail_ref_core_seq16` from NodeEntry. **upsert_remote:** Alive (pos_valid=false) no longer overwrites position; only last_seq, last_seen_ms, last_rx_rssi updated (per packet_truth_table_v02).
- **nodetable_snapshot:** unpack_record no longer sets removed ref-tail fields.
- **Tests:** v0.1 RX tests removed; added `test_rx_v01_packets_dropped`, `test_decode_header_rejects_v01_msgtypes`; `test_rx_alive_does_not_overwrite_core_position` uses Pos_Full seed; test_geo_beacon_codec header/roundtrip limited to v0.2 types and reject tests for v0.1.
- **Docs:** packet_migration_v01_v02.md and packet_sets_v0.md state cutover complete; RX v0.2-only.

## Final protocol state

- **Packet family:** Node_Pos_Full (0x06), Node_Status (0x07), Alive (0x02) — see packet_truth_table_v02.md.
- **RX:** Only 0x02, 0x06, 0x07 accepted; 0x01, 0x03, 0x04, 0x05 dropped.
- **NodeTable:** apply_pos_full, apply_status; upsert_remote (Alive path does not overwrite position). No apply_tail1/2/info; no last_applied_tail_ref* in NodeEntry.

## Validation

- **test_native_nodetable:** test_beacon_logic, test_geo_beacon_codec, test_node_table_domain — **pass**.
- **devkit_e22_oled_gnss** build — **SUCCESS**.

## Out of scope / known unrelated issue

- **test_ble_transport_core** link error (undefined BLE symbols) is pre-existing and out of scope for this PR; not introduced by #438.

#438 is to be closed after this PR is merged.
