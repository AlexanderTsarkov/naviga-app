**Packaging complete.** PR: #445 — [feat(#425): add traffic counters for runtime validation](https://github.com/AlexanderTsarkov/naviga-app/pull/445)

**What changed**
- Added `domain::TrafficCounters` (TX formation, TX outcome, RX accept/reject by type; slot replaced/starved).
- Wired via `BeaconLogic::set_traffic_counters()` from `M1Runtime::init()`; read/reset via `M1Runtime::traffic_counters()` / `reset_traffic_counters()`.
- Instrumentation: `packet_log_type_str` now includes POS_FULL and STATUS; `on_status_sent` now triggers on Node_Status (STATUS) for lifecycle.
- Tests: `test_traffic_counters_enqueue_and_slot_replaced`, `test_traffic_counters_starved`.
- Debug playbook: one bullet under D) Bench pointing at traffic counters.

No protocol or profile semantics changed.

**Validation**
- `test_beacon_logic`: 61 tests passed (including the two new counter tests).
- `devkit_e220_oled` build: success.

**Remains for #426 only**
- Broader current_state / runtime behavior documentation.
- Any further observability (e.g. trigger-reason counters) only if #426 or later work needs it.
