# Radio — Packet context & TX rules v0

**Status:** Canon (policy).  
**Issue:** [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351).

Consolidated outcome for packet context and TX rules: current v0.1 behavior (baseline from code), agreed v0.2 packetization direction (Node_Pos_Full, Node_Status), and migration notes. Semantic truth is this policy; implementation and protocol numbering follow from here.

---

## 1) Current v0.1 behavior (from code)

What the firmware does today — **baseline**, not the v0.2 target. Implementation: `firmware/src/domain/beacon_logic.cpp` (`update_tx_queue`) mirrors this table (#420).

| Aspect | Current code behavior |
|--------|------------------------|
| **Cadence gate** | Single clock: `elapsed = now_ms - last_tx_ms_`; `last_tx_ms_` updated only when **Core or Alive** is enqueued. All formation uses the same `time_for_min` / `time_for_silence`. No separate timers for Op or Info. |
| **allow_core** | Set **true** only by app when a position update is committed (SelfUpdatePolicy: DISTANCE, MAX_SILENCE, FIRST_FIX). Consumed (set false) after one formation pass. At most one Core (and one Tail) per position commit. |
| **Core + Tail** | Tail only enqueued in the same pass as Core, immediately after Core encode. One seq16 for Core, one for Tail; Tail carries `ref_core_seq16` = Core's seq16. No Tail without Core. |
| **Op + Info** | Both enqueued when `(time_for_min \|\| time_for_silence)` AND the relevant `has_*` flags. Same gate as Core/Alive. No separate 10 min for Info in code. |
| **Queue** | Five fixed slots (Core, Alive, Tail1, Tail2, Info). Replace-in-place by slot index. Selection: P0 > P2 > P3; within priority: replaced_count DESC, created_at_ms ASC. Formation runs only when `!send_policy_.has_pending()` (after last payload successfully sent). |

**Explicit TX rule table (current code):**

| Packet | Trigger | Priority | Coalesce | Notes |
|--------|---------|----------|----------|--------|
| Core_Pos | pos_valid AND (time_for_min AND allow_core) OR time_for_silence | P0 | One slot | allow_core app-driven. |
| Alive | !pos_valid AND time_for_silence | P0 | One slot | No allow_core. |
| Core_Tail | Only when Core_Pos enqueued same pass | P2 | One slot; ref_core_seq16 in payload | No independent trigger. |
| Operational (0x04) | (time_for_min \|\| time_for_silence) AND (has_battery \|\| has_uptime) | P3 | One slot | Same gate as Core. |
| Informative (0x05) | (time_for_min \|\| time_for_silence) AND (has_max_silence \|\| has_hw_profile \|\| has_fw_version) | P3 | One slot | Same gate as Core. |

---

## 2) Agreed v0.2 packetization direction (planning target)

- **Position path:** **Node_Pos_Full** = Core position (WHO + WHERE + freshness) + **2-byte Pos_Quality**. One packet, one slot, one seq16 per position update; no ref_core_seq16. Replaces Core_Pos + Core_Tail. **Split Core + Tail** is **not** the default; retained only as a possible **fallback/calibration hypothesis** if needed.
- **Alive:** Unchanged; no-fix liveness.
- **Status path:** **Node_Status** = single packet with **full snapshot** of operational + informative fields. One slot; full snapshot on any valid trigger.

**Node_Pos_Full payload (agreed):** Core position (Common + lat/lon) + 2-byte Pos_Quality. Pos_Quality bit layout TBD in encoding contract.

**Node_Status payload (agreed):** Common prefix + batteryPercent, battery_Est_Rem_Time, TX_power_Ch_throttle, uptime10m, role_id, maxSilence10s, hwProfileId, fwVersionId (encoding order and optionality TBD in contract).

**Node_Status TX semantics (agreed):** See **§2a** for the full Node_Status lifecycle policy (bootstrap, triggers, steady-state, timing).

- **Timing (agreed):** `min_status_interval_ms = 30s`; `T_status_max = 300s` (T_status_max = 10 × min_status_interval_ms). If no Node_Status was sent within T_status_max, send a full Node_Status at the next allowed opportunity.

**Explicit TX rule table (agreed v0.2):**

| Packet | Trigger | Earliest_at | Deadline | Coalesce | Priority | Replaceability under load |
|--------|---------|-------------|----------|----------|----------|----------------------------|
| Node_Pos_Full | pos_valid AND (min_interval AND allow_core) OR max_silence | last_pos_tx_ms + min_interval_ms | max_silence → force PosFull or Alive | One slot | P0 | Not replaceable by P3. |
| Alive | !pos_valid AND time_for_silence | — | max_silence | One slot | P0 | Not replaceable by P3. |
| Node_Status | Any urgent or threshold trigger; subject to earliest_at and T_status_max | last_status_enqueue_ms + min_status_interval_ms | T_status_max (bounded periodic refresh) | One slot; snapshot replace | P3 | Skip/expire first. |

---

### 2a) Node_Status lifecycle policy (agreed)

Applies **only to Node_Status**. Does not change Node_PosFull cadence or creation rules. Node_PosFull and Node_Status remain separate packet types. Node_Status is always sent as a **full snapshot**; no partial Node_Status delivery via other packet types. **Hitchhiking not allowed:** Node_Status is never enqueued in the same formation pass as Node_PosFull; status trigger and send are independent of the position path. No ad hoc piggybacking of status content into Node_PosFull or any other packet type.

**Initial Status Population (bootstrap):**

- On boot/restart set `status_bootstrap_pending = true`.
- Send one full Node_Status at the first allowed opportunity.
- Allow one additional bootstrap retry only. **Total bootstrap sends limited to 2.**
- Second bootstrap send not earlier than `min_status_interval_ms` after the first.
- Bootstrap then ends; steady-state rules apply.

**Trigger semantics:**

- **Urgent triggers:** TX_power_Ch_throttle, maxSilence10s, role_id (rare config-trigger / urgent-config). Change → eligible to enqueue (replace pending snapshot); send when earliest_at elapsed.
- **Threshold triggers:** batteryPercent, battery_Est_Rem_Time. Trigger when threshold crossed; also covered by T_status_max.
- **Hitchhiker-only:** uptime10m, hwProfileId, fwVersionId. Do **not** alone trigger a send; included when Status is sent for urgent/threshold trigger or periodic refresh.

**Steady-state:**

- Any valid trigger raises or updates pending status intent.
- If status is already pending, further changes **merge into the current pending snapshot**.
- One logical Node_Status slot/intent only.
- Node_Status stays standalone. Hitchhiking (same-pass or piggyback with PosFull) is **not allowed**.

**Timing:**

- `min_status_interval_ms = 30s`
- `T_status_max = 300s` (T_status_max = 10 × min_status_interval_ms)
- If no Node_Status was sent within T_status_max, send a full Node_Status at the next allowed opportunity.

---

## 3) Migration notes / impact

- **Obsolete with v0.2:** ref_core_seq16 (wire); last_applied_tail_ref_core_seq16 (RX); Core_Tail slot (P2); separate Operational and Informative slots; "MUST NOT send Info on every Operational" as separate rule.
- **Simpler:** 3 slots (PosFull, Alive, Node_Status); one seq16 per position update; one Status rule set; explicit earliest_at and T_status_max for Status.
- **New semantics:** Earliest_at / deadline for Status; trigger taxonomy (urgent / threshold / hitchhiker); merge-into-pending snapshot; PosFull wire format.

**#422 Path B (implemented):** v0.1 packet family retained; effective behavior aligned with canon via throttle/merge: status (Op/Info) uses `min_status_interval_ms` (30s) and `T_status_max` (300s); no hitchhiking (Op/Info not enqueued in same formation pass as Core/Alive); at most one status enqueued per pass; bootstrap up to 2 sends. See §1 for current code behavior; §2/§2a for target semantics that Path B approximates.

---

## 4) Open points and external dependencies

- **Pos_Quality 2-byte bit layout:** Encoding contract; tracked as external/dependent work under sibling issue [#359](https://github.com/AlexanderTsarkov/naviga-app/issues/359). Not a blocker for #407 semantic closure.

*Closed / out of scope for #407:* Node_Status lifecycle (including bootstrap and timing) agreed in §2a. Hitchhiking **closed — not allowed**; Node_Status is standalone, independent of PosFull formation pass. Protocol/wire numbering (msg_type, payloadVersion) is a later contract/wire-format follow-up, not part of #407 packet-purpose and TX-rule closure. Backward compatibility is out of scope for this policy.*

---

## 5) Related

- [packet_sets_v0](../../nodetable/policy/packet_sets_v0.md), [tx_priority_and_arbitration_v0](../../nodetable/policy/tx_priority_and_arbitration_v0.md) — Packet sets and TX priority (v0.1 alignment).
- [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407).
