## Goal

Add **debug counters and/or logs** needed to validate traffic model assumptions (e.g. packets sent per type per interval, coalesce events, skip/expire) without bloating production logs. Support verification that real behavior matches [traffic_model_v0](docs/product/areas/radio/policy/traffic_model_v0.md) and packet_context_tx_rules_v0.

## Definition of done

- Counters or structured log points for: packet type sent, trigger reason, coalesce/skip/expire.
- Minimal overhead (e.g. compile-time or runtime guard, or dev-only build).
- Documented where to read them (serial, or future export).

## Touched paths

- Formation/TX path; optional small observability module or debug header.
- Canon: traffic_model_v0, packet_context_tx_rules_v0.

## Tests

- Unit or manual: enable observability, run scenario, confirm counters/logs reflect expected behavior (e.g. one Core per position commit, Status within T_status_max).

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: E — Observability & sanity (P1/P2).
