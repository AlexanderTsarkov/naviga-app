## #422 — Packetization / canon-behavior transition (Path B)

Closes #422.

### Path and scope

- **#422 was executed via Path B** (v0.1 retained + canon-aligned behavior).
- **Current v0.1 packet family is retained:** Core (0x01), Alive (0x02), Tail1 (0x03), Operational (0x04), Informative (0x05). No new packet types, no wire format changes.
- **Canon-aligned behavior** is achieved via **throttle/merge rules** in TX formation:
  - No hitchhiking: Op/Info are not enqueued in the same formation pass as Core or Alive.
  - At most one status (Op or Info) enqueued per formation pass when status is due.
  - Status throttle: `min_status_interval_ms` = 30s, `T_status_max` = 300s; bootstrap allowance (≤2 sends).
  - M1Runtime calls `BeaconLogic::on_status_sent(now_ms)` when a TAIL2 or INFO frame is successfully sent, so lifecycle state is updated.
- **No v0.2 wire migration** in this issue (Node_Pos_Full, Node_Status are not introduced).
- **No RX semantic changes** in this issue; decoders and apply paths unchanged.
- **hw_profile_id / fw_version_id** remain uint16. Active-values plane and NodeTable/payload normalized naming unchanged.

### Validation

- **test_beacon_logic:** 73/73 passed (`pio test -e test_native -f test_beacon_logic`).
- **Devkit build:** `pio run -e devkit_e220_oled` passed.

### Changes

- `firmware/src/domain/beacon_logic.{h,cpp}`: status timing state, `on_status_sent`, no-hitchhiking and at-most-one-status-per-pass formation.
- `firmware/src/app/m1_runtime.cpp`: status timing init, `on_status_sent` on P3 send success.
- `firmware/test/test_beacon_logic/test_beacon_logic.cpp`: tests updated for Path B; new test `test_txq_422_status_throttle_min_interval_respected`.
- `docs/product/areas/radio/policy/packet_context_tx_rules_v0.md`: §3 note on #422 Path B implementation.
- `_working/422_contract_and_decision.md`, `_working/422_final_report.md`: decision record and final report.
