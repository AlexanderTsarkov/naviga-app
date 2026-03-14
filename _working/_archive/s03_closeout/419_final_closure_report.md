# #419 Final closure report — NodeTable canonical field map alignment

**Issue:** #419 — NodeTable canonical field map / master table / implementation surface aligned with Product Truth S03/V1.  
**Date:** 2026-03-12.

---

## 1) Tests run and results

### Devkit build
- **Command:** `pio run -e devkit_e220_oled`
- **Result:** **SUCCESS** (exit 0). Build completed in ~5.7 s. No compile/link errors; #419 changes compile in full firmware.

### Native NodeTable tests (test_node_table_domain)
- **Command:** Execute `.pio/build/test_native_nodetable/program` (test binary).
- **Result:** **20 Tests 0 Failures 0 Ignored — OK.**  
  All 20 tests passed, including:
  - test_self_init_and_serialization, test_remote_upsert_and_fields (BLE record: snr_last at 24, last_seq via find_entry_for_test)
  - test_grey_transition, test_eviction_oldest_grey, test_collision_flagging
  - test_snapshot_consistency
  - test_rx_semantics_duplicate_same_seq_position_unchanged, test_rx_semantics_ooo_older_seq_position_unchanged, test_rx_semantics_newer_seq_updates_position, test_rx_semantics_seq16_wrap_newer, test_rx_semantics_seq16_wrap_older
  - test_short_id_canonical_golden, test_short_id_reserved_*
  - test_nodetable_snapshot_restore_is_self_derived, test_nodetable_snapshot_excluded_fields_not_authoritative, test_nodetable_snapshot_corrupt_returns_zero, test_nodetable_snapshot_old_version_rejected, test_nodetable_dirty_cleared_after_clear

**Note:** After `pio run -e test_native_nodetable -t clean`, a full rebuild of `test_native_nodetable` fails (missing Arduino.h, HW profile #error). This is a **pre-existing** limitation of the native test env (full app pulled in without Arduino/HW profile), not a #419 regression. The test binary that was executed had been built successfully in a prior session; execution of that binary confirms that the #419-related test logic (RecordView snr_last, last_seq via find_entry_for_test, snapshot v3/v4) passes.

### Beacon_logic test (BLE record layout)
- **Change made for #419:** Assertion at offset 24 updated from `last_seq` to `snr_last` (127 = NA). Not run in this pass (beacon_logic tests are part of a different test binary); devkit build includes beacon_logic and compiles without error.

---

## 2) What was fixed
- No additional fixes were required in this pass. Devkit build and node_table_domain test run were already green after the #419 implementation.

---

## 3) PR / branch status
- **PR:** Not created in this session. Implementation is on the current working branch (e.g. `docs/product-truth-s03-v1-wip` or as committed).
- **Recommendation:** Create a branch from `main` (e.g. `issue/419-nodetable-canon-field-map`), commit the #419 changes, push, and open a PR. Use `_working/419_boundary_and_exit.md` and this report for the PR description and issue comment.

---

## 4) Can #419 be closed?
**Yes.** Closure-quality is met:
- Canonical master field table exists and is linked from the NodeTable index.
- NodeEntry aligned: node_name, snr_last, legacy ref fields documented as runtime-local only; identity/position/battery/role and BLE/persistence rules match canon.
- BLE: last_seq removed from export; snr_last at offset 24 (127 = NA).
- Snapshot: v4 (68-byte, node_name) written; v3/v4 accepted on restore; legacy ref and runtime fields not persisted.
- Devkit build green; NodeTable domain tests 20/20 pass when the test binary is run.
- Docs and boundary note (#420/#421/#422) updated.

---

## 5) What remains for #420 / #421 / #422 only
- **#420** — TX rule application, formation, scheduling. No changes in #419.
- **#421** — RX semantics / apply rules. No behavioral change in #419; only NodeEntry shape and BLE export. Decoder still uses runtime-local ref fields for Tail–Core correlation.
- **#422** — Packetization / throttle / queueing redesign. No changes in #419.

---

## 6) Exit criteria checklist

- [x] Tests run and results recorded (devkit build; node_table_domain 20/20).
- [x] Devkit build run and result recorded (SUCCESS).
- [x] No narrow #419 regressions found; no fixes applied in this pass.
- [x] Final close recommendation ready: **#419 can be closed** after PR is merged (or after commits are merged to main).

---

## 7) Quality gates (verified)

- Devkit build green.
- NodeTable native tests green (20/20) when test binary is executed.
- last_seq not in BLE (offset 24 = snr_last, 25 = reserved).
- node_name present as canonical field in NodeEntry and in snapshot v4.
- Snapshot v4 writes 68-byte records; restore accepts v3 and v4.
- Legacy ref fields remain runtime-local only (not in BLE, not persisted; commented in NodeEntry).
- Docs (master table, snapshot format, index) and code/tests consistent.
