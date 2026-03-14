# S03 closeout — _working classification and migration

**Date:** 2026-03-14  
**Purpose:** Archive S03-specific working artifacts and establish cleaner _working structure for S04.

## Summary

- **To _archive (s03_closeout):** All root-level S03 execution artifacts (reports, plans, issue/PR drafts, contract docs, post-merge plans), plus `docs_integrity/`, `docs_hygiene/`, `protocol_research/`. From `research/`: issue/PR drafting files (issue_351_*, pr_405_body, etc.) and S03-specific research that doubles as drafting.
- **Remain active in root:** `README.md`, `ITERATION.md` (unchanged; human updates when S04 starts).
- **New folders:** `_working/_archive/`, `_working/issues/body/`, `_working/issues/comments/`, `_working/pr/body/`, `_working/pr/comments/`, `_working/logs/`, `_working/tmp/`. Existing: `hw_tests/`, `research/` (kept; research notes stay, drafting moved to archive).

## Root-level file classification

| File / pattern | Classification | Destination |
|----------------|----------------|-------------|
| S03_* (all) | S03 historical | _archive/s03_closeout/ |
| 351_*, 361_* | S03 / legacy | _archive/s03_closeout/ |
| 416_* … 426_*, 435_*, 437_*, 438_*, 443_* | S03 reports/comments/plans | _archive/s03_closeout/ |
| 418_*, 419_*, 420_*, 421_*, 422_* (issue/pr/comment) | S03 issue/PR drafts | _archive/s03_closeout/issues_pr/ |
| 423_* … 426_*, 435_*, 438_*, 443_* (idem) | S03 issue/PR drafts | _archive/s03_closeout/issues_pr/ |
| 447_*, 451_*, 452_453_* | S03 HW/readiness | _archive/s03_closeout/ |
| audit_296_355_report, canon_promotion_*, product_truth_*, issue_realignment_* | S03 reports | _archive/s03_closeout/ |
| docs_post_s03_canon_cleanup_plan | S03 plan (done) | _archive/s03_closeout/ |
| issue_*_body*, user_profile_*, oled_*, issue_v02_*, issue_ootb_* | Issue drafts | _archive/s03_closeout/issues_pr/ |
| pr_*_body*, pr_body_workspace, oled_ms_seconds_pr_body | PR drafts | _archive/s03_closeout/issues_pr/ |
| v02_packet_redesign_design_note | S03 design note | _archive/s03_closeout/ |
| S02_289_evidence_procedure | S02 artifact | _archive/s03_closeout/ (legacy) |
| docs_integrity/, docs_hygiene/, protocol_research/ | S03 folders | _archive/s03_closeout/ (whole dirs) |
| README.md, ITERATION.md | Active | Root (README updated) |
| .DS_Store | Ignore | (unchanged / gitignore) |

## research/ subfolder

- **Stay in research/:** E220/airtime bench, persistence/nodetable reports, alignment/preflight, tail_instrumentation, hw317 findings, packetization/tx policy notes (technical research).
- **Move to _archive/s03_closeout/research_drafts/:** issue_351_*, issue_361_*, issue_405_*, issue_406_*, issue_407_*, pr_361_body, pr_405_body, pr_406_body, pr_407_body (draft text for issues/PRs).

## New structure (target)

```
_working/
  _archive/
    s03_closeout/          # All S03 artifacts (reports, issues_pr, research_drafts, docs_hygiene, protocol_research)
  hw_tests/                # (existing)
  logs/                    # (new, empty)
  research/                # (existing; research notes only after move)
  issues/
    body/
    comments/
  pr/
    body/
    comments/
  tmp/
  README.md
  ITERATION.md
```

## Ambiguous / left in place

- None; all root files classified as either archive or keep.
