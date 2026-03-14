# #419 Exit checklist and boundary note

## Exit criteria

- [x] Canonical master field table produced — `docs/product/areas/nodetable/policy/nodetable_master_field_table_v0.md`
- [x] Required Product Truth fields present in NodeEntry (node_id, short_id, node_name, position block, battery/uptime, role/profile, last_rx_rssi, snr_last; legacy ref kept as runtime-local only)
- [x] Legacy fields removed from canonical surface — not in BLE, not in persistence; commented as runtime-local in NodeEntry
- [x] Identity/name semantics aligned — single node_name field; is_self/short_id_collision derived
- [x] BLE projection aligned — last_seq removed from BLE; snr_last at offset 24 (127 = NA)
- [x] Persistence follow-through — snapshot v4 with node_name (68-byte record); v3 still accepted on restore
- [x] Tests updated — RecordView uses snr_last; last_seq asserted via find_entry_for_test; beacon_logic BLE assertion updated
- [x] Docs/inventory updated — master field table, snapshot format v4, index link
- [x] Boundary note — #420/#421/#422 left unchanged; see master table §10

## Boundary

- **#417** — Sender seq16 persistence: not touched.
- **#418** — Snapshot/restore: only minimal follow-through (v4 + node_name); v3/v4 accept policy.
- **#420** — TX formation/scheduling: no change.
- **#421** — RX semantics: no behavioral change; NodeEntry shape and BLE export only.
- **#422** — Packetization/throttle: no change.

## Files changed (summary)

- `firmware/src/domain/node_table.h` — NodeEntry: node_name, snr_last, comments (canon vs runtime-local)
- `firmware/src/domain/node_table.cpp` — BLE get_page/get_snapshot_page: snr_last at 24, reserved at 25; no last_seq in BLE
- `firmware/src/domain/nodetable_snapshot.h` — v3/v4 record sizes; version 4
- `firmware/src/domain/nodetable_snapshot.cpp` — pack/unpack node_name; unpack_record(record_bytes); restore accepts v3 or v4
- `firmware/src/platform/naviga_storage.h` — kMaxNodeTableSnapshotBytes 8192
- `firmware/test/test_node_table_domain/test_node_table_domain.cpp` — RecordView snr_last; last_seq via find_entry_for_test
- `firmware/test/test_beacon_logic/test_beacon_logic.cpp` — BLE record offset 24 = snr_last
- `docs/product/areas/nodetable/policy/nodetable_master_field_table_v0.md` — new
- `docs/product/areas/nodetable/policy/nodetable_snapshot_format_v0.md` — v4, node_name
- `docs/product/areas/nodetable/index.md` — link to master field table
