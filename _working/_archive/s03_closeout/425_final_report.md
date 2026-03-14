# #425 Observability — final report

**Issue:** #425 · **Umbrella:** #416 · **Iteration:** S03__2026-03__Execution.v1

---

## 1) Contract (executed)

Add debug counters and structured observability so we can **validate traffic behavior** against traffic_model_v0 and packet_context_tx_rules_v0 (packet types sent/dropped, cadence, coalesce/skip/expire, RX accept/reject), **without changing protocol or profile semantics**.

---

## 2) Counters and logs added

### New: `domain::TrafficCounters` (`firmware/src/domain/traffic_counters.h`)

| Counter | Updated in | Meaning |
|---------|------------|---------|
| `tx_enqueue_pos_full`, `tx_enqueue_alive`, `tx_enqueue_status` | BeaconLogic `update_tx_queue` | Formation enqueued this type (once per enqueue_slot call for that type). |
| `tx_slot_replaced` | BeaconLogic `enqueue_slot` | Enqueue replaced an existing slot (coalesce). |
| `tx_starved` | BeaconLogic `dequeue_tx` | One slot dequeued; another present slot got starvation increment. |
| `tx_sent_pos_full`, `tx_sent_alive`, `tx_sent_status` | M1Runtime `handle_tx` (on send ok) | Packet of this type successfully sent. |
| `tx_drop_channel_busy`, `tx_drop_send_fail` | M1Runtime `handle_tx` (on drop) | Pending packet dropped: channel busy vs radio send failure. |
| `rx_ok_pos_full`, `rx_ok_alive`, `rx_ok_status` | M1Runtime `handle_rx` (updated) | RX accepted and applied for this type. |
| `rx_reject` | M1Runtime `handle_rx` (!updated) | RX frame rejected (unknown msg_type or decode/validate error). |

### API

- **Read:** `M1Runtime::traffic_counters()` → `const TrafficCounters&`
- **Reset:** `M1Runtime::reset_traffic_counters()` (e.g. before a bench run)
- **Wiring:** `BeaconLogic::set_traffic_counters(TrafficCounters*)` called from `M1Runtime::init()`

### Fixes (no protocol/profile change)

- **`packet_log_type_str`:** Added `POS_FULL` and `STATUS` so instrumentation lines show correct type names instead of "?".
- **`on_status_sent`:** Now invoked when `last_tx_type_ == PacketLogType::STATUS` (was TAIL2/INFO), so Node_Status lifecycle (min_status_interval, T_status_max, bootstrap) is correct.

---

## 3) Runtime questions now answerable

| # | Question | How it’s answered now |
|---|----------|------------------------|
| 1 | What packet types are sent and in what proportion? | `tx_sent_pos_full`, `tx_sent_alive`, `tx_sent_status`. |
| 2 | What types received and how many accepted vs rejected? | `rx_ok_*` by type; `rx_reject` for decode/validate failure. |
| 3 | How often are TX slots replaced vs starved? | `tx_slot_replaced`, `tx_starved`. |
| 4 | Is Status throttling / no-hitchhiking behaving? | Status enqueue/sent via `tx_enqueue_status`, `tx_sent_status`; `on_status_sent` fixed for STATUS. |
| 5 | How many TX drops and for what reason? | `tx_drop_channel_busy`, `tx_drop_send_fail`. |
| 6 | Cadence: enqueue vs send by type? | Compare `tx_enqueue_*` vs `tx_sent_*` (and `tx_slot_replaced`/`tx_starved` for queue behavior). |
| 7 | Only supported types on RX and how many rejected? | `rx_ok_*` for supported; `rx_reject` for unsupported/bad decode. |

---

## 4) Tests

- **Added:** `test_traffic_counters_enqueue_and_slot_replaced`, `test_traffic_counters_starved` in `test_beacon_logic.cpp` (61 tests total, all passing).
- **Existing:** All 59 existing beacon_logic tests still pass; devkit_e220_oled build succeeds.

---

## 5) Docs updated

- **`_working/425_contract_and_gaps.md`:** Gap table, minimal set, exit checklist; §6 “Where to read counters”.
- **`docs/dev/debug_playbook.md`:** One bullet under D) Bench — Traffic validation (#425), pointer to `traffic_counters()` and contract doc.

No change to current_state or product specs; #426 owns broader current_state.

---

## 6) What remains for #426 only

- Broader current_state / runtime behavior documentation.
- Any further observability (e.g. trigger-reason counters, boot/provisioning hooks) only if #426 or later work needs them.

---

## 7) Close recommendation

**Recommend closing #425 as implemented.**

- Minimal, sufficient counter set added and mapped to the 7 runtime questions.
- No protocol or profile semantics changed; no reopen of #417–#424 or #435/#438/#443.
- Counters are optional (BeaconLogic accepts null); no noisy logs; instrumentation strings and Status lifecycle callback fixed.
- Tests and devkit build green; debug playbook updated for traffic validation.

**Next:** PR from branch `issue/425-observability-traffic-counters` (or equivalent), then handoff for merge/close as per repo rules.
