# Naviga — AI workflow (Cursor + ChatGPT)

This document defines how we work on Naviga with Cursor and ChatGPT.
Goal: keep PRs small, CI green, and context transferable between chats.

## Source of truth

- GitHub repo: AlexanderTsarkov/naviga-app
- GitHub Project board: Project #3
- Issues/PRs are the planning + audit trail.
- "Inventory / progress" lives in the repo and is updated after merges (do NOT duplicate it in ChatGPT Project Files).

## Default development loop

1. Pick an Issue from the Project board (or create one).
2. Create a branch from `main`:
   - `feat/<issue-id>-short-name` or `fix/<issue-id>-short-name` or `docs/<short-name>`
3. Implement minimal scoped change.
4. Run local checks (as applicable).
5. Open PR:
   - link Issue (e.g., "Closes #NN")
   - include test evidence / how to verify
   - list docs/inventory updates (if any)
6. Keep CI green.
7. Merge.
8. Post-merge hygiene:
   - close/transition issue
   - update inventory/progress docs if needed
   - add bench notes if behavior changed

## Definition of Done (DoD)

A change is "done" when:

- expected behavior works (or spec updated)
- tests updated/added where it makes sense
- CI is green
- docs updated if user-visible / architectural / operational
- no legacy components are accidentally re-enabled

## Cursor prompt contract (what every prompt MUST include)

When ChatGPT writes a prompt for Cursor, it must contain:

1. **Model recommendation**
   - Auto OR specific model (e.g., GPT-5.2 Codex) + 1-line justification.

2. **Goal**
   - What to implement/change, in one paragraph.

3. **Non-goals**
   - Explicitly list what NOT to do (scope guardrails).

4. **Context pointers**
   - Issue number + PR references (if relevant)
   - target paths / files
   - runtime wiring points (e.g., M1Runtime)
   - test locations

5. **Step plan**
   - 3–7 steps, ordered.
   - small commits preferred.

6. **Quality gates**
   - unit/integration tests to run
   - CI expectation
   - docs/inventory update requirement (if touched areas demand it)

7. **Exit criteria**
   - concrete checklist of "ready to merge".

## Model selection guide (Cursor)

Use **Auto** when:

- small local changes in 1–3 files
- docs edits
- straightforward test additions
- mechanical refactors (rename, move) with low risk

Use **a stronger selected model (e.g., GPT-5.2 Codex)** when:

- multi-file refactor across modules
- runtime wiring changes (M1Runtime, transports, interfaces)
- flaky tests / CI investigation
- protocol / encoding changes
- anything that can silently break integration behavior

Rule of thumb:

- If a mistake costs <15 minutes: Auto.
- If a mistake costs hours (or hides until field test): pick the stronger model.

## PR template (recommended)

- Summary: what + why
- Scope: what is included/excluded
- How to test: commands or bench steps
- Risk/notes: potential regressions
- Docs: updated? (yes/no, links)
- Issue link: Closes #NN

## Naviga constraints (do not violate)

- E220 UART is treated as a MODEM (adapter implements IRadio).
- SPI driver is out of scope unless explicitly planned.
- Real CAD/LBT is out of scope unless explicitly planned.
- Domain (NodeTable/BeaconLogic) must stay radio-agnostic.
- Legacy BLE service must remain disabled unless explicitly reintroduced by plan.
