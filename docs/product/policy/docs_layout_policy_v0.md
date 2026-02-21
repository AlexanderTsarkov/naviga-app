# Docs layout policy v0

**Status:** Canon (process).  
**Scope:** Target folder structure and naming for product docs; promotion move mechanics and redirect stubs.

---

## 1) Purpose and scope

- **Purpose:** Define where normative (canon) vs incubator (WIP) vs non-normative (legacy, OOTB) docs live, so that promotion is mechanical: move + redirect stub + spec_map update. Align with [docs_promotion_policy_v0.md](docs_promotion_policy_v0.md).
- **Scope:** Folder layout and naming rules; redirect stub convention after promotion; how legacy/OOTB reference canon without becoming source of truth. Does not move existing docs in this policy (moves are separate PRs after layout is approved).

## 2) Non-goals

- Do not move or rename existing V1-A docs in this PR; that will be a separate series of PRs.
- Do not change protocol/policy semantics of domain specs.
- Do not create or populate `docs/legacy/` or `docs/ootb/` in this step; only define their role.

---

## 3) Four-layer structure

| Layer | Root path | Role | Normative? |
|-------|-----------|------|------------|
| **Canon** | `docs/product/` | Normative product specs; source of truth for implementation and review. | Yes |
| **WIP** | `docs/product/wip/` | Incubator: drafts, in-review, ready-to-promote. Not normative until promoted. | No |
| **Legacy** | `docs/legacy/` | Non-normative concept/history docs; superseded or pre-pipeline. Reference only. | No |
| **OOTB** | `docs/ootb/` | As-implemented v0 reference (defaults, current behaviour, UI). Not product truth. Reference only. | No |

- **Canon** overrides Legacy and OOTB when there is a conflict. Legacy and OOTB may link to canon for “current truth” but must not be cited as normative source. See [docs_promotion_policy_v0.md](docs_promotion_policy_v0.md) §6 and [docs/dev/ai_model_policy.md](../../dev/ai_model_policy.md).

---

## 4) Canon structure (target for promoted docs)

Promoted (canon) specs live under:

```
docs/product/
├── policy/           # Process docs (e.g. promotion, layout)
├── areas/
│   └── <area>/       # e.g. nodetable, radio, firmware, identity, registry, hardware, domain, session
│       ├── contract/ # Byte layout, APIs, formats
│       ├── policy/   # Rules, invariants, lifecycle
│       └── guides/   # Optional: how-to, cross-area
```

- **&lt;area&gt;** = single domain (nodetable, radio, firmware, identity, registry, hardware, domain, session). Add subdirs only when needed (e.g. no `guides/` until we have guide content).
- WIP equivalents today live under `docs/product/wip/areas/<area>/{contract,policy}/...`. Promotion move: copy content to `docs/product/areas/<area>/{contract,policy}/...` (or leave in place and reclassify per S02.3; this policy defines the target when we do move).

---

## 5) Naming rules

- **Versioned specs:** `*_v0.md`, `*_v1.md`, etc. Consistent suffix; no “random” names (e.g. avoid mixed `foo_v0.md` and `bar-draft.md` in the same area).
- **Contracts:** e.g. `beacon_payload_encoding_v0.md`, `registry_bundle_format_v0.md`.
- **Policies:** e.g. `field_cadence_v0.md`, `boot_pipeline_v0.md`.
- **Process docs:** e.g. `docs_promotion_policy_v0.md`, `docs_layout_policy_v0.md`.
- When promoting, keep the same basename unless there is a strong reason to rename (prefer no semantic renames in promotion-only PRs).

---

## 6) Header conventions

- **Normative docs** (canon): At top of doc, state **Status: Canon** or **Status: Canon (contract)** / **Canon (policy)**. Enables reviewers to treat the doc as source of truth.
- **Non-normative docs** (legacy, OOTB, or WIP): At top, state **Status: Non-normative** or **Status: WIP** and optionally **Reference only** or **As-implemented v0**. Ensures we do not treat OOTB or legacy as product truth.

---

## 7) Promotion mechanics (move + redirect stub + spec_map)

When a doc is **Promoted** (status WIP-Ready → Promoted):

1. **Move (or copy-then-deprecate):** Place canonical content at the **promoted path** under `docs/product/areas/<area>/{contract,policy}/<name>_v0.md`. If the doc currently lives under `docs/product/wip/areas/...`, either move the file or copy content and then replace WIP file with a redirect stub (see below).
2. **Redirect stub (WIP after promotion):** If the old path under `wip/` is kept, replace its content with a short stub containing:
   - A **Promoted-to** link to the canonical path: `docs/product/areas/<area>/...`.
   - A **Deprecation notice:** “This doc has been promoted to canon. The normative version is at [link]. This stub is kept for redirects only.”
   - No normative content in the stub.
3. **Spec map:** In `docs/product/wip/spec_map_v0.md`, set **Promotion status** to **Promoted** and set **Promoted path** to the canonical path (e.g. `docs/product/areas/nodetable/policy/field_cadence_v0.md`). See §8 below.

---

## 8) Spec map: Promoted path convention

- For each Inventory row with **Promotion status** = **Promoted**, the spec map records the **Promoted path**: the repo path to the canonical doc (e.g. `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md`).
- Convention: add a **Promoted path** column or a single note under the Inventory table: “When status = Promoted, canonical path is `docs/product/areas/<area>/{contract,policy}/<name>.md`; document in spec_map or in a per-row note.”
- This enables mechanical linking and avoids ambiguity about where the canon version lives after move.

---

## 9) Referencing canon from legacy and OOTB

- **Legacy** and **OOTB** docs may reference promoted (canon) docs by linking to the **Promoted path**. Such links are “see canonical spec” pointers only.
- Legacy/OOTB docs must **not** copy normative content from canon into themselves; they must not become a second source of truth. A one-line “Normative spec: [link]” or “Canon: [link]” at the top of a legacy/OOTB doc is sufficient.
- OOTB is not product truth: see [docs/dev/ai_model_policy.md](../../dev/ai_model_policy.md).

---

## 10) Related

- Umbrella: [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- S02 Epic: [#224](https://github.com/AlexanderTsarkov/naviga-app/issues/224); S02.2: [#226](https://github.com/AlexanderTsarkov/naviga-app/issues/226)
- Promotion policy: [docs_promotion_policy_v0.md](docs_promotion_policy_v0.md)
- Spec map: [docs/product/wip/spec_map_v0.md](../wip/spec_map_v0.md)
- AI model policy: [docs/dev/ai_model_policy.md](../../dev/ai_model_policy.md)
