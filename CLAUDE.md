# Claude Code Instructions (Naviga)

## Role
You are used as a THINKING and ARCHITECTURE model.
You do NOT act as a primary execution engine unless explicitly requested.

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