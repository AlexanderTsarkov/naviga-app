# #425 Observability — contract, gaps, minimal set

**Iteration:** S03__2026-03__Execution.v1 (first line of `_working/ITERATION.md`)  
**Issue:** #425 · **Umbrella:** #416

---

## 1) #425 contract (restated)

**Goal:** Add debug counters and/or structured log points so we can **validate traffic behavior** against traffic_model_v0 and packet_context_tx_rules_v0 (packet types sent/dropped, cadence, coalesce/skip/expire, RX accept/reject) **without changing protocol or profile semantics**.

**In scope:** TX/RX/runtime traffic observability for the **current canonical runtime** (v0.2 slots: PosFull, Alive, Status). Counters/signals must answer concrete validation questions; minimal overhead; no noisy logs.

**Out of scope:** No protocol/packet-format changes; no profile/cadence semantics changes; no reopen of #417–#424, #435/#438/#443; no BLE/UI; no generic logging overhaul; no broad current_state doc rewrite (#426).

---

## 2) Runtime questions we cannot answer today (with evidence)

| # | Question | Why we can't answer today | Evidence (file: lines) |
|---|----------|---------------------------|-------------------------|
| 1 | What packet types are being **sent** and in what proportion? | Only total `tx_count`; no breakdown by type. | `m1_runtime.cpp`: `stats_.tx_count++` (267); `last_tx_type_` used only for logs and status callback, not aggregated. |
| 2 | What packet types are **received** and how many are **accepted vs rejected**? | `rx_count` is total; no per-type RX counts; no reject count. | `m1_runtime.cpp`: `stats_.rx_count++` (324); DECODE_OK/DECODE_ERR logged but not counted (355–359). |
| 3 | How often are TX slots **replaced** (coalesce) vs **starved**? | `replaced_count` is per-slot internal state; no global “replace” or “starved” counters. | `beacon_logic.cpp`: `enqueue_slot` increments `slot.replaced_count` when `slot.present` (55–56); dequeue starvation loop (306–309); no global counters. |
| 4 | Is **Status throttling** (min_interval, T_status_max, no-hitchhiking) behaving as expected? | Status lifecycle state is internal to BeaconLogic; we don’t see “Status enqueued vs sent” or bootstrap. | `beacon_logic.cpp`: `last_status_tx_ms_`, `status_bootstrap_count_` (217–234) not exposed; no enqueue-vs-sent by type. |
| 5 | How many TX packets are **dropped** and for what reason (channel busy vs send failure)? | Only instrumentation lines; no counters. | `m1_runtime.cpp`: “pkt drop … CHANNEL_BUSY” / “SEND_FAIL” (244–254, 297–307); no `stats_` fields. |
| 6 | **Cadence:** Are we enqueueing at the expected rate (e.g. one PosFull per position commit, Status within T_status_max)? | No enqueue counters; cannot compare “N enqueued” vs “M sent” by type. | `beacon_logic.cpp`: `update_tx_queue` has no counters; formation is opaque. |
| 7 | Do we see **only supported packet types** on RX and how many are **rejected** (unknown type / decode error)? | No per-type RX accept counts; no reject counter. | `beacon_logic.cpp`: `on_rx` returns false for bad header/decode (345–346, 358–359, etc.); `m1_runtime.cpp` only logs DECODE_ERR. |

---

## 3) Observability gap table

| Need | Current | Gap |
|------|---------|-----|
| TX sent by type | `tx_count` only | No `tx_sent_pos_full`, `tx_sent_alive`, `tx_sent_status`. |
| TX dropped by reason | Instrumentation log only | No `tx_drop_channel_busy`, `tx_drop_send_fail`. |
| TX enqueue / coalesce / starve | Per-slot `replaced_count` (internal) | No global `tx_enqueue_*`, `tx_slot_replaced`, `tx_starved`. |
| RX accepted by type | None | No `rx_ok_pos_full`, `rx_ok_alive`, `rx_ok_status`. |
| RX rejected | DECODE_ERR log only | No `rx_reject` count. |
| Status lifecycle | Internal to BeaconLogic | No exposed enqueue/sent for Status; fix `on_status_sent` to use STATUS type. |
| Instrumentation strings | `packet_log_type_str` | Missing POS_FULL, STATUS → logs show "?". |

---

## 4) Minimal observability set for #425

All of the following map to the 7 questions above.

- **TX (M1Runtime after send/drop):**  
  `tx_sent_pos_full`, `tx_sent_alive`, `tx_sent_status`;  
  `tx_drop_channel_busy`, `tx_drop_send_fail`.
- **TX formation (BeaconLogic → counters):**  
  `tx_enqueue_pos_full`, `tx_enqueue_alive`, `tx_enqueue_status`;  
  `tx_slot_replaced` (when enqueue replaces existing slot);  
  `tx_starved` (when dequeue adds starvation to other slots).
- **RX (M1Runtime after on_rx):**  
  `rx_ok_pos_full`, `rx_ok_alive`, `rx_ok_status`;  
  `rx_reject` (decode/validate failed).
- **Fixes (no semantic change):**  
  - `packet_log_type_str`: add POS_FULL, STATUS.  
  - `on_status_sent`: call when `last_tx_type_ == STATUS` (not TAIL2/INFO).

**Storage:** One struct `TrafficCounters` in domain, owned and exposed by M1Runtime; BeaconLogic receives optional pointer and increments enqueue/replace/starved.

**No:** trigger-reason counters, boot/provisioning hooks (unless needed later), extra log lines beyond existing instrumentation.

---

## 5) Exit criteria checklist

- [x] #425 contract restated
- [x] 5–7 unanswered runtime questions documented
- [x] Observability gap table produced
- [x] Minimal counter set implemented
- [x] Tests added/updated and passing
- [x] Devkit build passing
- [x] Docs synchronized where needed (see §6)
- [x] Final close recommendation prepared (see _working/425_final_report.md)

## 6) Where to read counters

- **Runtime:** `M1Runtime::traffic_counters()` returns `const domain::TrafficCounters&`. Call from app after `tick()` (e.g. serial dump, BLE export, or bench harness).
- **Reset:** `M1Runtime::reset_traffic_counters()` zeros all fields (e.g. before a bench run).
- **Instrumentation logs:** Existing `set_instrumentation_logger()` lines now use correct type strings for POS_FULL and STATUS; Node_Status send correctly triggers `BeaconLogic::on_status_sent()`.
