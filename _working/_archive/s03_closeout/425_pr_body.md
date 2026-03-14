## Summary

Adds **observability for traffic validation** only: structured `TrafficCounters` and instrumentation fixes so runtime behavior can be checked against traffic_model_v0 and packet_context_tx_rules_v0. **No protocol or profile semantics changed.** This is validation/debug support, not a protocol redesign.

Closes #425 (S03 P1/P2 observability). Umbrella: #416.

---

## Motivation: runtime questions we could not answer before

The work was driven by 7 concrete runtime questions that could not be answered confidently from existing logs/counters:

1. What packet types are **sent** and in what proportion?
2. What types are **received** and how many **accepted vs rejected**?
3. How often are TX slots **replaced** (coalesce) vs **starved**?
4. Is **Status throttling** (min_interval, T_status_max, no-hitchhiking) behaving?
5. How many TX packets are **dropped** and for what reason (channel busy vs send failure)?
6. **Cadence:** enqueue vs send by type?
7. Do we see only **supported packet types** on RX and how many **rejected**?

---

## What was added

### Structured counters: `domain::TrafficCounters` (`firmware/src/domain/traffic_counters.h`)

- **TX formation (BeaconLogic):** `tx_enqueue_pos_full`, `tx_enqueue_alive`, `tx_enqueue_status`; `tx_slot_replaced`; `tx_starved`.
- **TX outcome (M1Runtime):** `tx_sent_pos_full`, `tx_sent_alive`, `tx_sent_status`; `tx_drop_channel_busy`, `tx_drop_send_fail`.
- **RX (M1Runtime):** `rx_ok_pos_full`, `rx_ok_alive`, `rx_ok_status`; `rx_reject`.

**API:** `M1Runtime::traffic_counters()` (read), `M1Runtime::reset_traffic_counters()` (e.g. before a bench run). BeaconLogic receives an optional pointer via `set_traffic_counters()` from `M1Runtime::init()`.

### Instrumentation fixes (no behavior change to protocol/profile)

- **`packet_log_type_str`:** Now handles `POS_FULL` and `STATUS` so instrumentation logs show correct type names instead of "?".
- **`on_status_sent`:** Now triggered when `last_tx_type_ == STATUS` (was TAIL2/INFO), so Node_Status lifecycle (min_status_interval, T_status_max, bootstrap) is correct.

---

## Validation

- **`test_beacon_logic`:** 61 tests passed, including:
  - `test_traffic_counters_enqueue_and_slot_replaced`
  - `test_traffic_counters_starved`
- **devkit_e220_oled** build: success.

---

## Out of scope (unchanged)

- **No protocol or profile semantics changed.** No reopen of #417–#424 or #435/#438/#443.
- **#426** remains the place for broader current_state / runtime documentation. This PR only adds counters and the debug-playbook pointer.
