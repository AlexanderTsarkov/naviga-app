## #419 implementation complete

Implementation is packaged in **PR #431** ([link](https://github.com/AlexanderTsarkov/naviga-app/pull/431)).

**Aligned work:**
- **Canonical master field table** — `docs/product/areas/nodetable/policy/nodetable_master_field_table_v0.md` added; field classification (identity, position, battery, role, runtime, derived), BLE/persisted, legacy ref excluded.
- **node_name** — Single canonical name field in NodeEntry; persisted in snapshot v4 (68-byte record).
- **BLE** — `last_seq` removed from BLE export (canon: NodeTable only); offset 24 = `snr_last` (127 = NA), 25 = reserved.
- **Snapshot v4** — 68-byte record; restore accepts v3 or v4; `kMaxNodeTableSnapshotBytes` 8192.
- **Legacy ref fields** — Remain in NodeEntry for decoder only; not in BLE, not persisted; documented as runtime-local.

**Validation:**
- `pio run -e devkit_e220_oled` — passed.
- NodeTable native test binary executed: 20/20 tests passed.
- Fresh rebuild of `test_native_nodetable` env still fails (pre-existing `Arduino.h` / HW profile issue); out of scope for #419.

**Remaining work:** #420 (TX rules), #421 (RX semantics), #422 (packetization) only. No further #419 scope.

After merge of #431, this issue can be closed.
