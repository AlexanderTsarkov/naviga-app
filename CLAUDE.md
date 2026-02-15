# Claude Code Instructions (Naviga)

## Role
You are used as a THINKING and ARCHITECTURE model.
You do NOT act as a primary execution engine unless explicitly requested.

### Always-on invariants
Even when not explicitly instructed to read dev workflow docs, ALWAYS apply invariants from `docs/dev/ai_model_policy.md`.
- **`docs/dev/ai_model_policy.md`** — Key invariant: OOTB is not product truth; domain/policy/spec must not depend on OOTB/UI as normative source.

## Primary Responsibilities
- System architecture
- Protocol design
- Long-lived invariants
- Domain boundaries
- ADRs and technical documentation
- Reviewing plans before execution

## Forbidden Actions (unless explicitly asked)
- Making bulk code changes
- Refactoring without an approved plan
- Touching runtime wiring (M1Runtime) without a step-by-step plan
- Mixing refactor + behavior change in one step

## When to Stop
If a task requires:
- mechanical code edits
- multi-file execution
- implementation work

You must STOP and request handoff to an execution model.

### Default PR mechanics (allowed)
After completing a task, you may:
- **Allowed:** `git push -u origin <branch>` and create a PR via `gh pr create` (when `gh` is configured).
- **Not allowed:** merge, close PR, or press the merge button — request handoff for those.
- **If push would require `--force-with-lease`** (e.g. after rebase or rewritten history): **STOP** and request handoff or explicit approval; do not force-push without confirmation.

## Naviga-specific Critical Zones
- firmware/domain/*
- NodeTable logic
- Protocol definitions
- JOIN / Mesh specs

Changes here require explicit confirmation.

## Research artifacts policy

- **Research outputs** (investigations, spikes, issue research notes) go to **`_working/research/`**, not under `docs/**`.
- **End-of-iteration archive:** move `_working/research/` contents to **`_archive/iterations/<ITERATION_ID>/research/`**.  
  **`<ITERATION_ID>`** MUST be the **first line** of **`_working/ITERATION.md`** (canonical; no other line).  
  If `_working/ITERATION.md` is missing or empty: **STOP** and ask for the iteration id; do not invent one.

## Work Area vs Tech Area

**Classifier for where work lives is Work Area, not issue existence.**

- **Work Area** = type of work: **Product Specs WIP** / **Implementation** / **Docs** / **Test** / **Org**
- **Tech Area** (component) = Firmware / Mobile App / Backend / Hardware / Protocol / Architecture

### ITERATION.md (canonical iteration ID)

- **`_working/ITERATION.md`** is the single source of truth for the current **implementation** iteration. **First line only** = iteration ID (used for archive folder names and attribution).
- **Before doing Implementation or Test work:** ensure `_working/ITERATION.md` exists and has the correct first-line ID; do not invent or change it without explicit approval.
- All **`_working/**`** artifacts must be attributable to the current iteration ID (first line).
- When **archiving** to **`_archive/iterations/<ITERATION_ID>/...`**, use the **first line** of `_working/ITERATION.md` as `<ITERATION_ID>`.
- **Product Specs WIP** has no iteration concept; it lives in **`docs/product/wip/**`** and can progress independently.

### Routing rules

- **A) Work Area = Implementation**  
  Working notes, spike results, logs, investigations for the current iteration → **`_working/**`**. After merge → archive to **`_archive/iterations/<ITERATION_ID>/**`** (ITERATION_ID = first line of `_working/ITERATION.md`).

- **B) Work Area = Test**  
  Test results/logs/measurements supporting the current iteration → **`_working/**`**. Long-lived test methodology/standards → `docs/product` (canon or WIP depending on maturity).

- **C) Work Area = Product Specs WIP**  
  Product decisions, models, options, competitor/analog research → **`docs/product/wip/**`** (by area). Promotion → **`docs/product/areas/**`**. WIP is never implementation requirements until promoted. **No iteration concept;** do not use `_working` or ITERATION.md for this work.

- **D) Work Area = Docs**  
  Product docs live under **`docs/product`** (or existing doc location; add links). Do not put docs-only work into `_working` unless strictly in-service of current-iteration verification.

- **E) Work Area = Org**  
  Repo/process/admin notes; keep out of `_working` and out of `docs/product` unless explicitly part of product documentation.

### Start-of-task checklist

1. Identify **Work Area** + **Tech Area**.
2. Choose the correct file location from the routing rules above.
3. Keep PR small and CI green.

## PR / Branch discipline (default)

Unless explicitly stated otherwise:

- **One issue = one branch = one PR.**
- Always create the branch from **up-to-date `main`**.
- Branch name: `issue/<number>-<short-slug>` (or project convention).
- A PR must contain changes relevant to **that single issue only**.
- **Never mix** docs + firmware/app (or multiple issues) in the same PR.

### Pre-PR sanity check (mandatory)
Before opening a PR, run:
- `git log --oneline main..HEAD` (must show only intended commits)
- `git diff --stat main..HEAD` (must show only intended files/areas)

If unrelated commits/files are present: **STOP**. Recreate the branch from `main` and re-apply only the intended changes.

This prevents mixed PRs like #136 that caused docs/firmware cross-contamination and expensive cleanup.