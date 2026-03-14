# Issue #407 — planning outcome update (draft)

**Post this as a comment on [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407) to record the planning outcome. Still WIP under [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351); not canon or implementation.**

---

**S03 Planning outcome (packetization v0.2):** Consensus captured in **docs/product/wip/areas/radio/policy/packet_context_tx_rules_v0_1.md**.

**Agreed direction:**
- **Position path:** **Node_Pos_Full** = Core position + 2-byte Pos_Quality (one packet, one slot, one seq16 per update). Replaces current Core_Pos + Core_Tail split. Split Core+Tail is no longer the default; retained only as a possible fallback/calibration hypothesis if needed.
- **Status path:** **Node_Status** = single packet with full snapshot of: batteryPercent, battery_Est_Rem_Time, TX_power_Ch_throttle, uptime10m, role_id, maxSilence10s, hwProfileId, fwVersionId. Trigger taxonomy (agreed): **urgent triggers** — TX_power_Ch_throttle, maxSilence10s, role_id (urgent-config); **threshold triggers** — batteryPercent, battery_Est_Rem_Time; **hitchhiker-only** — uptime10m, hwProfileId, fwVersionId. Anti-burst pacing (min_status_interval_ms), bounded periodic refresh (T_status_max), repeated quick changes merge into pending/current snapshot (one slot).

**Baseline:** Current code (five slots, single cadence gate, Core+Tail coupled, Op+Info same gate) remains the reference; v0.2 is the agreed planning target, not yet implemented.

**Open (for later):** Pos_Quality 2-byte bit layout (external [#359], not a blocker for #407 closure). Lifecycle, bootstrap, timing, and hitchhiking are agreed and recorded in WIP §2a; protocol numbers are out of scope for #407 semantic closure (later contract/wire follow-up).

**Follow-up WIP:** packet_sets_v0_1, tx_priority_and_arbitration_v0_1, channel_utilization_budgeting doc — add pointer to consolidated WIP / v0.2 direction when convenient; no removal of v0.1 content until v0.2 is adopted in canon.

*(Comment posted to #407; see issue thread.)*

---

**Addendum — Node_Status lifecycle policy fixed in WIP:** The agreed Node_Status lifecycle (scope, Initial Status Population / bootstrap, trigger taxonomy, steady-state, timing) is now recorded in **docs/product/wip/areas/radio/policy/packet_context_tx_rules_v0_1.md** §2a. Concrete values: `min_status_interval_ms = 30s`, `T_status_max = 300s`. Bootstrap: `status_bootstrap_pending` on boot; up to 2 sends; second not earlier than min_status_interval_ms; then steady-state. **Hitchhiking closed — not allowed** (Node_Status standalone; no same-pass or piggyback with PosFull). Protocol numbers out of scope for #407 closure. Remaining: Pos_Quality as external dependency [#359] only; #407 semantically ready for final review and docs-only PR prep.
