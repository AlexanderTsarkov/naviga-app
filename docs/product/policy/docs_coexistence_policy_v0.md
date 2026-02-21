# Docs coexistence policy v0

**Status:** Canon (process).  
**Scope:** Rules for legacy and OOTB docs coexisting with promoted V1-A canon; no “two truths”; migration is mechanical.

---

## 1) Purpose and scope

- **Purpose:** Define where legacy and OOTB docs live, how they are marked, how they reference canon (without copying normative text), and how to annotate OOTB divergence from canon. Single source of truth remains canon; legacy/OOTB are reference only.
- **Scope:** Header banners, referencing rules, deprecation/divergence annotation, migration guidance, spec_map meta. Does not move or edit existing legacy/OOTB content in this policy (moves/edits are separate PRs).

## 2) Non-goals

- Do not move or rename existing legacy/OOTB docs in this PR.
- Do not change semantics of product contract/policy docs.
- Do not create or populate `docs/legacy/` or `docs/ootb/` in this step; only define rules.

---

## 3) Definitions

| Term | Meaning |
|------|--------|
| **Canon** | Normative product spec; source of truth. Lives under `docs/product/areas/...` per [docs_layout_policy_v0.md](docs_layout_policy_v0.md). |
| **WIP** | Incubator; not normative until promoted. Lives under `docs/product/wip/`. |
| **Legacy** | Non-normative concept/history docs. Target path: `docs/legacy/`. Reference only; do not cite as truth. |
| **OOTB** | As-implemented v0 reference (defaults, current behaviour, UI). Target path: `docs/ootb/`. Not product truth; may diverge from canon. See [docs/dev/ai_model_policy.md](../../dev/ai_model_policy.md). |

---

## 4) Normativity rules

- **Canon is normative.** For specification and review, canon overrides legacy and OOTB. In case of conflict, canon wins.
- **WIP is tentative.** Not cited as implementation requirement until promoted.
- **Legacy and OOTB are non-normative.** Reference or “as-implemented” only. They may describe current or historical state but must not be treated as source of truth. If a legacy/OOTB doc conflicts with canon, it must explicitly defer to canon (see §5).

---

## 5) Header banner templates (copy/paste)

Place at the top of the doc, after the title line.

### Legacy banner

```markdown
**Status:** Non-normative (Legacy). Reference only. Canon: [link to promoted path if applicable].
```

### OOTB banner (as-implemented; may diverge from canon)

```markdown
**Status:** Non-normative (OOTB). As-implemented v0 reference. May diverge from canon. Canon: [link to promoted path if applicable].
```

### Redirect-stub banner (WIP doc after promotion)

```markdown
**Status:** Deprecated. Promoted to canon. Normative version: [link]. This stub is for redirects only.
```

---

## 6) Referencing rules

- **Link to canon; do not copy.** Legacy and OOTB docs may link to the promoted (canon) path. Do not copy normative paragraphs or tables from canon into legacy/OOTB — that would create two sources of truth.
- **If a legacy/OOTB doc conflicts with canon,** add an explicit deferral at the top or in a short “Relation to canon” section: “For normative behaviour see [canon link]. This doc is reference only and may be outdated.”
- One-line “Canon: [link]” in the header banner is sufficient when the doc has a corresponding promoted spec.

---

## 7) Deprecation / divergence annotation

### OOTB diverges from canon

When implementation or OOTB docs are known to diverge from canon (e.g. not yet migrated), mark it clearly so reviewers do not treat OOTB as truth:

- In the **OOTB doc header or intro:** “OOTB diverges from canon since &lt;date or PR&gt;. Normative spec: [canon link].”
- Optionally add a **Known divergences** section (see template below).

### Known divergences section template

```markdown
## Known divergences (OOTB vs canon)

| Area / behaviour | Canon (source of truth) | OOTB / current | Note |
|------------------|-------------------------|----------------|------|
| (e.g. field X)   | [link to canon]         | (brief note)   | Tracked in #issue |
```

Keep the table minimal; use it to avoid “two truths” and to drive migration issues.

---

## 8) Migration guidance (high-level)

- **When to move to legacy:** Doc is concept/history only; superseded by a promoted spec or no longer maintained as normative. Move to `docs/legacy/` and add Legacy banner + Canon link.
- **When to move to OOTB:** Doc describes current implementation/defaults/UI for reference. Move to `docs/ootb/` and add OOTB banner + Canon link; add “Known divergences” if needed.
- **See canon links:** Every legacy/OOTB doc that has a corresponding promoted spec should have a one-line “Canon: [link]” (in banner or §Relation to canon). No normative copy-paste.

---

## 9) Spec map integration

- Spec map tracks **product** (WIP and promoted) specs. Legacy and OOTB docs are **optional** in the Inventory: they may be listed in a separate “Legacy/OOTB references” table or as meta notes (path + “Non-normative; canon: [link]”) without changing product semantics.
- When adding a legacy/OOTB reference in spec_map, use meta only: path, status (Legacy/OOTB), optional link to canon. Do not use spec_map to define contract/policy semantics for legacy/OOTB.

---

## 10) Related

- Umbrella: [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- S02 Epic: [#224](https://github.com/AlexanderTsarkov/naviga-app/issues/224); S02.3: [#227](https://github.com/AlexanderTsarkov/naviga-app/issues/227)
- Promotion policy: [docs_promotion_policy_v0.md](docs_promotion_policy_v0.md)
- Layout policy: [docs_layout_policy_v0.md](docs_layout_policy_v0.md)
- Spec map: [docs/product/wip/spec_map_v0.md](../wip/spec_map_v0.md)
- AI model policy (OOTB not truth): [docs/dev/ai_model_policy.md](../../dev/ai_model_policy.md)
