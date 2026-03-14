# Design / truth-table PR for #435 (docs only)

**Issue:** [#435](https://github.com/AlexanderTsarkov/naviga-app/issues/435) — Post-P0: v0.2 packet redesign (Path A).

This PR establishes the **canonical v0.2 packet redesign** in docs only. No firmware or code behavior changes. Implementation and compatibility code follow in later PR(s).

## What this PR does

1. **Adds [packet_truth_table_v02](docs/product/areas/nodetable/policy/packet_truth_table_v02.md)** — Canonical v0.2 packet family (Node_Pos_Full, Node_Status, Alive), per-packet field composition, trigger/lifecycle, RX/apply consequences, product-vs-wire boundary, airtime/size expectations. Explicit note on what later implementation PR(s) must do and what the compatibility-cleanup PR will remove.

2. **Adds [packet_migration_v01_v02](docs/product/areas/nodetable/policy/packet_migration_v01_v02.md)** — Explicit compatibility policy: TX sends v0.2 only; RX accepts v0.1 + v0.2 during transition; cutover expectation; v0.1 is compatibility-only.

3. **Updates cross-links** — [packet_sets_v0](docs/product/areas/nodetable/policy/packet_sets_v0.md), [packet_context_tx_rules_v0](docs/product/areas/radio/policy/packet_context_tx_rules_v0.md), [seq_ref_version_link_metrics_v0](docs/product/areas/nodetable/policy/seq_ref_version_link_metrics_v0.md), and [NodeTable index](docs/product/areas/nodetable/index.md) now point to the v0.2 truth table and migration policy.

## Constraints preserved

- NodeTable remains normalized product truth (#419); no redefinition of canonical fields.
- active-values only; hw_profile_id / fw_version_id remain uint16; uptime_10m at canon layer.
- M1Runtime single composition point; domain remains radio-agnostic.
- No hybrid “old and new both canon” ambiguity.

## Out of scope (later PRs)

- TX/RX code changes; packet encoders/decoders; compatibility layer implementation.
- Reopening #417–#422.

Addresses #435 (design/truth-table scope only; implementation and cleanup remain in follow-up PRs).
