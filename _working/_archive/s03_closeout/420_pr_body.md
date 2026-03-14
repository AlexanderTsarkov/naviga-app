# TX formation v0.1 canon alignment (#420)

## Summary
Locks current v0.1 TX formation behavior to the canon table in `packet_context_tx_rules_v0.md` §1: code and comments explicitly mirror trigger, earliest_at, deadline, coalesce, and replaceability. **No v0.2 packet migration** in this PR (Node_Pos_Full, Node_Status remain #422).

**Umbrella:** #416. **Boundary:** #421 = RX semantics; #422 = packetization / v0.2.

## Changes
- **beacon_logic.cpp** — Comment block above `update_tx_queue` mapping each branch to the v0.1 table; note on wire vs canon field names (e.g. uptime_sec vs uptime_10m); **hw_profile_id / fw_version_id** remain **uint16** (no width change).
- **beacon_logic.h** — Docstring for `update_tx_queue` updated to cite policy and list trigger conditions.
- **test_beacon_logic.cpp** — New test `test_txq_op_info_not_enqueued_before_cadence`: Op/Info not enqueued when `elapsed < min_interval` and `elapsed < max_silence` (cadence gate).
- **packet_context_tx_rules_v0.md** — One-line implementation reference to `beacon_logic.cpp` (update_tx_queue).

## Validation
- **`pio run -e devkit_e220_oled`** — **PASSED.**
- **New test** — `test_txq_op_info_not_enqueued_before_cadence` added and registered; validates cadence gate.
- **Full `test_beacon_logic` suite** — Execution on the devkit env is currently limited by existing test-infra: production build does not define `NAVIGA_TEST`, so `NodeTable::find_entry_for_test` is not compiled and other tests in the same file fail to build. Treated as **out of scope for #420**; no claim that all beacon_logic tests are green on this env.

## Boundary
- #421 (RX semantics): no change.
- #422 (packetization / v0.2): no change; v0.1 truth-lock only.

Closes #420.
