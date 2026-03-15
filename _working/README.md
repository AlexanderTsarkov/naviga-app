# Working — current implementation iteration

`_working/` is the **active work area** for the **current implementation iteration** and its verification artifacts: research notes, hardware test evidence, logs, and temporary issue/PR drafting. It is **not** for canon or WIP product truth — that lives in [docs/product/](../docs/product/) and [docs/product/wip/](../docs/product/wip/).

When an iteration is closed, working artifacts are moved into **`_working/archive/`** (see Working archive convention below) so the root stays minimal and the next phase can start clean.

**Issue existence is not the classifier; Work Area is.** Only Work Area = Implementation (with Test allowed) uses the iteration concept. Product Specs WIP has no iteration and lives in `docs/product/wip/`.

---

## ITERATION.md contract

- **First line** = Iteration ID (required, stable). Single source of truth for the current implementation iteration. Parsers and archive paths use this line only.
- **Remaining lines** = optional metadata. Human-editable; keep short.
- Update [ITERATION.md](ITERATION.md) when starting a new iteration; move closed-iteration artifacts into `_working/archive/` before resetting.

---

## Working archive convention

Active phase materials stay in `_working/` (root and subfolders such as research/, issues/, pr/).

Archived phase materials move to **`_working/archive/`**.

**Recommended archive shape:**

- `_working/archive/iterations/<iteration-tag>/...`

If older archives exist in another location (e.g. `_archive/iterations/` at repo root), treat `_working/archive/` as the standard going forward. Migrate older materials only in dedicated housekeeping passes.

---

## First-pass task discipline

Unless a task is trivially straightforward, start with reconnaissance:

- Inspect relevant current code.
- Inspect nearby legacy code/docs when relevant.
- Inspect canon/policy/docs that constrain the change.

If reconnaissance shows the task is straightforward, continue.

If it reveals contradictions, branching options, or meaningful ambiguity, stop and write a report for decision instead of selecting a path implicitly.

---

## Folder layout

| Folder | Purpose |
|--------|---------|
| **[archive/](archive/)** | Closed-phase snapshots. Use `archive/iterations/<iteration-tag>/...`. Do not use for active work. |
| **[hw_tests/](hw_tests/)** | Hardware test runs: serial captures, run scripts, notes. One subfolder per run (e.g. `2026-02-23_s02_281/`). |
| **[logs/](logs/)** | Raw serial dumps, build logs, one-off log files. Keep separate from summarized results/evidence. |
| **[research/](research/)** | Research notes, investigations, spike results. Not issue/PR body text — use `issues/` and `pr/` for drafts. |
| **[issues/body/](issues/)** | Draft text for GitHub issue **bodies**. |
| **[issues/comments/](issues/)** | Draft text for GitHub issue **comments**. |
| **[pr/body/](pr/)** | Draft text for GitHub PR **descriptions**. |
| **[pr/comments/](pr/)** | Draft text for GitHub PR **comments**. |
| **[tmp/](tmp/)** | One-off scratch files, paste buffers, transient notes. |

**Do not** leave issue/PR body or comment drafts in the root. Put them in the folders above.

---

## Naming conventions (temp issue/PR files)

Use consistent names so drafts are easy to find and don’t clutter the root:

- **Issue body:** `issue_<number>_body.md` (e.g. `issue_452_body.md`)
- **Issue comment:** `issue_<number>_comment_<purpose>.md` (e.g. `issue_452_comment_closeout.md`)
- **PR body:** `pr_<number>_body.md` (e.g. `pr_457_body.md`)
- **PR comment:** `pr_<number>_comment_<purpose>.md` (e.g. `pr_457_comment_followup.md`)

These files are **not** authoritative; the canonical content is on GitHub.

---

## Allowed in _working

- **Work Area = Implementation:** working notes, spike results, logs, investigations for the **current** iteration.
- **Work Area = Test:** test results, logs, measurements for the current iteration.
- Research outputs → [research/](research/). On iteration close, move iteration-specific research to [archive/](archive/) (e.g. `archive/iterations/<iteration-tag>/research/`) if needed.

---

## Not allowed in _working

- **Product Specs WIP** — [docs/product/wip/](../docs/product/wip/).
- **Canon / long-lived product docs** — [docs/](../docs/), [docs/product/](../docs/product/). Only docs strictly in-service of current-iteration verification may sit here.
- **Org / process** — repo/admin notes stay out of `_working` unless explicitly part of product documentation.

---

## Closed-phase hygiene

When an iteration (e.g. S05) is closed:

1. Move phase-specific artifacts from root and subfolders into **`_working/archive/iterations/<iteration-tag>/`** (iteration-tag = first line of ITERATION.md).
2. Keep only: README, ITERATION.md, and the folder structure. Optionally keep a short classification/migration note inside the archive folder.
3. Update ITERATION.md when starting the next phase.

This keeps the root small and ready for the next phase without losing history.

---

## Current state

- **Iteration:** See first line of [ITERATION.md](ITERATION.md). (S05 active: Android mobile foundation on BLE.)
- **Mobile app WIP:** [s05_android_mobile_app_wip.md](s05_android_mobile_app_wip.md).
- **Evidence index:** See [hw_tests/](hw_tests/) subfolders; each run typically has a `notes.md` with issue links.
