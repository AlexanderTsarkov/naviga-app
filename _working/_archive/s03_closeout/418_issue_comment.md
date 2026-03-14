# Issue #418 — comment (summary for close)

**Alignment with corrected canon (S03/V1).**

- **Snapshot format v3:** 35-byte record; persisted set = identity, position block (incl. pos_age_s), Tail-1 quality, telemetry. Does **not** persist: last_seen_ms, last_seq, last_rx_rssi, last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16 (product_truth_s03_v1 §7; seq_ref_version_link_metrics_v0).
- **Restore:** Unpack sets those to 0/false; is_self from (node_id == self_node_id). last_seen_ms = 0 ⇒ restored entries start stale until first RX/TX.
- **Version:** Only v3 accepted; v1/v2 rejected (clean start).
- **Tests:** Round-trip, excluded fields, corrupt/old-version rejection, dirty flag — all in test_node_table_domain (20/20 pass).
- **Docs:** `docs/product/areas/nodetable/policy/nodetable_snapshot_format_v0.md` added; contract + classification in `_working/research/418_contract_and_classification.md`.

**Boundary vs #419:** Field-map alignment (uptime_10m, node_name, master table) remains in #419; this issue is snapshot schema and restore policy only.
