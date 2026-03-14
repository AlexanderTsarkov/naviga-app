# #422 — Packetization / canon-behavior transition: final report

**Issue:** #422. **Iteration:** S03 (first line of _working/ITERATION.md).

---

## 1) Chosen path

**Path B (v0.1 retained + throttle/merge).**

- Current packet family unchanged: Core (0x01), Alive (0x02), Tail1 (0x03), Operational (0x04), Informative (0x05).
- Effective behavior aligned with canon via: status throttle (min_status_interval_ms 30s, T_status_max 300s), no hitchhiking, at most one status enqueue per formation pass, bootstrap (up to 2 sends), and status only when cadence is due (time_for_min || time_for_silence).

---

## 2) What became canonical after #422

- **TX formation (BeaconLogic):**
  - Op/Info are **not** enqueued in the same formation pass as Core or Alive (no hitchhiking).
  - Status (Op or Info) is enqueued at most once per formation pass when status is due; preference: Op if `has_battery||has_uptime`, else Info.
  - Status is only considered when cadence is due (`time_for_min || time_for_silence`), then subject to `min_status_interval_ms` (30s), `T_status_max` (300s), and bootstrap (≤2 sends).
  - When a P3 (Op or Info) is successfully sent, M1Runtime calls `BeaconLogic::on_status_sent(now_ms)` so lifecycle state (`last_status_tx_ms_`, `status_bootstrap_count_`) is updated.

- **NodeTable / payload names:** No wire or field renames. Master table already defines canon product name `uptime_10m` with storage as `uptime_sec`; implementation and docs are aligned.

- **hw_profile_id / fw_version_id:** Remain uint16; no change.

---

## 3) What compatibility remains

- **Wire:** Unchanged. All existing RX decoders and apply paths unchanged; no legacy/new packet coexistence logic.

- **Backward compatibility:** N/A for Path B (no new packet types). Old receivers continue to accept Core, Alive, Tail1, Op, Info as before.

---

## 4) What tests prove

- **test_txq_op_info_not_enqueued_before_cadence:** Op/Info gated by cadence (time_for_min || time_for_silence).
- **test_txq_422_status_throttle_min_interval_respected:** After two `on_status_sent` calls, a formation 1 ms later does not enqueue/replace status (min_status_interval respected).
- **test_txq_dequeue_core_before_operational / test_txq_dequeue_tail1_before_operational:** Two-pass formation (Op then Core+Tail1) so Op and Core both present; no hitchhiking.
- **test_txq_starvation_increments_replaced_count / test_txq_priority_ordering_p0_beats_all:** Same two-/three-pass pattern for no hitchhiking and at most one P3 per pass.
- **test_txq_p2_tail1_before_p3_operational_informative:** Op and Info enqueued in two separate passes; P2 before P3 ordering.
- **test_txq_empty_telemetry_no_operational_no_informative:** With allow_core=true, Core+Tail1 enqueued and Op/Info not (telemetry empty; no hitchhiking).
- All 73 beacon_logic tests pass; devkit_e220_oled build succeeds.

---

## 5) Docs/code alignment

- **packet_context_tx_rules_v0.md:** §3 updated with #422 Path B implementation note (throttle/merge, no hitchhiking, min_status_interval, T_status_max, bootstrap).
- **nodetable_master_field_table_v0.md:** Already states `uptime_10m` as canon name, `uptime_sec` storage; no change.
- **BeaconLogic / M1Runtime:** Comments reference #422 and packet_context_tx_rules_v0 §2a where relevant.

---

## 6) Follow-up after P0

- **Path A (v0.2) later:** If Node_Pos_Full + Node_Status are adopted, new encoders/decoders, msg_type or payloadVersion, and Pos_Quality 2-byte layout (§4 packet_context_tx_rules_v0) will be needed; Path B does not block that.
- **role_id on Informative:** packet_sets_v0 §3 lists role_id (new) for Informative; not in current wire. Add in a later change if product requires it.
- **T_status_max force send:** Logic forces enqueue when `(now - last_status_enqueue_ms_ >= T_status_max_ms_)`; actual send still depends on dequeue order and radio. No further change in #422.

---

## 7) Close recommendation

#422 is **complete** for the chosen scope (Path B: v0.1 + throttle/merge). Exit criteria are met; tests and devkit build are green; docs and code are aligned. Ready for branch/PR and issue closure.
