# PR: #418 — NodeTable persistence snapshot format + restore policy (canon-aligned)

## Summary

Implements **#418**: NodeTable persistence snapshot format and restore policy aligned with corrected S03/V1 canon (product_truth_s03_v1, seq_ref_version_link_metrics_v0). No legacy ref-state in snapshot; restored entries start stale.

**Umbrella:** #416. **Boundary:** #419 owns NodeTable canonical field-map / master-table alignment; this PR touches only snapshot schema and restore semantics.

## Changes

- **Snapshot format v3 (35-byte record):** Persisted set: node_id, short_id, pos_valid, lat_e7, lon_e7, pos_age_s, has_pos_flags/pos_flags/has_sats/sats, telemetry (battery, uptime, max_silence, hw_profile, fw_version). **Not** persisted: last_seen_ms, last_seq, last_rx_rssi, last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16 (canon §7).
- **Restore:** Unpack sets runtime/derived/legacy fields to 0/false; is_self derived from (node_id == self_node_id). last_seen_ms = 0 ⇒ restored entries start stale.
- **Version:** Only v3 accepted; v1/v2 (legacy layouts) rejected ⇒ clean start.
- **Wiring:** Unchanged: app_services restore after runtime_.init(); save in tick with 30 s debounce when nodetable_dirty(); NVS keys nt_snap_len / nt_snap.

## Tests

- `test_nodetable_snapshot_restore_is_self_derived` — round-trip, is_self derived, last_seen_ms 0, pos_age_s round-trip, has_core_seq16/last_core_seq16 false/0.
- `test_nodetable_snapshot_excluded_fields_not_authoritative` — all excluded fields 0/false after restore.
- `test_nodetable_snapshot_corrupt_returns_zero`, `test_nodetable_snapshot_old_version_rejected` (v1 and v2), `test_nodetable_dirty_cleared_after_clear`.

All 20 tests in test_node_table_domain pass.

## Docs

- **New:** `docs/product/areas/nodetable/policy/nodetable_snapshot_format_v0.md` — snapshot format and restore policy.
- **Index:** Added link in NodeTable hub §4 Policies.
- **Working:** `_working/research/418_contract_and_classification.md` — contract restatement and classification table with file-path evidence.

## Intentionally left for #419

- NodeTable canonical field-map alignment (e.g. uptime_10m naming, node_name, full product field set).
- Any new fields or renames that #419 adds to NodeEntry; snapshot record layout is not extended in this PR.

Closes #418.
