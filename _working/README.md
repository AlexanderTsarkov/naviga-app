# Working — current implementation iteration

`_working/` is the **volatile sandbox** for the **current implementation iteration** and its verification artifacts. When an iteration is closed, the archive snapshot/index lives under **`archive/iterations/<ITERATION_ID>/`** at repo root (canonical location for iteration snapshots; `/archive/` may be gitignored and thus local). **Issue existence is not the classifier; Work Area is.** Only **Work Area = Implementation** (with Test artifacts allowed) uses the iteration concept; Product Specs WIP has no iteration and lives in `docs/product/wip/**`.

## ITERATION.md contract

- **First line** = Iteration ID (required, stable). This is the single source of truth for the current implementation iteration identifier. Parsers and archive paths use this line only.
- **Remaining lines** = optional metadata (recommended fields below). Human-editable; keep short.
- The iteration concept applies **only** to Work Area = Implementation and its Test artifacts. Product Specs WIP progresses in `docs/product/wip/` and does not use `_working` or ITERATION.md.

## Allowed in _working

- **Work Area = Implementation:** working notes, spike results, logs, investigations that support the current iteration. On iteration close, archive snapshot/index to **`archive/iterations/<ITERATION_ID>/`** where `<ITERATION_ID>` is the **first line** of [ITERATION.md](ITERATION.md). (Canonical location; `_archive/iterations` is deprecated — see that folder’s README for back-compat.)
- **Work Area = Test:** test results, logs, measurements that support the current iteration.

Research outputs for the current iteration go under `_working/research/` (see CLAUDE.md Research artifacts policy).

## Not allowed in _working

- **Product Specs WIP** — belongs in [docs/product/wip/](../docs/product/wip/).
- **Docs** (general) — product and long-lived docs live under [docs/](../docs/); only docs strictly in-service of current-iteration verification may sit here.
- **Org** — repo/process/admin notes; keep out of `_working` and out of `docs/product` unless explicitly part of product documentation.

## How to find stuff

- **HW tests (bench evidence):** [hw_tests/](hw_tests/) — Serial captures, run scripts, and notes for S02 acceptance. Key subfolders (by date + issue): see Evidence index below.
- **Docs hygiene reports:** [docs_integrity/](docs_integrity/) (link-fix reports, e.g. #278 PR-1), [docs_hygiene/](docs_hygiene/) (legacy/OOTB inventory, e.g. #278 PR-2).
- **GH issue/PR body snapshots:** Root-level `gh_*.md`, `pr_*.md`, `epic_*.md`, `S02_*.md` — draft or snapshot text for issues/PRs; not authoritative.

## Evidence index (S02 bench & reports)

| date | area | path | closes / supported | note |
|------|------|------|--------------------|------|
| 2026-02-23 | HW smoke | [hw_tests/2026-02-23_s02_268c/](hw_tests/2026-02-23_s02_268c/) | #268, #270 | Symmetric RX; both devices see each other (NodeTable). |
| 2026-02-23 | HW smoke | [hw_tests/2026-02-23_s02_268b/](hw_tests/2026-02-23_s02_268b/) | #268 | Instrumentation verification post-PR #273. |
| 2026-02-23 | HW RF | [hw_tests/2026-02-23_s02_269/](hw_tests/2026-02-23_s02_269/), [hw_tests/2026-02-23_s02_269b/](hw_tests/2026-02-23_s02_269b/) | #269 | RF session capture & analysis; role cadence. |
| 2026-02-23 | HW | [hw_tests/2026-02-23_s02_281/](hw_tests/2026-02-23_s02_281/), [hw_tests/2026-02-23_s02_281b_det_gnss/](hw_tests/2026-02-23_s02_281b_det_gnss/) | #281, #288, #277 | Role-derived TX cadence; deterministic GNSS scenarios. |
| 2026-02-23 | HW | [hw_tests/2026-02-23_s02_288/](hw_tests/2026-02-23_s02_288/) | #288 | GNSS scenario emulator v0 bench. |
| 2026-02-25 | HW | [hw_tests/2026-02-25_hw_sanity_post_framing/](hw_tests/2026-02-25_hw_sanity_post_framing/), [hw_tests/2026-02-25_protocol_regression_post_framing/](hw_tests/2026-02-25_protocol_regression_post_framing/) | — | Post-framing sanity & protocol regression. |
| — | Docs | [docs_integrity/278_pr1_report.md](docs_integrity/278_pr1_report.md) | #278 PR-1 | Stale link fixes (spec_map §4). |
| — | Docs | [docs_hygiene/278_pr2_inventory.md](docs_hygiene/278_pr2_inventory.md) | #278 PR-2 | Legacy/OOTB banner inventory. |

Other hw_tests subfolders: `2026-02-23_s02_266` (provision/reboot), `2026-02-23_s02_267` (OOTB baseline), `2026-02-23_s02_268` (radio smoke), `2026-02-23_s02_276_tail_proof` (tail proof). See each folder’s `notes.md` for issue links.

## Current iteration

- **Iteration ID/Name:** S03__2026-03__Execution.v1 (from [ITERATION.md](ITERATION.md))
- **Work Area:** Implementation (primary), Test (allowed)
- **Tech Area(s):** Firmware, Protocol, Radio, Persistence, Testing
- **Execution umbrella:** [#416](https://github.com/AlexanderTsarkov/naviga-app/issues/416) — S03 Execution planning: implement canon slice (code+tests plan)
- **P0 issues:** #417, #418, #419, #420, #421, #422
- **Referenced trackers:** #296, #355
- **Goal / DoD:** Implement canon slice per #416; P0 complete + CI green + bench sanity evidence captured

### How we work in S03 execution

- **Keep here:** logs, bench evidence, investigation notes, and other artifacts that support the current implementation/test iteration.
- **Product specs stay in** [docs/product/](../docs/product/) and [docs/product/wip/](../docs/product/wip/); do not add WIP product specs to `_working/`.
- **Execution entry point:** #416 (not the planning umbrella #351). Work branches and PRs should link to #416 and the relevant P0 sub-issue.
