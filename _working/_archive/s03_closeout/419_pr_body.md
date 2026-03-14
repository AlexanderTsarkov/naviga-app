# NodeTable canonical field map alignment (#419)

## Summary
Aligns NodeTable canonical field set, BLE export, and persistence snapshot with Product Truth S03/V1. Adds master field table and snapshot format v4; keeps legacy ref fields runtime-local only.

**Umbrella:** #416. **Boundary:** #420 (TX), #421 (RX), #422 (packetization) unchanged.

## Changes
- **Canonical master field table** — New `nodetable_master_field_table_v0.md`: identity, position, battery, role, runtime, derived; legacy ref excluded; BLE/persisted classification.
- **node_name** — Single canonical name field in NodeEntry; persisted in snapshot v4 (1 byte length + 32 bytes).
- **BLE** — `last_seq` removed from BLE export (canon: NodeTable only); offset 24 = `snr_last` (127 = NA), 25 = reserved.
- **Snapshot v4** — 68-byte record (v3 + node_name); restore accepts v3 or v4; `kMaxNodeTableSnapshotBytes` 8192.
- **Legacy ref** — `last_core_seq16`, `has_core_seq16`, `last_applied_tail_ref*` remain in NodeEntry for decoder only; not in BLE, not persisted; documented as runtime-local.
- **Docs** — `nodetable_snapshot_format_v0.md` (v3/v4 layout, restore policy); index links to master table and snapshot format.

## Validation
- **`pio run -e devkit_e220_oled`** — **PASSED** (full firmware build).
- **NodeTable native tests** — Executed `.pio/build/test_native_nodetable/program`: **20 tests, 0 failures** (serialization, BLE record, RX semantics, snapshot restore/excluded/version/dirty).
- **Fresh rebuild of `test_native_nodetable`** — After `pio run -t clean`, rebuild currently **fails** (missing `Arduino.h` / HW profile). Pre-existing native test env limitation; **out of scope for #419**. Test binary run above confirms #419 test logic passes.

## Boundary
- #417 / #418 not touched.
- #420 / #421 / #422: no changes; remaining work stays there.

Closes #419.
