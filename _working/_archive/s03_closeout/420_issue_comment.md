## #420 implementation complete

Implementation is packaged in **PR #432** (https://github.com/AlexanderTsarkov/naviga-app/pull/432).

**Scope:** Current v0.1 TX formation behavior alignment only. No v0.2 packet migration (Node_Pos_Full, Node_Status stay in #422).

**Changes:**
- Formation rules in code and comments explicitly mirror `packet_context_tx_rules_v0.md` §1 (trigger, earliest_at, deadline, coalesce, replaceability).
- New test: `test_txq_op_info_not_enqueued_before_cadence` (Op/Info not enqueued before cadence gate).
- Policy doc: one-line implementation reference. **hw_profile_id / fw_version_id** widths unchanged (uint16).

**Validation:**
- `pio run -e devkit_e220_oled` — passed.
- Full `test_beacon_logic` suite on devkit env is limited by existing test-infra (`NAVIGA_TEST` / `find_entry_for_test` not built in production); out of scope for #420. No overclaim on “all tests green.”

**Boundary:** #421 = RX semantics (unchanged). #422 = packetization / v0.2 (unchanged).

After merge of #432, this issue can be closed.
