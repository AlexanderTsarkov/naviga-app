# Post-merge checkbox audit — #435 and #438

## Summary

Checklist audit completed for **#435** and **#438** against merged PRs (#436, #437, #440) and repo state. All checkboxes verified; confirmed-done items ticked in issue bodies; audit comments and umbrella update posted.

---

## #435 (v0.2 packet redesign — design + implementation)

| # | DoD item | Evidence | Status |
|---|----------|----------|--------|
| 1 | v0.2 truth table | #436: packet_truth_table_v02.md, packet_migration_v01_v02.md, packet_sets_v0 | DONE |
| 2 | Explicit compatibility policy | #436: packet_migration_v01_v02.md (TX v0.2, RX v0.1+v0.2, cutover) | DONE |
| 3 | TX formation | #437: Node_Pos_Full, Node_Status, Alive; encoders; lifecycle | DONE |
| 4 | RX decode/apply | #437: v0.2 dispatch + v0.1 compat layer; apply_pos_full, apply_status | DONE |
| 5 | Tests | #436, #437: beacon_logic, ootb e2e, PosFull/Status apply | DONE |
| 6 | Devkit build | #437: devkit build passing | DONE |
| 7 | Docs/inventory | #436, #437: packet sets, migration, seq_ref | DONE |

- **Result:** 7/7 DONE. No items moved to #438 (cutover removal was always follow-up scope). No remaining undone.
- **Issue body:** All 7 checkboxes set to `[x]`.
- **Comment:** [435#issuecomment-4045986289](https://github.com/AlexanderTsarkov/naviga-app/issues/435#issuecomment-4045986289)

---

## #438 (final v0.2-only cutover)

| # | DoD item | Evidence | Status |
|---|----------|----------|--------|
| 1 | v0.1 RX compatibility removed | #440: decode_header rejects 0x01,0x03,0x04,0x05; BeaconLogic branches removed | DONE |
| 2 | Obsolete compat/apply remnants removed | #440: apply_tail1/2/info, last_applied_tail_ref* removed; upsert_remote Alive fix | DONE |
| 3 | Runtime-local leftovers reviewed | #440: Tail ref removed; last_core_seq16 retained for apply_pos_full | DONE |
| 4 | Tests updated and passing | #440: v0.2-only tests; test_native_nodetable pass | DONE |
| 5 | Devkit build passing | #440: devkit_e22_oled_gnss SUCCESS | DONE |
| 6 | Docs/inventory updated | #440: packet_migration_v01_v02, packet_sets_v0 cutover complete | DONE |
| 7 | #435 follow-up linkage | #438 is documented final cutover; #435 link in place | DONE |

- **Result:** 7/7 DONE. No remaining undone.
- **Issue body:** All 7 checkboxes set to `[x]`.
- **Comment:** [438#issuecomment-4045987486](https://github.com/AlexanderTsarkov/naviga-app/issues/438#issuecomment-4045987486)

---

## #416 (umbrella)

- **Update posted:** [416#issuecomment-4045987679](https://github.com/AlexanderTsarkov/naviga-app/issues/416#issuecomment-4045987679)
- **Content:** #435 and #438 checklists reconciled; final protocol state (TX/RX v0.2-only); no remaining follow-ups for this scope; BLE linker out of scope.

---

## Exit criteria

- [x] #435 checklist audited item by item
- [x] #435 boxes updated (7/7 ticked)
- [x] #435 audit comment posted
- [x] #438 checklist audited item by item
- [x] #438 boxes updated (7/7 ticked)
- [x] #438 audit comment posted
- [x] #416 umbrella update posted
- [x] Short final report prepared

**Remaining undone items:** None. Both issues are in a disciplined, audit-friendly state; every checked box has merged evidence behind it.
