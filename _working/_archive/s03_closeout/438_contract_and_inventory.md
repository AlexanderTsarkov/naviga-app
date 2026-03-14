# #438 — Final protocol cutover: v0.2-only RX/TX

**Issue:** #438 (final post-v0.2 cleanup / cutover).  
**Baseline:** #436 (design), #437 (main implementation).  
**Canon:** packet_truth_table_v02.md, packet_migration_v01_v02.md.

---

## Contract (restatement)

- **TX = v0.2 only** (already true from #437): Node_Pos_Full (0x06), Node_Status (0x07), Alive (0x02).
- **RX = v0.2 only** after this issue: accept only 0x02, 0x06, 0x07; **reject** 0x01 (BeaconCore), 0x03 (BeaconTail1), 0x04 (BeaconTail2), 0x05 (BeaconInfo).
- No redesign of v0.2 packet family; no BLE/app/UI; no change to canonical field widths or packet roles.
- Remove temporary v0.1 RX compatibility and all obsolete compat/apply remnants; tests and docs state v0.2-only.

---

## Cleanup inventory

| Location | What exists | Action |
|----------|-------------|--------|
| **packet_header.h** | `decode_header` accepts msg_type ≤ BeaconStatus (0x07) | Reject 0x01, 0x03, 0x04, 0x05; accept only 0x02, 0x06, 0x07. |
| **beacon_logic.cpp** | RX branches for BeaconCore, BeaconTail1, BeaconTail2, BeaconInfo | Remove all four branches. |
| **beacon_logic.cpp/h** | Includes: tail1_codec, tail2_codec, info_codec | Remove from both (no longer used by RX). Keep geo_beacon_codec.h in header for GeoBeaconFields API. |
| **node_table** | `apply_tail1`, `apply_tail2`, `apply_info` | Remove. |
| **NodeEntry** | `last_applied_tail_ref_core_seq16`, `has_applied_tail_ref_core_seq16` | Remove (only used by apply_tail1). Keep last_core_seq16/has_core_seq16 (used by apply_pos_full). |
| **nodetable_snapshot.cpp** | `unpack_record` sets ref tail fields to 0 | Remove the two lines. |
| **Tests** | test_rx_core_success, Tail1/2/Info RX tests, test_tail_never_overwrites_position, test_rx_dedupe_*, test_decode_header_accepts_0x05 | Remove or replace with v0.1-reject tests; test_rx_alive_does_not_overwrite_core_position: seed with Pos_Full not Core. |
| **test_geo_beacon_codec** | test_header_roundtrip_all_types (includes 0x01, 0x03, 0x04) | Roundtrip only 0x02, 0x06, 0x07; add test that 0x01, 0x03, 0x04, 0x05 are rejected. |
| **Docs** | packet_migration_v01_v02.md, packet_sets_v0.md | State transition complete; RX v0.2-only. |

---

## Implementation summary (done)

- **packet_header.h:** `decode_header()` rejects msg_type 0x01, 0x03, 0x04, 0x05; accepts only 0x02, 0x06, 0x07.
- **beacon_logic:** Removed RX branches for BeaconCore, BeaconTail1, BeaconTail2, BeaconInfo; removed tail1/tail2/info codec includes from header/cpp.
- **NodeTable:** Removed `apply_tail1`, `apply_tail2`, `apply_info`; removed `last_applied_tail_ref_core_seq16` and `has_applied_tail_ref_core_seq16` from NodeEntry. **upsert_remote:** Alive (pos_valid=false) no longer overwrites position; only last_seq, last_seen_ms, last_rx_rssi updated (packet_truth_table_v02).
- **nodetable_snapshot:** unpack_record no longer sets ref tail fields.
- **Tests:** test_rx_v01_packets_dropped, test_decode_header_rejects_v01_msgtypes; test_rx_alive_does_not_overwrite uses Pos_Full seed + tolerance for lat/lon; v0.1 RX tests and seed_core removed. test_geo_beacon_codec: header golden decode uses 0x02; framed Core decode uses payload-only decode; round_trip/clamp/bad_version/nodeid use payload decode.
- **Docs:** packet_migration_v01_v02.md and packet_sets_v0.md state cutover complete; RX v0.2-only.

**Validation:** test_native_nodetable (test_beacon_logic, test_geo_beacon_codec, test_node_table_domain) pass. devkit_e22_oled_gnss build SUCCESS. test_ble_transport_core link error is pre-existing (undefined BLE symbols), not introduced by #438.

---

## Canonical truth after #438

- **Packet family:** Node_Pos_Full, Node_Status, Alive (packet_truth_table_v02.md).
- **RX:** Only 0x02, 0x06, 0x07 accepted; 0x01, 0x03, 0x04, 0x05 dropped.
- **NodeTable:** apply_pos_full, apply_status; upsert_remote (Alive path only for non-position). No apply_tail1/2/info. No last_applied_tail_ref* in NodeEntry.
