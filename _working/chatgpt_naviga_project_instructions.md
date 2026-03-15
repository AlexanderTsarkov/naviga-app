# Naviga — ChatGPT Project Files: operational instructions

**Purpose:** Stable operational guidance for ChatGPT when working on the Naviga project. These Project Files are **not** the live source of project state; they tell you where to look and how to behave.

---

## What these files are

- **Stable context** for reasoning about Naviga: architecture, workflow, and review/debug discipline.
- **Not** product documentation (that lives in the repo under `docs/product/`).
- **Not** phase snapshots: current iteration and focus live in the repo and on GitHub; do not duplicate them here or replace this file every phase.

---

## Source of truth

- **Current project state, current phase, active work:** GitHub repo (AlexanderTsarkov/naviga-app), GitHub Project board, open issues/PRs, and repo docs:
  - `docs/product/current_state.md` — product/iteration summary and next focus
  - `_working/ITERATION.md` — current implementation iteration (first line = iteration tag)
  - `_working/` WIP and research artifacts for the active phase
- **ChatGPT Project Files** = operational context only. They do **not** override GitHub or repo docs for live status. Inventory and progress live in the repo and are updated after merges; do not duplicate them in Project Files.

---

## When GitHub cannot be read

If you cannot read the GitHub repo or project state:

- **Do not assume** GitHub is unavailable in principle. Often the **connector has expired** or needs refresh.
- **Tell the user** to refresh or reconnect the GitHub connector.
- **Do not compensate** by inventing or inferring project state from stale summaries, old uploads, or indirect clues.

---

## Copy-paste output rule

Whenever you produce text that is **explicitly meant to be copied into another app or tool** — for example Cursor prompts, shell commands, git commands, commit messages, PR descriptions, issue comments, or config snippets — that portion **must** be emitted as a **copy-paste-ready snippet or block**, not as ordinary surrounding prose.

This requirement applies **only** to content intended for transfer into another application; the rest of the dialogue can remain normal conversational explanation.

---

## Model selection (Cursor prompts)

- **Auto or efficiency-oriented default:** Small housekeeping, packaging, comments, doc touch-ups, simple follow-ups, narrow edits. Use when a mistake costs little time.
- **Stronger reasoning/code model:** Multi-file implementation, protocol/runtime work, CI/debug investigation, ambiguity-heavy review, conflict resolution, repo-sensitive analysis. Use when a mistake would cost hours or hide until field test.
- Phrase this as **recommended model tier**, not one fixed model name. Stay **cost-aware**: do **not** recommend the most expensive frontier/top-tier models as the routine default for Naviga. Use stronger models when justified; keep routine work on an appropriate tier.

---

## First-pass reconnaissance (non-trivial tasks)

- **Non-trivial tasks** must begin with a **reconnaissance phase** before implementation.
- **Inspect:** relevant current code; nearby legacy code when relevant; docs/contracts/policies that constrain the task.
- **If straightforward** (no contradictions, no branching paths): continue in the same task.
- **If ambiguities, contradictions, or multiple plausible approaches:** stop and produce a short report for explicit user decision. Do **not** choose a path autonomously when repo/docs support more than one.

---

## Prompt discipline for Cursor

- **Small, reviewable slices.** One issue → one branch → one PR where possible.
- **Issue/PR grounded.** Link goals to issues; list docs/updates in PR.
- **Scope narrow.** Non-goals and guardrails in the prompt.
- **Phase-specific state** (current iteration, current focus) should be **read from repo/GitHub**, not embedded in Project Files as a per-phase upload.
