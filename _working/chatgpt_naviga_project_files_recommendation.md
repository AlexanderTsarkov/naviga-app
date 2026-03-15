# Naviga — ChatGPT Project Files: upload recommendation

Use this to configure **Naviga** Project Files in ChatGPT. Phase and current-status docs stay in the repo and on GitHub; do not duplicate them as uploads.

---

## Files to upload (recommended set)

| File | Purpose |
|------|--------|
| `chatgpt_naviga_project_instructions.md` | Main operational instructions: source of truth, GitHub connector rule, model tier, reconnaissance, prompt discipline. |
| `chatgpt_naviga_project_architecture.md` | Stable architecture guardrails (M1Runtime, radio-agnostic domain, BLE, OOTB-first). |
| `chatgpt_naviga_project_debug_and_review.md` | Debug/review principles, triage, model guidance for CI/debug. |
| `chatgpt_naviga_project_files_recommendation.md` | This file: what to upload and what to remove. |

**Location in repo:** `_working/` (e.g. `_working/chatgpt_naviga_project_instructions.md`). Upload from there after review.

**Reason:** One minimal, stable set that gives ChatGPT the right operational context without duplicating product docs or phase-specific state. Current phase and focus are read from repo/GitHub, not from Project Files.

---

## Files to remove from Project Files (when adopting this set)

Remove **any** of the following if they are currently in Naviga Project Files:

- **Vision / product concept** docs (high-level vision, “what Naviga is” essays) — product truth lives in repo `docs/product/`; do not keep stale copies in Project Files.
- **OOTB / JOIN / Mesh** standalone concept or history docs — covered by architecture guardrails and repo canon; avoid duplicate or outdated summaries.
- **Product Core** or similar broad inventory docs — inventory and progress live in repo and GitHub; they are updated after merges and should not be duplicated here.
- **Phase-specific iteration summaries** (e.g. “current phase is S04”) — current phase is in `_working/ITERATION.md` and `docs/product/current_state.md`; re-uploading a phase snapshot each iteration is unnecessary and error-prone.
- **Old “instructions” or “workflow”** uploads that are superseded by the new `chatgpt_naviga_project_*` set — keep one coherent set to avoid conflicting guidance.

**Reason:** Reduces noise and staleness. Single source of truth for state = repo + GitHub. Project Files = stable operational guidance only.

---

## What stays out of Project Files

- **Phase/current-status docs** — remain in repo (`docs/product/current_state.md`, `_working/ITERATION.md`) and GitHub (Project, issues, PRs). User or connector supplies live state; do not embed “current phase” in an uploaded file that must be replaced every iteration.
- **Full product canon** — lives in `docs/product/areas/`; reference by path when needed, do not paste into Project Files.
- **Large historical/concept** docs — reference repo when needed; do not duplicate in Project Files.
