# #417 Alignment audit report — Persisted seq16 + NVS adapter

**Issue:** #417 — S03 P0: Persisted seq16 + NVS adapter (execution; links #296, #355)  
**Audit date:** 2026-03-11  
**Goal:** Strict alignment check under S03 canon / Product Truth. Outcome: close unchanged / text-fix only / reopen implementation.

---

## 1) #417 strict contract (extracted)

| Requirement | Source |
|-------------|--------|
| NVS adapter exists for seq16 | DoD / PR #428 |
| `restored_seq16` read on boot | Canon rx_semantics_v0 §5.3 |
| First TX after restore uses `restored_seq16 + 1` when `restored_seq16 > 0` | Canon §5.3 |
| Write policy documented and implemented | Per-TX after successful send |
| Unit tests cover restore + first-TX behavior | DoD |

---

## 2) Repo evidence (four yes/no questions)

| Question | Answer | Evidence |
|----------|--------|----------|
| **Is there a dedicated persistence adapter boundary?** | **Yes** | `firmware/src/platform/naviga_storage.h` (106–119): `load_seq16(uint16_t* out)`, `save_seq16(uint16_t value)`; `naviga_storage.cpp` (152–171): NVS namespace `"naviga"`, key `"seq16"`. Platform-only; no domain dependency on NVS. |
| **Is seq16 restored on boot?** | **Yes** | `firmware/src/app/app_services.cpp` (300–306): after `runtime_.init()`, `load_seq16(&restored_seq)`; if true and `restored_seq > 0`, `runtime_.set_initial_seq16(restored_seq)`. |
| **Does first TX after restore use restored_seq16 + 1?** | **Yes** | `BeaconLogic::set_initial_seq16(value)` sets `seq_ = value`; `next_seq16()` returns `++seq_`. So after restore 42, first packet gets 43. Tests: `test_seq16_set_initial_then_first_packet_is_initial_plus_one` (42→43), `test_seq16_set_initial_100_first_packet_is_101` (100→101). |
| **Is there a test proving that behavior?** | **Yes** | `firmware/test/test_beacon_logic/test_beacon_logic.cpp`: `test_seq16_default_first_packet_is_one`, `test_seq16_set_initial_then_first_packet_is_initial_plus_one`, `test_seq16_set_initial_100_first_packet_is_101`, `test_seq16_wraparound_first_packet_is_zero`. |

**Write policy (per-TX):** `app_services.cpp` (457–465): after `runtime_.tick()`, `get_last_sent_seq16(&sent_seq)`; if first time or `sent_seq != last_persisted_seq16_`, `save_seq16(sent_seq)`. `has_persisted_seq16_` ensures first sent value (including 0 after wraparound) is persisted. M1Runtime sets `last_sent_seq16_` / `has_last_sent_seq16_` from sent frame in `handle_tx()` (`m1_runtime.cpp` 266–272).

---

## 3) Canon fit at narrow #417 boundary

- **#417 does NOT depend on stale NodeTable ref-state:** Seq16 persistence is **sender-side** only: one uint16 in NVS, no NodeTable fields. Canon seq_ref_version_link_metrics_v0 and product_truth_s03_v1 state that last_core_seq16 / last_applied_tail_ref_core_seq16 are **not** NodeTable truth; #417 does not persist or expose them. No conflict.
- **Seq16 persistence is runtime/persistence behavior, not NodeTable field-map work:** Confirmed. Adapter is in platform (`naviga_storage`); domain has only `set_initial_seq16`; no NodeTable schema change.
- **Snapshot/restore mention:** #417 is **not** NodeTable snapshot. NodeTable snapshot is #418 (separate keys `nt_snap` / `nt_snap_len`). No scope bleed.

---

## 4) Requirement classification (#417 vs #418 vs #419 vs already done)

| Requirement | Owner | Status |
|-------------|-------|--------|
| NVS key "seq16", load/save in platform | #417 | **Done** — `naviga_storage` |
| Restore seq16 on boot, set_initial_seq16 when restored > 0 | #417 | **Done** — `app_services.cpp` init block |
| First TX uses restored + 1 | #417 | **Done** — BeaconLogic + tests |
| Per-TX write policy (after successful send) | #417 | **Done** — `app_services.cpp` tick + M1Runtime `get_last_sent_seq16` |
| Wraparound: seq16 0 valid, persisted | #417 | **Done** — `has_last_sent_seq16_` / `has_persisted_seq16_` |
| Unit tests: default 1, set_initial 42→43, 100→101, wraparound 65535→0 | #417 | **Done** — test_beacon_logic.cpp |
| NodeTable snapshot format + restore policy | #418 | Not #417 |
| NodeTable fields match canon field map | #419 | Not #417 |

---

## 5) Boundary vs #418 and #419

- **#418:** NodeTable snapshot blob (save/load `nt_snap`, `nt_snap_len`); restore in same init block **after** seq16 restore. Separate keys, separate format. #417 does not depend on #418.
- **#419:** NodeTable field set and master table alignment. No overlap with seq16 NVS key or sender counter.

---

## 6) Disposition

**Outcome: 1) CLOSE #417 unchanged** — scope/DoD/touched-paths/tests are canon-compatible; implementation is present and verified.

- No code changes required.
- No issue body edit required for canon alignment: #417 scope is sender seq16 only; implementation never touched NodeTable ref fields.
- If the issue body elsewhere contains legacy wording (e.g. “ref” or “Tail/Core” in a checklist), the close comment below clarifies scope.

---

## 7) Draft issue comment (for close)

**Suggested comment when closing #417:**

---

**Alignment audit (S03 canon).** Verified against codebase and canon:

- **Adapter:** `naviga_storage`: `load_seq16` / `save_seq16` (namespace `"naviga"`, key `"seq16"`). Platform-only; domain uses only `set_initial_seq16`.
- **Boot:** `app_services.cpp` restores seq16 after `runtime_.init()`; when restored > 0, `runtime_.set_initial_seq16(restored)`.
- **First TX:** First packet after restore uses `restored_seq16 + 1` (BeaconLogic + M1Runtime pass-through). Tests: default first packet 1; set_initial 42→43, 100→101; wraparound 65535→0.
- **Write:** Per-TX persistence after successful send; `get_last_sent_seq16` + `save_seq16` in tick; wraparound (seq16 0) persisted via validity flag.
- **Scope:** Sender seq16 only. No NodeTable ref fields (last_core_seq16 / last_applied_tail_ref_core_seq16); canon rx_semantics_v0 §5.3 and seq_ref_version_link_metrics_v0 satisfied.

Closing as implemented and canon-aligned. No further work for this issue.

---

## Exit criteria checklist

- [x] #417 requirements decomposed: still-valid, already-done in repo
- [x] Boundary vs #418 and #419 documented
- [x] Repo evidence collected (adapter, restore path, first-TX, tests)
- [x] Disposition: **close** (unchanged)
- [x] Close comment drafted with rationale
- [x] No implementation required; no spill into NodeTable canon redo
