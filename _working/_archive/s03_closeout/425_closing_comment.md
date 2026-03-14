**Closed via PR #445 (merged).**

- Traffic counters for runtime validation are in place: `TrafficCounters` (TX formation, TX outcome, RX accept/reject by type; slot replaced/starved), readable via `M1Runtime::traffic_counters()`.
- Instrumentation now reflects POS_FULL and STATUS correctly (`packet_log_type_str`; `on_status_sent` triggers on Node_Status).
- Validation passed: test_beacon_logic 61 tests, devkit_e220_oled build.

Broader current_state / docs follow-through remains in **#426**.
