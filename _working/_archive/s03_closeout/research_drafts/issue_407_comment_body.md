**S03 Planning outcome (packetization v0.2)** — still WIP under [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351); not canon or implementation.

Consensus captured in **docs/product/wip/areas/radio/policy/packet_context_tx_rules_v0_1.md**.

**Agreed direction:**
- **Position path:** **Node_Pos_Full** = Core position + 2-byte Pos_Quality (one packet, one slot, one seq16 per update). Replaces current Core_Pos + Core_Tail split. Split Core+Tail is no longer the default; retained only as a possible fallback/calibration hypothesis if needed.
- **Status path:** **Node_Status** = single packet with full snapshot of: batteryPercent, battery_Est_Rem_Time, TX_power_Ch_throttle, uptime10m, role_id, maxSilence10s, hwProfileId, fwVersionId. Trigger taxonomy (agreed): **urgent triggers** — TX_power_Ch_throttle, maxSilence10s, role_id (urgent-config); **threshold triggers** — batteryPercent, battery_Est_Rem_Time; **hitchhiker-only** — uptime10m, hwProfileId, fwVersionId. Anti-burst pacing (min_status_interval_ms), bounded periodic refresh (T_status_max), repeated quick changes merge into pending/current snapshot (one slot).

**Baseline:** Current code (five slots, single cadence gate, Core+Tail coupled, Op+Info same gate) remains the reference; v0.2 is the agreed planning target, not yet implemented.

**Open (for later):** Pos_Quality 2-byte bit layout; backward compat for legacy Core/Tail; concrete min_status_interval_ms and T_status_max; hitchhiking policy; protocol numbers; **Initial Status Population** (when/how first Node_Status is produced — deferred to follow-up under #351).

**Follow-up WIP:** packet_sets_v0_1, tx_priority_and_arbitration_v0_1, channel_utilization_budgeting doc — add pointer to consolidated WIP / v0.2 direction when convenient; no removal of v0.1 content until v0.2 is adopted in canon.
