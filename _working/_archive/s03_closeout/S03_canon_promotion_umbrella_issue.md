# S03: WIP → Canon Promotion (planning slice to canon)

**Use:** Create the GitHub issue, then update #351 and add to Project #3.

```bash
# From repo root
gh issue create --title "S03: WIP → Canon Promotion (planning slice to canon)" --body-file _working/S03_canon_promotion_umbrella_issue_body.md
```

Then: (1) Add the new issue to **Project #3**, Status = **Ready**; (2) Edit **#351** and add the "Next: S03 WIP → Canon Promotion" section from `_working/S03_canon_promotion_subissues_and_351_update.md` (replace the link with the new issue URL). Optionally create sub-issues from the same file and add them to Project #3 Ready.

**Inventory:** Concrete "promote vs stay WIP" mapping is in `_working/S03_canon_promotion_inventory.md`.

---

## Title

**S03: WIP → Canon Promotion (planning slice to canon)**

---

## Body (copy below the line)

---

### Objective

Promote S03 WIP decisions to canonical product docs so that:

- Canon is the single source of truth for semantics and contracts.
- Canon is coherent and navigable (spec_map / architecture / current_state updated).
- Current firmware implementation is treated as **reference / previous implementation only**; semantics are defined by canon docs, not by code or OOTB/UI.

No code changes, no refactors, no firmware implementation in this phase.

---

### Scope (doc areas to promote)

| Area | Content |
|------|--------|
| **NodeTable** | Field map, packet→NodeTable mapping, packet sets, presence/age, identity/naming/persistence/eviction, seq/ref/version/link metrics, TX priority/arbitration; S03 slice index; RX semantics where not yet canon. |
| **Radio / packet** | Packet context and TX rules; traffic model; optional: tx power contract, radio profiles model, E220 mapping, channel-utilization (or keep as WIP with banner if not stable). |
| **Firmware policy** | Provisioning baseline, OOTB autonomous start; fwVersionId and hwProfileId **semantics** (registries already in canon); boot/provisioning links. |
| **Registries** | fwVersionId registry, hwProfileId registry — already in canon; ensure canon policy docs link to canon registry paths and that semantics docs are promoted or explicitly WIP. |
| **Mobile gate** | BLE snapshot S04 gate doc: confirm it remains **WIP gate**; link from canon (e.g. current_state or index) without defining BLE as specified. |

---

### Explicit policy note

- **Project is in development; no release.** Do not enforce backward compatibility or proliferate protocol versions. No release process or backward-compatibility guarantees in this phase.
- **Canon is truth; code is reference.** Current firmware (and OOTB/UI) are reference/previous implementation only. Domain and spec semantics are defined by canon docs. See `docs/dev/ai_model_policy.md` and `docs/product/policy/docs_promotion_policy_v0.md`.

---

### Promotion steps (checklist)

- [ ] **a) Inventory**  
  Enumerate S03 WIP docs and map each to:  
  - **Promote to canon** (move/merge to `docs/product/areas/...` per layout policy), OR  
  - **Keep in WIP** and add explicit **"WIP (not canon)"** banner + one-line reason (e.g. "Deferred until S04" / "Under review").

- [ ] **b) Canon edits**  
  For each promoted item: update/merge the canonical doc(s); add **Status: Canon** header; replace WIP-only links with canon paths; leave redirect stubs in WIP per `docs/product/policy/docs_layout_policy_v0.md`.

- [ ] **c) Cross-links**  
  Update `docs/product/wip/spec_map_v0.md` (and any architecture map / `current_state.md`) so canon is navigable and no canonical doc points to WIP for normative definitions.

- [ ] **d) Reference-impl disclaimer**  
  Add or confirm a short canonical note (e.g. in `docs/product/current_state.md` or a dedicated policy line):  
  *"Current firmware implementation is a reference/previous implementation; semantics are defined by canon docs."*

- [ ] **e) Final integrity pass**  
  Ensure no canonical doc depends on `_working/` or WIP-only paths for definitions. All normative semantics resolvable from canon + promoted paths only.

---

### Links

- **Master planning umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) — use its "Promotion to Canon" checklist as baseline; this issue is the **next phase** after S03 planning.
- **Key completed planning / issues:**  
  - NodeTable field map, packet mapping, presence semantics, traffic model, packet context/TX rules: [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352), [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407), [#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358), [#360](https://github.com/AlexanderTsarkov/naviga-app/issues/360).  
  - Provisioning baseline, OOTB autonomous start: [#385](https://github.com/AlexanderTsarkov/naviga-app/issues/385), [#354](https://github.com/AlexanderTsarkov/naviga-app/issues/354).  
  - fwVersionId + registry: [#405](https://github.com/AlexanderTsarkov/naviga-app/issues/405), [#409](https://github.com/AlexanderTsarkov/naviga-app/issues/409).  
  - hwProfileId + registry: [#406](https://github.com/AlexanderTsarkov/naviga-app/issues/406), [#410](https://github.com/AlexanderTsarkov/naviga-app/issues/410).  
  - BLE snapshot gate (S04): [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361).
- **Repo rules:** WIP under `docs/product/wip/**`; canon under `docs/product/areas/**`.  
  Policies: [docs_promotion_policy_v0.md](docs/product/policy/docs_promotion_policy_v0.md), [docs_layout_policy_v0.md](docs/product/policy/docs_layout_policy_v0.md).

---

### Non-goals (this phase)

- No code changes, no refactors, no firmware implementation.
- No BLE protocol spec (S04 gate stays as gate).
- No release process or backward-compatibility guarantees.
- Do not bump protocol/payload versions unless required by an actual wire-format change (none expected in this phase).
