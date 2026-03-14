**PR:** [#446](https://github.com/AlexanderTsarkov/naviga-app/pull/446) — docs(#426): reconcile current_state with merged S03 execution

**Summary:**
- Updated `docs/product/current_state.md` to reflect merged S03 execution: v0.2 packet family (Node_Pos_Full, Node_Status, Alive), RX v0.2-only, and accurate iteration/What changed/Next focus.
- Removed/replaced stale v0.1 wording (packet types, TX queue, Core_Tail/Tail-1, Operational vs Informative).
- Added reconciliation note: merged implementation vs corrected Product Truth — no open mismatch for S03 scope.
- NodeTable field-origin audit: no remaining ambiguity; all canonical fields have a clear source; legacy ref fields documented as runtime-local only.
- No code changes; scope #426 only.

**#416:** After this PR is merged, the umbrella is ready for final closure/recap (post-merge step).
