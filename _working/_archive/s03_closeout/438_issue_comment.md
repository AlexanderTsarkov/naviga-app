**PR:** #439 — [feat(#438): remove v0.1 RX compatibility and finalize v0.2-only cutover](https://github.com/AlexanderTsarkov/naviga-app/pull/439)

**Summary**
- Removed temporary v0.1 RX compatibility: `decode_header` and BeaconLogic now accept only 0x02 (Alive), 0x06 (Pos_Full), 0x07 (Status); 0x01, 0x03, 0x04, 0x05 are rejected.
- Removed obsolete compat/apply: `apply_tail1`, `apply_tail2`, `apply_info` and `last_applied_tail_ref_core_seq16` / `has_applied_tail_ref_core_seq16` from NodeEntry; Alive path in `upsert_remote` no longer overwrites position.
- Docs updated: packet_migration_v01_v02.md and packet_sets_v0.md state cutover complete; RX v0.2-only.

**Final protocol state**
- TX = v0.2 only; RX = v0.2 only. Packet family: Node_Pos_Full, Node_Status, Alive.

**Validation**
- test_native_nodetable (test_beacon_logic, test_geo_beacon_codec, test_node_table_domain) — pass.
- devkit_e22_oled_gnss build — SUCCESS.
- test_ble_transport_core link error remains pre-existing; out of scope.

#438 to be closed after PR merge.
