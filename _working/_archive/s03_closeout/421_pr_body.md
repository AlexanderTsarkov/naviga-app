# RX semantics v0.1 canon alignment (#421)

## Summary
Makes RX/apply semantics **explicit** in code and docs: comments and policy reference state that behavior mirrors rx_semantics_v0 and packet_to_nodetable_mapping_v0. **No behavior change** — implementation already matched canon; this is documentation/truth-lock only.

**Umbrella:** #416. **Boundary:** #422 = packetization / v0.2 (no change in this PR).

## Changes
- **node_table.cpp** — Comment block above `upsert_remote`: RX/apply semantics follow rx_semantics_v0 and packet_to_nodetable_mapping_v0 (Newer vs Same/Older; no overwrite of position/telemetry). **Legacy ref fields** (last_core_seq16, last_applied_tail_ref_core_seq16) are **runtime-local decoder state only** — not BLE, not persisted (nodetable_master_field_table_v0 §7). Short comment above `apply_tail1`: ref match and one-per-ref; MUST NOT touch position (rx_semantics_v0 §4).
- **rx_semantics_v0.md** — §6 Implementation: implementation lives in node_table.cpp (upsert_remote, apply_tail1, apply_tail2, apply_info); ref fields runtime-local per master table §7.

## Validation
- **`pio run -e devkit_e220_oled`** — PASSED.
- **Tests:** No new tests added. Existing tests (test_beacon_logic apply_tail1, test_node_table_domain seq/ooo and restore exclusions) are relied upon; no claim of new coverage or new green suites beyond what was already in place.

## Boundary
- #422 (packetization / v0.2): no change. RX/apply remains v0.1.

Closes #421.
