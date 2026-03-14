# docs(#426): reconcile current_state with merged S03 execution

Closes #426. Umbrella: #416.

## Summary

Updates `docs/product/current_state.md` so it reflects the **actual merged S03 execution outcome** (#416): v0.2 packet family, reconciled implementation vs corrected Product Truth, and accurate Next focus. No code changes.

## What changed

- **Stale content removed/replaced:** v0.1 packet types (Core_Pos, Core_Tail, Operational, Informative) and TX-queue wording were replaced with the canonical v0.2 family (Node_Pos_Full, Node_Status, Alive) and v0.2-only RX. Domain/NodeTable section no longer describes Core_Tail ref_core_seq16 linkage or Tail-1 apply; it now states v0.2 semantics and that legacy ref fields are runtime-local decoder only. Next focus no longer points to completed work (persisted seq16, snapshot semantics, or S03 planning as primary).
- **current_state now reflects merged S03 execution:** Iteration tag set to S03 execution (post–#416). Added one “What changed” row for S03 execution (P0 #417–#422, #423–#425, #435, #438, #443). Firmware section notes OOTB autonomous start (#423) and user profile baseline (#443) as landed.
- **Implementation vs corrected Product Truth reconciled:** A one-line note was added under Source of truth: merged S03 implementation is reconciled with corrected Product Truth; no open mismatch for S03 scope.
- **NodeTable field-origin audit:** Performed; no remaining ambiguity. All canonical NodeTable fields have a clear source (radio, profile, runtime, derived, or runtime-local decoder only); legacy ref fields are explicitly not in BLE or persistence.
- **No code changes.** Scope is docs only; no reopening of #417–#425, #435, or #438.

## #416

After this PR is merged, #416 is ready for final closure/recap (post-merge step).
