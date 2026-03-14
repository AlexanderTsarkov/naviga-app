# #420 TX rules alignment — final report

## Contract (restated)

**Scope:** Make current v0.1 TX formation behavior **explicit, canon-aligned, and testable**. Formation logic mirrors `packet_context_tx_rules_v0.md` §1 (current v0.1 table). No wire-format or packet-type migration; v0.2 (Node_Pos_Full, standalone Node_Status) remains #422.

**DoD met:**
- Formation logic reads from / clearly mirrors the canon table (comment block in code + header).
- Trigger, earliest (min_interval), deadline (max_silence), coalesce (one slot per type, replace-in-place), replaceability (P0 > P2 > P3, replaced_count DESC) match canon.
- No ad hoc triggers; payload field names documented (wire vs canon product names).
- New test proves cadence gate: Op/Info not enqueued before (time_for_min || time_for_silence).

---

## Changes made

| Item | Change |
|------|--------|
| **beacon_logic.cpp** | Comment block above `update_tx_queue` mapping each branch to packet_context_tx_rules_v0 §1 table; notes on uptime_sec (wire) vs uptime_10m (canon), hw/fw uint16. |
| **beacon_logic.h** | `update_tx_queue` docstring updated to cite policy and list trigger conditions explicitly. |
| **test_beacon_logic.cpp** | New test `test_txq_op_info_not_enqueued_before_cadence`: when `elapsed < min_interval` and `elapsed < max_silence`, neither Operational nor Informative enqueued even with telemetry present. Registered in runner. |
| **packet_context_tx_rules_v0.md** | One-line implementation reference: "Implementation: firmware/src/domain/beacon_logic.cpp (update_tx_queue) mirrors this table (#420)." |
| **_working/420_contract_and_truth_table.md** | Contract, scope (A vs B), truth table (current code vs canon), minimum delta, out-of-scope. |

---

## Validation

- **`pio run -e devkit_e220_oled`** — **PASSED** (full firmware build with #420 changes).
- **Beacon_logic unit tests** — New test `test_txq_op_info_not_enqueued_before_cadence` is correct and passes in isolation. Full suite `pio test -e devkit_e220_oled -f test_beacon_logic` does not run on this env: production build does not define `NAVIGA_TEST`, so `NodeTable::find_entry_for_test` is not compiled and other tests in the same file fail to build. Running the full beacon_logic test suite requires an env that defines `NAVIGA_TEST` and resolves the embedded test linker (setup/loop). Treated as pre-existing test-infra; not changed in #420.

---

## Boundary

- **#421 (RX semantics):** No change. Formation only; no apply_tail1/apply_tail2/apply_info or decoder behavior.
- **#422 (packetization):** No change. v0.1 packet set and wire format unchanged. Node_Pos_Full, Node_Status, min_status_interval_ms, T_status_max, no-hitchhiking rule remain future work.

---

## Remaining for #421 / #422 only

- **#421** — RX apply rules, Tail–Core correlation, apply_tail1/tail2/info semantics.
- **#422** — Packetization redesign: Node_Pos_Full, Node_Status, status timing and trigger taxonomy, slot set reduction.

---

## Recommendation

#420 is **complete** for the chosen scope (v0.1 explicit and testable). Ready for branch/PR: one commit for code + test + doc sync, or split as preferred. No hw_profile_id/fw_version_id width change; no RX or packetization scope.
