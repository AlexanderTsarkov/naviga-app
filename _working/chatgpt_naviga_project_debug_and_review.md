# Naviga — ChatGPT Project Files: debug and review discipline

Stable engineering discipline for reviews, CI failures, debug passes, and pre-merge work. See repo `docs/dev/debug_playbook.md` for full triage.

---

## Principles

- **Fix root cause**, then add or adjust focused tests so the issue cannot return silently.
- **Separate** device-only vs CI-only vs architecture issues. Do not mix fixes.
- **Avoid broad redesign during debug.** If a refactor is needed, split: (1) refactor-only PR, (2) bugfix PR.
- **No “debug by redesign.”** Minimal delta for the fix.

---

## Triage (symptom categories)

- **(A)** build/compile — toolchain, includes, config, bisect last change.
- **(B)** unit tests — intent vs implementation, time-dependent logic, mocks.
- **(C)** native E2E — wiring (M1Runtime), transport boundary, radio adapter contract, ordering/races.
- **(D)** device/bench — observability first (logging, traffic counters); config/wiring/params; clean-room repro.
- **(E)** CI-only — flakiness, ordering, platform differences; make tests deterministic.

---

## Model guidance for debug/review

- **Auto / efficiency-oriented:** Small packaging, comment-only fixes, obvious one-file corrections.
- **Stronger but cost-aware reasoning/code model:** Non-trivial CI/debug/review/investigation, multi-file impact, ambiguity. Do **not** normalize the most expensive top-tier models for routine debugging.

---

## Pre-merge

- PR: summary, scope, how to test, risk/notes, docs updated, issue link.
- Keep PRs small; CI green; no accidental re-enable of legacy components.
