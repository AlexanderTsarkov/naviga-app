# Packetization & TX v0.2 — planning proposal (for #407)

**Status:** WIP planning — consensus captured; still under [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351). No implementation, no canon updates, no protocol number finalization.  
**Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351). **Issue:** [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407).  
**Sources:** [_working/research/tx_rules_from_code_407.md](tx_rules_from_code_407.md), [_working/research/packetization_tx_policy_s03_state_407.md](packetization_tx_policy_s03_state_407.md), [packet_sets_v0_1](../docs/product/wip/areas/nodetable/policy/packet_sets_v0_1.md), [tx_priority_and_arbitration_v0_1](../docs/product/wip/areas/nodetable/policy/tx_priority_and_arbitration_v0_1.md), [traffic_model_s03_v0_1](../docs/product/wip/areas/radio/policy/traffic_model_s03_v0_1.md), [channel_utilization_budgeting_and_profile_strategy_s03_v0_1](../docs/product/wip/areas/radio/policy/channel_utilization_budgeting_and_profile_strategy_s03_v0_1.md).

---

## Consensus reached (summary for #407)

- **Position path:** One packet **Node_Pos_Full** = Core position (WHO + WHERE + freshness) + **2-byte Pos_Quality**. Replaces current Core_Pos + Core_Tail split. One slot, one seq16 per position update; no ref_core_seq16. **Split Core + Tail** is no longer the default; it remains only as a possible fallback/calibration hypothesis if needed later.
- **Status path:** One packet **Node_Status** = merged operational + informative fields. Payload fields (agreed): batteryPercent, battery_Est_Rem_Time, TX_power_Ch_throttle, uptime10m, role_id, maxSilence10s, hwProfileId, fwVersionId. **Full snapshot** on any valid trigger. Trigger taxonomy: **urgent triggers**, **threshold triggers**, **hitchhiker-only** fields. **Anti-burst pacing** via min_status_interval_ms; **bounded periodic refresh**; repeated quick changes **merge into pending/current status snapshot** (replace-in-place, one slot).
- **Baseline:** Current code (five slots, single cadence gate, Core+Tail coupled, Op+Info same gate) remains the reference for “what exists today”; v0.2 is the agreed **planning target**, not yet implemented.

---

## 1) Current code baseline (summary)

What the firmware does today — **not** the target; the baseline to compare against.

| Aspect | Current code behavior |
|--------|------------------------|
| **Cadence gate** | Single clock: `elapsed = now_ms - last_tx_ms_`; `last_tx_ms_` updated only when **Core or Alive** is enqueued. All formation (Core, Alive, Tail, Op, Info) uses the same `time_for_min` / `time_for_silence`. No separate timers for Op or Info. |
| **allow_core** | Set **true** only by app when a position update is committed (SelfUpdatePolicy: DISTANCE, MAX_SILENCE, FIRST_FIX). Consumed (set false) after one formation pass. So at most one Core (and one Tail) per position commit. |
| **Core + Tail** | Tail is **only** enqueued in the same pass as Core, immediately after Core encode. One seq16 for Core, one for Tail; Tail carries `ref_core_seq16` = Core’s seq16. No Tail without Core. |
| **Op + Info** | Both enqueued when `(time_for_min \|\| time_for_silence)` AND the relevant `has_*` flags. **Same gate** as Core/Alive. No “10 min” for Info; no “MUST NOT send Info on every Operational” in code. |
| **Queue** | Five fixed slots (Core, Alive, Tail1, Tail2, Info). Replace-in-place by slot index; no explicit coalesce_key. Selection: P0 > P2 > P3; within priority: replaced_count DESC, created_at_ms ASC. Starvation: non-selected present slots get replaced_count++. Formation runs only when `!send_policy_.has_pending()` (after last payload was successfully sent). |

**Reference:** [tx_rules_from_code_407.md](tx_rules_from_code_407.md).

---

## 2) Agreed v0.2 packet model

| Packet | Role | Replaces (current) | Note |
|--------|------|---------------------|------|
| **Node_Pos_Full** | One position-bearing update: WHO + WHERE + freshness + **2-byte Pos_Quality** in a single frame. | Core_Pos (0x01) + Core_Tail (0x03) | One slot per position update; one seq16 per update; no separate ref_core_seq16. |
| **Alive** | Unchanged: no-fix liveness; identity + seq16 only. | Alive (0x02) | Still sent when !pos_valid && time_for_silence. |
| **Node_Status** | One “status” update: full snapshot of operational + informative fields in a single frame. | Operational (0x04) + Informative (0x05) | One slot; one eligibility/coalesce rule set; full snapshot on any valid trigger. |

**Node_Pos_Full payload (agreed):** Core position (Common + lat/lon) + **2-byte Pos_Quality**. Pos_Quality packs position quality (e.g. fix_type, sats, accuracy_bucket, pos_flags_small); exact bit layout TBD in encoding contract. Split Core + separate Core_Tail is **not** the default; it is retained only as a possible **fallback/calibration hypothesis** if needed (e.g. link-quality-driven fallback or future profiling).

**Node_Status payload (agreed):** Common prefix + the following fields (encoding order and optionality TBD in contract): **batteryPercent**, **battery_Est_Rem_Time**, **TX_power_Ch_throttle**, **uptime10m**, **role_id**, **maxSilence10s**, **hwProfileId**, **fwVersionId**.

---

## 3) Agreed TX rule tables (v0.2)

### 3.1 Node_Pos_Full

| Column | Agreed v0.2 rule |
|--------|--------------------|
| **Trigger** | Position valid AND (min_interval elapsed AND allow_core) OR max_silence elapsed. (Same logical trigger as current Core_Pos; allow_core still app-driven per position commit.) |
| **Earliest_at** | `last_pos_tx_ms + min_interval_ms` (or equivalent: no PosFull before min_interval since last position-bearing enqueue). |
| **Deadline / forced refresh** | If no position enqueue for `max_silence_ms`, force one PosFull (or Alive if !pos_valid) to satisfy liveness. |
| **Coalesce unit** | One slot per “position update”. New position sample replaces pending PosFull (same slot). Key: logical “position slot” (implicit: one slot). |
| **Priority / budget class** | P0 (same as current Core+Alive). Must not be starved below status packets. |
| **Depends_on** | allow_core from app (for min_interval path); pos_valid; GNSS/source of position. No dependency on Tail or any other packet. |
| **Replaceability / expiry under load** | Not replaceable by lower-priority traffic. Under load: drop P3 (Node_Status) first; PosFull and Alive preserved. |

### 3.2 Node_Status

| Column | Agreed v0.2 rule |
|--------|------------------|
| **Trigger** | Any **urgent** or **threshold** trigger (see §4) satisfied; subject to earliest_at and periodic refresh. **Full snapshot** of all Node_Status fields is sent on any valid trigger (no partial updates). |
| **Earliest_at** | **Anti-burst pacing:** no new Status enqueue before `last_status_enqueue_ms + min_status_interval_ms`. Repeated quick changes merge into the **pending/current status snapshot** (replace-in-place in the single Status slot); the next send carries the latest snapshot when the trigger is valid and earliest_at has elapsed. |
| **Deadline / forced refresh** | **Bounded periodic refresh:** at least one Node_Status must be sent every T_status_max (e.g. 10 min) so receivers get hitchhiker and other fields (uptime10m, hwProfileId, fwVersionId, etc.). Forces a send when interval exceeds T_status_max even if no urgent or threshold trigger. |
| **Coalesce unit** | One slot. New snapshot **replaces** pending (same slot). Repeated changes before send update the in-slot snapshot; only one pending Status at a time. |
| **Priority / budget class** | P3 (throttled). First to drop when U high; never above position traffic. |
| **Depends_on** | None on position. Optional hitchhike policy (e.g. enqueue Status in same formation pass as PosFull when conditions met) TBD. |
| **Replaceability / expiry under load** | Replaceable by newer Status (same slot). Under load: Status skipped/expired first; PosFull/Alive preserved. |

---

## 4) Node_Status field classification and trigger taxonomy (agreed)

Fields are classified by how they affect **when** to send Node_Status. On any valid trigger, a **full snapshot** of all Node_Status payload fields is sent.

| Class | Meaning | Fields | TX role |
|-------|--------|--------|--------|
| **Urgent triggers** | Change in value can trigger a send (subject to earliest_at and periodic refresh). | TX_power_Ch_throttle, maxSilence10s, role_id | Any change → eligible to enqueue Status (replace pending snapshot); send at next opportunity when earliest_at elapsed. role_id is rare config-trigger; if needed, treat as **urgent-config** (same family as urgent triggers). |
| **Threshold triggers** | Send when value crosses a threshold or is first set (e.g. bucketed battery, ETA). | batteryPercent, battery_Est_Rem_Time | Trigger when threshold crossed; also covered by bounded periodic refresh (T_status_max). |
| **Hitchhiker-only** | Included in Node_Status when it is sent for other reasons; do **not** alone trigger a send. | uptime10m, hwProfileId, fwVersionId | Only included in payload when Status is already being sent for an urgent or threshold trigger (or periodic refresh). |

Payload field set (agreed): batteryPercent, battery_Est_Rem_Time, TX_power_Ch_throttle, uptime10m, role_id, maxSilence10s, hwProfileId, fwVersionId.

---

## 5) Comparison: baseline vs agreed v0.2

### 5.1 What becomes simpler than current code

| Item | Today (code) | Agreed v0.2 |
|------|--------------|-------------|
| Position path slots | Two (Core + Tail); two seq16 per update; ref_core_seq16 in Tail. | One slot (PosFull); one seq16 per update; no ref_core_seq16. **Split Core + Tail** is no longer the default; only a possible fallback/calibration hypothesis if needed. |
| Tail RX logic | last_core_seq16, last_applied_tail_ref_core_seq16, match ref_core_seq16, at-most-once per ref. | Obsolete for PosFull; single-packet apply. |
| Status path slots | Two (Operational, Informative); same time gate; both enqueued when gate + has_* true. | One slot (Node_Status); one rule set; full snapshot on any valid trigger; repeated changes merge into pending. |
| Cadence for status | Op and Info share Core’s gate (no independent 10 min or “not on every Op”). | Explicit min_status_interval_ms (anti-burst), T_status_max (bounded periodic refresh). |
| Queue size | 5 slots. | 3 slots (PosFull, Alive, Node_Status). |

### 5.2 What new semantics are introduced in v0.2

| Item | New in v0.2 |
|------|-------------|
| **Earliest_at / deadline** | Not in code today. v0.2: explicit earliest_at for PosFull (same as min_interval); **min_status_interval_ms** (anti-burst) and **T_status_max** (bounded periodic refresh) for Node_Status. |
| **Status trigger taxonomy** | Code today: only “same gate as Core + has_*”. v0.2: **urgent triggers**, **threshold triggers**, **hitchhiker-only**; full snapshot on any valid trigger. |
| **Merge into pending snapshot** | Repeated quick changes to status fields update the single Status slot (replace-in-place); next send carries latest snapshot when trigger valid and earliest_at elapsed. |
| **PosFull layout** | New wire format: Common + lat/lon + **2-byte Pos_Quality**. New msg_type or version; RX path for “one packet = position + quality”. |

### 5.3 What old Tail/ref behavior becomes obsolete

| Obsolete (if v0.2 adopted) | Current role |
|----------------------------|--------------|
| **ref_core_seq16** (wire) | Tail linked to Core sample; RX applies only if ref == last_core_seq16. |
| **last_core_seq16** (RX) | Updated on Core_Pos RX; used for Tail apply. With PosFull, one packet carries position+quality; last_core_seq16 can still denote “last position update seq” but no Tail to match. |
| **last_applied_tail_ref_core_seq16** (RX) | Dedupe: at most one Tail per Core. No separate Tail in v0.2. |
| **Core_Tail slot (P2)** | Separate slot and priority for Tail. Replaced by PosFull (P0). |
| **Separate Operational and Informative slots** | Two P3 slots. Replaced by one Node_Status slot. |
| **“MUST NOT send Info on every Operational”** | Doc rule; not in code. With one Status packet, rule is replaced by “Status earliest_at / periodic floor” so Status is not sent on every PosFull unless designed (e.g. hitchhike). |

---

## 6) Open product questions (remaining)

1. **Pos_Quality 2-byte bit layout:** Exact packing of fix_type, sats, accuracy_bucket, pos_flags_small (or subset) into 2 B; encoding contract. [#359](https://github.com/AlexanderTsarkov/naviga-app/issues/359) GNSS Tail completeness.
2. **Backward compat:** Should legacy Core (0x01) and Tail (0x03) remain supported for a transition period? If yes, RX accepts both PosFull and Core+Tail; TX policy (when to send legacy vs PosFull) TBD.
3. **min_status_interval_ms and T_status_max:** Concrete values and source (config, role-based, or fixed). T_status_max agreed in principle (bounded periodic refresh); value (e.g. 10 min) TBD.
4. **Hitchhiking:** May Node_Status be enqueued in the same formation pass as PosFull when conditions met, or strictly independent trigger only?
5. **Protocol numbers:** msg_type or payloadVersion path for PosFull and Node_Status; decision deferred.

---

## 7) References

- **Baseline from code:** [tx_rules_from_code_407.md](tx_rules_from_code_407.md)
- **Docs/state consolidation:** [packetization_tx_policy_s03_state_407.md](packetization_tx_policy_s03_state_407.md)
- **Traffic/ToA:** [traffic_model_s03_v0_1](../docs/product/wip/areas/radio/policy/traffic_model_s03_v0_1.md) (§4 merged vs split U); [channel_utilization_budgeting_and_profile_strategy_s03_v0_1](../docs/product/wip/areas/radio/policy/channel_utilization_budgeting_and_profile_strategy_s03_v0_1.md) (§4 Ops/Info merge, P3)
- **WIP packet/priority:** [packet_sets_v0_1](../docs/product/wip/areas/nodetable/policy/packet_sets_v0_1.md), [tx_priority_and_arbitration_v0_1](../docs/product/wip/areas/nodetable/policy/tx_priority_and_arbitration_v0_1.md)

This note is the **authoritative WIP snapshot** for packetization/TX v0.2 planning under #407 and #351. It reflects current product consensus; v0.2 is not yet implemented or canon.

---

## 8) Follow-up WIP cleanup (no edits in this pass)

- **packet_sets_v0_1.md:** Still describes separate Node_Core_Tail (P2), Node_Operational, Node_Informative. Follow-up: add pointer to this proposal and note v0.2 direction (PosFull + Node_Status); do not remove v0.1 content until v0.2 is adopted in canon.
- **tx_priority_and_arbitration_v0_1.md:** Describes P0–P3 and five packet types. Follow-up: when v0.2 is promoted, add section or link for v0.2 (three slots: PosFull, Alive, Node_Status).
- **channel_utilization_budgeting_and_profile_strategy_s03_v0_1.md:** §4 already mentions “PosFull” and “Ops/Info merge into OpsInfo”; align naming (Node_Status) and split-Core-Tail as fallback hypothesis in a follow-up pass if needed.
