## Goal

Implement packet formation so that **trigger**, **earliest_at**, **deadline**, **coalesce**, **preemptible**, and **budget_class** (or equivalent) follow the canon TX rules table in [packet_context_tx_rules_v0](docs/product/areas/radio/policy/packet_context_tx_rules_v0.md). Current v0.1 table and agreed v0.2 table are the source of truth.

## Definition of done

- Formation logic reads from a single source of rules (or code clearly mirrors the canon table).
- For each packet type: trigger conditions, earliest send time, deadline (e.g. max_silence), coalesce behavior (one slot, snapshot replace where applicable), and replaceability under load match canon.
- No ad hoc triggers that contradict canon.

## Touched paths

- TX/formation path (e.g. `firmware/src/domain/beacon_logic.cpp`, scheduler or queue); M1Runtime wiring if formation is there.
- Canon: packet_context_tx_rules_v0, traffic_model_v0, field_cadence_v0.

## Tests

- Unit: given trigger state, correct packet type and payload are formed at the right time (or enqueued with correct rules).
- Deterministic simulation (if feasible): sequence of triggers yields expected sequence of packets per canon table.

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: C — Packetization v0.2 + TX scheduler rules (P0/P1).
