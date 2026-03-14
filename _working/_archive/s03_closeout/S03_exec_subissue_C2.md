## Goal

Implement packetization per canon: either (1) **redesign** to PosFull (Node_Pos_Full) + Node_Ops_Info (Node_Status) as in agreed v0.2, or (2) keep v0.1 but add **throttle/merge rules** so behavior matches canon. Canon [packet_context_tx_rules_v0](docs/product/areas/radio/policy/packet_context_tx_rules_v0.md) §2 defines Node_Pos_Full and Node_Status; §2a defines Node_Status lifecycle (bootstrap, triggers, steady-state, T_status_max).

## Definition of done

- If v0.2: Node_Pos_Full (position + 2B Pos_Quality) and Node_Status (full snapshot of operational + informative) implemented; encoding and TX semantics per canon.
- If v0.1 retained: throttle and merge rules added so that effective behavior aligns with canon (e.g. status interval, no hitchhiking).
- Node_Status lifecycle: bootstrap (up to 2 sends), urgent/threshold/hitchhiker triggers, T_status_max periodic refresh, per §2a.

## Touched paths

- Packet encoding (Core/Tail/Op/Info or PosFull/Status); formation/scheduler; payload builders.
- Canon: packet_context_tx_rules_v0, beacon_payload_encoding_v0, info_packet_encoding_v0, tail2_packet_encoding_v0 (if split retained).

## Tests

- Unit: PosFull/Status payload layout and content match contracts.
- Unit or integration: Node_Status bootstrap and T_status_max behavior (e.g. at least one Status within T_status_max).
- Deterministic simulation if possible.

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: C — Packetization v0.2 + TX scheduler rules (P0/P1).
