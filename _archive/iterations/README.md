# Archive — iterations

This folder holds **frozen snapshots** of iteration artifacts after phase closure. **Existing links and paths must stay valid:** do not move or delete originals in `_working/` to satisfy external references (e.g. issue comments, PR descriptions).

## Archiving rule

1. **Create** `_archive/iterations/<ITERATION_ID>/` when closing an iteration.  
   **Naming:** `<ITERATION_ID>` = first line of [\_working/ITERATION.md](../../_working/ITERATION.md) at closure time (e.g. `S01__2026-02__OOTB.v1_consolidation_cleanup`, `S02_2026-03-02`).

2. **Copy, do not move.** When preserving a snapshot, **copy** selected artifacts from `_working/` into the archive folder. Do **not** move or delete the originals in `_working/` — that would break existing links (e.g. from GitHub issues/PRs to `_working/hw_tests/...`).

3. **Stable references.** Prefer one of:
   - **Link from archive to `_working`:** In the archive README, link to the canonical location under `_working/` so readers use the live copy.
   - **Copy with note:** If you copy files into the archive, add a short “Copied from `_working/...` on &lt;date&gt;” note so the relationship is clear; the source in `_working/` remains the reference for existing links.

4. **No cleanup of history.** This policy does not require removing or “cleaning” files from `_working/` when archiving. Index + policy only.

## Existing iteration folders

- **S01__2026-02__OOTB.v1_consolidation_cleanup** — (existing)
- **S02_2026-03-02** — Seed snapshot index only; see [S02_2026-03-02/README.md](S02_2026-03-02/README.md) for links to key `_working/` evidence (no file copy in this PR).

## Related

- Current iteration workspace and evidence index: [\_working/README.md](../../_working/README.md).
- Iteration ID source of truth: [\_working/ITERATION.md](../../_working/ITERATION.md) (first line only).
