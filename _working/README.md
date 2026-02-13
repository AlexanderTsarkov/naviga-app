# Working — current implementation iteration

`_working/` is the workspace for the **current implementation iteration** and its verification artifacts. **Issue existence is not the classifier; Work Area is.** Only **Work Area = Implementation** (with Test artifacts allowed) uses the iteration concept; Product Specs WIP has no iteration and lives in `docs/product/wip/**`.

## ITERATION.md contract

- **First line** = Iteration ID (required, stable). This is the single source of truth for the current implementation iteration identifier. Parsers and archive paths use this line only.
- **Remaining lines** = optional metadata (recommended fields below). Human-editable; keep short.
- The iteration concept applies **only** to Work Area = Implementation and its Test artifacts. Product Specs WIP progresses in `docs/product/wip/` and does not use `_working` or ITERATION.md.

## Allowed in _working

- **Work Area = Implementation:** working notes, spike results, logs, investigations that support the current iteration. After merge, archive to `_archive/iterations/<ITERATION_ID>/` where `<ITERATION_ID>` is the **first line** of [ITERATION.md](ITERATION.md).
- **Work Area = Test:** test results, logs, measurements that support the current iteration.

Research outputs for the current iteration go under `_working/research/` (see CLAUDE.md Research artifacts policy).

## Not allowed in _working

- **Product Specs WIP** — belongs in [docs/product/wip/](../docs/product/wip/).
- **Docs** (general) — product and long-lived docs live under [docs/](../docs/); only docs strictly in-service of current-iteration verification may sit here.
- **Org** — repo/process/admin notes; keep out of `_working` and out of `docs/product` unless explicitly part of product documentation.

## Current iteration (template)

- **Iteration ID/Name:** *(read from [ITERATION.md](ITERATION.md); do not invent)*
- **Work Area:** Implementation (primary), Test (allowed)
- **Tech Area(s):** Firmware / Mobile App / Backend / Hardware / Protocol / Architecture
- **Related Issue(s)/PR(s):** *(optional)*
- **Goal / DoD / Status:** *(optional)*
