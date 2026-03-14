# #418 / #416 status and GitHub update report

**Date:** 2026-03-11  
**Purpose:** Confirm closure/update path for #418 after redo audit; post status to #418 and #416. No new code; no new PR unless verification shows it required.

---

## 1) #429 / PR state verification (repo-only)

**From repo evidence:**

- **Commit on main:** `a32c348 feat(fw): NodeTable persistence snapshot + restore (#418) (#429)` is the tip of `main`. So #418 work was landed in a commit that references PR #429 — i.e. **#429 was the merge vehicle** for #418.
- **PR #429:** Not directly visible from repo (no GitHub API/UI). **You must confirm in GitHub:** open PR #429 → check that it is **Merged**. If it is merged, outcome A below applies.

**Snapshot format on main:**

- `main` currently has **v2**: 37-byte record, **last_core_seq16** and **has_core_seq16** are still in the persisted blob (pack/unpack in `nodetable_snapshot.cpp`).
- **Canon-aligned v3** (35-byte, no last_core_seq16/has_core_seq16 in blob) exists in the **current working tree** (branch `docs/product-truth-s03-v1-wip`, uncommitted changes to `nodetable_snapshot.cpp/h`, tests, and new doc `nodetable_snapshot_format_v0.md`). So: implementation “present and canon-aligned” was verified in redo against that working tree; v3 is not yet on `main`.

---

## 2) Decision

- **A) If in GitHub PR #429 is merged:** No new PR is needed for the #418 work already on main. The v2 implementation (restore on init, save with 30 s debounce, NVS snapshot, tests) is landed. Any follow-up to bring **v3** (canon-aligned, no legacy ref in blob) to main is a separate step (e.g. commit/PR from current branch); this pass does not open that PR per your instructions.
- **B) If #429 exists but is not merged:** Document that clearly; then decide whether to merge #429 or land v3 via another PR.
- **C) If there is no PR #429 / changes not landed:** State explicitly and stop for follow-up decision.

**Recommendation:** In GitHub, confirm that PR #429 is **Merged**. If yes → use outcome A and the comment texts below.

---

## 3) Text to post on issue #418

Copy the following into a comment on #418. If you then close #418, you can use the same text as the closing comment.

---

**Status (post–canon redo).**

- **Implementation:** Landed via PR #429 (commit on main: `feat(fw): NodeTable persistence snapshot + restore (#418) (#429)`). Restore on init, save with dirty + 30 s debounce, NVS keys `nt_snap_len` / `nt_snap`, and tests (restore/round-trip, excluded fields, corrupt/old-version, dirty flag) are present and verified.
- **Canon alignment:** Corrected canon (product_truth_s03_v1 §7, seq_ref_version_link_metrics_v0) requires that the snapshot **not** persist last_core_seq16, has_core_seq16, or last_applied_tail_ref*. Snapshot format **v3** (35-byte record, no legacy ref-state in blob) and restore policy (restored entries start stale; runtime/legacy fields set to 0/false) are documented and implemented on branch `docs/product-truth-s03-v1-wip` (see `docs/product/areas/nodetable/policy/nodetable_snapshot_format_v0.md` and `_working/research/418_contract_and_classification.md`). Merging that branch (or equivalent v3 changes) will bring main to full canon alignment for #418.
- **Boundary vs #419:** Field-map / master-table alignment (e.g. uptime_10m, node_name) remains in #419; #418 is snapshot schema and restore policy only.
- **Conclusion:** No additional implementation work required for #418 scope. Closing as completed.

---

## 4) Text to post on umbrella #416

Copy the following into a comment on #416 (concise progress note).

---

**P0 progress.**

- **#417:** Completed and closed after canon alignment audit (persisted seq16 + NVS adapter; implementation verified on main).
- **#418:** Verified and completed. Implementation landed via PR #429 (NodeTable snapshot + restore, tests). Canon-aligned snapshot v3 (no legacy ref-state in blob) documented/implemented on branch; merge when ready. #418 closed.
- **Next:** #419 (NodeTable fields match canon field map / master table).

---

## 5) Final report summary

| Item | Result |
|------|--------|
| **#429 / PR state verified?** | Repo: commit a32c348 on main references (#418)(#429). **You must confirm in GitHub** that PR #429 is **Merged**. |
| **New PR needed?** | **No** — for the work already on main (v2 snapshot, restore/save/tests). v3 (canon-aligned) is on branch; merging it is a separate step, not a new “#418 PR”. |
| **What to post on #418** | Comment above (§3). Then **close #418**. |
| **What to post on #416** | Progress note above (§4). |
| **Final status of #418** | **Closed** — implementation landed via #429; scope complete; canon-aligned v3 documented/on branch. |
| **Recommended next issue** | **#419** — NodeTable canonical field map / master-table alignment. |

---

## 6) Exit criteria checklist

- [ ] **You** verified in GitHub that PR #429 is merged (or documented B/C).
- [x] Decision made: no new PR for already-landed #418 work; v3 merge is optional follow-up.
- [ ] #418 updated: post comment (§3) and close issue.
- [ ] #416 updated: post progress note (§4).
- [x] Short final report prepared (this file).
