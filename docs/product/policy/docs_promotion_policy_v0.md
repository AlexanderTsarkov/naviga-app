# Docs promotion policy v0

**Status:** Canon (process).  
**Scope:** Product spec docs lifecycle and source-of-truth rules for Naviga repo.

---

## 1) Purpose and scope

- **Purpose:** Define how product specification documents move from draft/WIP to canonical (normative) state, and how the spec map reflects promotion status. Ensure a single source of truth for product/domain decisions.
- **Scope:** Applies to docs under `docs/product/` (including `docs/product/wip/areas/`). Process and change-control rules; does not define target folder layout (see S02.2) or physical file moves.

## 2) Non-goals

- Do not define protocol or policy semantics of domain specs (NodeTable, radio, firmware, etc.).
- Do not move or rename existing docs in this policy; layout and moves are separate work.
- Do not treat OOTB or UI as normative source (see §5 and `docs/dev/ai_model_policy.md`).

---

## 3) Definitions

| Term | Meaning |
|------|--------|
| **WIP** | Work-in-progress: doc lives under `docs/product/wip/` (e.g. `wip/areas/<area>/`). May be incomplete or under review. Not normative for implementation until promoted. |
| **Canon** | Canonical: doc is the normative source of truth for its scope. Policy and implementation must align to canon; canon overrides legacy and as-implemented behaviour when stated. |
| **Legacy** | Older or idea-level docs (e.g. in `docs/product/` or `docs/firmware/`) that predate or sit alongside the WIP→Canon pipeline. Not source of truth unless explicitly promoted to canon. |
| **OOTB** | Out-of-the-box (defaults, current implementation, or UI behaviour). **OOTB is not product truth.** It may be used as an example or an indicator of current state only when explicitly marked non-normative. Domain and spec docs must not depend on OOTB/UI as normative source. See `docs/dev/ai_model_policy.md`. |

---

## 4) Promotion pipeline

States for a spec in the pipeline:

1. **Idea** — Early or stub; no stable contract yet. May be issue-only or a short concept doc.
2. **Candidate** — Doc exists; under review or has open dependencies. Not yet ready for implementation as normative source.
3. **Ready (Ready-to-implement)** — Doc is complete for its scope; decisions captured; ready to be treated as normative when implementation aligns. Still under WIP path until promoted.
4. **Promoted** — Doc has been elevated to canon (normative). Location may stay under WIP or move to canonical path per layout policy (S02.2). Spec map marks it as canonical.

Transition criteria (short): Idea → Candidate when a doc exists and is linked in spec map; Candidate → Ready when owners agree and dependencies are satisfied; Ready → Promoted when layout/move rules are applied and spec map is updated. Change control: small PRs, no semantic changes in promotion-only PRs.

---

## 5) Change control and small-PR discipline

- Promotion and spec_map updates: prefer small, reviewable PRs. One or few docs per PR; avoid mixing promotion with semantic edits.
- When updating promotion status in the spec map, do not change contract/policy semantics in the same commit.
- Who decides: maintainers / doc owners; spec map is the index of record. Disputes on “Ready” vs “Candidate” are resolved in issues (e.g. umbrella #147).

---

## 6) Coexistence rules (legacy and OOTB vs canon)

- **Canon is normative.** For any area covered by a promoted (canon) doc, that doc overrides legacy docs and OOTB behaviour for the purposes of specification and review.
- **Legacy and OOTB** are reference or “as-implemented” only. They may be used to inform defaults or examples only when explicitly marked non-normative. Implementation may temporarily deviate from canon during migration; such deviations must be tracked (e.g. in issues), not justified by OOTB as truth.
- Coexistence plan (detailed) is in scope of S02.3; this section states the invariant only.

---

## 7) Spec map integration

- The **spec map** (`docs/product/wip/spec_map_v0.md`) is the index of product specs and their promotion status.
- **Promotion status** for each V1-A–relevant entry is marked in the Inventory table. Convention:
  - **Promotion status** values: **WIP-Idea** | **WIP-Candidate** | **WIP-Ready** | **Promoted** | **Deprecated**.
  - Existing “Promote” column (Idea / Candidate / Ready) maps to WIP-Idea / WIP-Candidate / WIP-Ready. “Promoted” is used when the doc is elevated to canon. “Deprecated” when superseded and no longer normative.
- The spec map links to this policy so that status semantics are defined in one place. Changelog entries reference the policy and issues (#224, #225) when promotion status convention is introduced or updated.

---

## 8) Related

- Umbrella: [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- S02 Epic: [#224](https://github.com/AlexanderTsarkov/naviga-app/issues/224); S02.1: [#225](https://github.com/AlexanderTsarkov/naviga-app/issues/225)
- AI model policy (OOTB not truth): [docs/dev/ai_model_policy.md](../../dev/ai_model_policy.md)
- Spec map: [docs/product/wip/spec_map_v0.md](../wip/spec_map_v0.md)
