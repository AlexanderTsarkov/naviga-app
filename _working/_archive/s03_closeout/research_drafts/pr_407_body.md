## Summary

- Consolidate #407 WIP planning outcome for packet purpose / contents / creation / send rules.
- Record Node_Status lifecycle policy and explicit “no hitchhiking”.

## Scope

- Authoritative WIP snapshot in `docs/product/wip/areas/radio/policy/packet_context_tx_rules_v0_1.md`.
- Minimal supporting WIP alignment: pointers from packet_sets_v0_1, tx_priority_and_arbitration_v0_1, channel_utilization_budgeting (related/artifact tables only).
- No implementation, no canon, no protocol numbering.

## How to verify

- Read `docs/product/wip/areas/radio/policy/packet_context_tx_rules_v0_1.md`.
- Confirm Node_PosFull / Node_Status semantics are explicit.
- Confirm Node_Status lifecycle values: 30s / 300s.
- Confirm Pos_Quality is external to sibling issue track (#359).
- Confirm hitchhiking is explicitly not allowed.

## Risk / notes

- Docs-only; no behavior change.
- Current code baseline remains documented separately as baseline, not target.

## Docs

- Yes, WIP docs updated.

## Issue link

- Relates to #407 under #351.
- Do not use “Closes #407”; issue remains open until review/merge and any final follow-up status transition is decided.
