# TX rules from code — actual behavior (research for #407)

**Purpose:** Reverse-engineer the **actual** TX formation, eligibility, queue, and arbitration logic from firmware code. Code is the source of truth for “what happens today”; WIP docs are compared for mismatches. No implementation or canon changes.  
**Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351). **Issue:** [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407). **Companion:** [_working/research/packetization_tx_policy_s03_state_407.md](packetization_tx_policy_s03_state_407.md) (docs/state consolidation).

---

## Summary

- **Entry:** TX is driven by `M1Runtime::tick()` → `handle_tx()`. Formation runs only when `BeaconSendPolicy::has_pending()` is false (i.e. after the previous payload was successfully sent). So formation runs once per consumed TX, not every tick.
- **Time gate:** All formation uses a single clock: `elapsed = now_ms - last_tx_ms_`, where `last_tx_ms_` is updated only when **Core or Alive** is enqueued. So Operational and Informative have **no separate cadence** in code; they are only enqueued when the same “time for Core/Alive” condition is true.
- **allow_core:** Core_Pos is only enqueued when `(time_for_min && allow_core) || time_for_silence`. `allow_core` is set true only by the app layer when a position update is committed (SelfUpdatePolicy); after formation it is set false. So at most one Core (and thus one Tail) per “position commit”.
- **Queue:** Slots persist until dequeued. Formation adds or **replaces** by slot index; no explicit coalesce_key in code. Selection: P0 > P2 > P3; within same priority, replaced_count DESC then created_at_ms ASC. On dequeue, all other present slots get replaced_count++ (starvation).
- **seq16 / ref_core_seq16:** One global seq per node in BeaconLogic; each enqueued packet gets next_seq16(). Tail’s ref_core_seq16 is set to the Core sample’s seq16. RX applies Tail only if ref_core_seq16 == last_core_seq16 and at most once per ref (last_applied_tail_ref_core_seq16).

---

## Code paths inspected

| Path | File(s) | Role |
|------|---------|------|
| TX entry | `firmware/src/app/m1_runtime.cpp` — `tick()` → `handle_tx()` | Formation only when `!send_policy_.has_pending()`; then `update_tx_queue()`; then `dequeue_tx()` one slot; then `on_payload_built()`. Actual send when `ready_to_attempt(now_ms)`; `on_send_result(ok)` clears pending on success. |
| Formation | `firmware/src/domain/beacon_logic.cpp` — `update_tx_queue()` | Computes `time_for_min`, `time_for_silence` from `elapsed` vs `min_interval_ms_`, `max_silence_ms_`. Enqueues Core/Alive/Tail1/Tail2/Info into fixed slots. |
| allow_core source | `firmware/src/app/app_services.cpp` — `SelfUpdatePolicy::commit()` path | `runtime_.set_allow_core_send(true)` when position is committed (DISTANCE, MAX_SILENCE, FIRST_FIX). |
| allow_core consumption | `firmware/src/app/m1_runtime.cpp` — `handle_tx()` L178–181 | After `update_tx_queue(..., allow_core_send_)`, if true then `allow_core_send_ = false`. |
| Send policy | `firmware/src/domain/beacon_send_policy.cpp` | `has_pending()` = payload built but not yet sent; `on_send_result(true)` sets `pending_ = false`. Jitter/backoff only affect **when** we attempt send, not formation. |
| Queue / selection | `firmware/src/domain/beacon_logic.cpp` — `enqueue_slot()`, `dequeue_tx()` | Replace-in-place per slot; selection by priority, be_rank, replaced_count DESC, created_at_ms ASC; starvation +1 on dequeue. |
| Seq / ref (TX) | `beacon_logic.cpp` — `next_seq16()`, Tail formation L152–158 | Core gets seq S, Tail gets seq S+1, ref_core_seq16 = S. |
| Seq / ref (RX) | `firmware/src/domain/node_table.cpp` — `upsert_remote()`, `apply_tail1()` | `last_core_seq16` set only on Core_Pos RX; Tail applied only if `ref_core_seq16 == last_core_seq16` and not already applied for that ref. |
| Self telemetry | `firmware/src/app/app_services.cpp` — init, tick | `self_telemetry_.has_max_silence`, `max_silence_10s` from role; `has_battery`/`battery_percent` stubbed (e.g. 100); `has_hw_profile`/`has_fw_version` set false (not yet formalized). Uptime set each tick. |

---

## Current packet streams in code

| Packet | Formed in code? | Condition (code) |
|--------|------------------|------------------|
| **Core_Pos** (0x01) | Yes | `pos_valid && ((time_for_min && allow_core) \|\| time_for_silence)` |
| **Alive** (0x02) | Yes | `!pos_valid && time_for_silence` |
| **Core_Tail** (0x03) | Yes | Only when Core_Pos was just enqueued in the same `update_tx_queue()` call (immediately after Core encode). |
| **Operational** (0x04) | Yes | `(time_for_min \|\| time_for_silence) && (telemetry.has_battery \|\| telemetry.has_uptime)` |
| **Informative** (0x05) | Yes | `(time_for_min \|\| time_for_silence) && (telemetry.has_max_silence \|\| telemetry.has_hw_profile \|\| telemetry.has_fw_version)` |

So all five are formable. Core_Tail has no independent trigger; it is strictly tied to Core_Pos in the same formation pass.

---

## Actual TX rules per packet type (from code)

### 1) What packets can be formed today in code?

All five: Core_Pos, Alive, Core_Tail, Operational, Informative. Encoders: `encode_geo_beacon`, `encode_alive_frame`, `encode_tail1_frame`, `encode_tail2_frame`, `encode_info_frame` in `beacon_logic.cpp`. Tail1 uses `protocol::Tail1Fields` (ref_core_seq16, optional posFlags/sats); Tail2/Info use current v0 layout (no packed battery16/radio8/uptime10m in code).

### 2) Exact conditions that cause each packet to be generated

- **Core_Pos:**  
  - `self_fields.pos_valid != 0`  
  - AND `(time_for_min && allow_core) || time_for_silence`  
  - Where `time_for_min = (elapsed >= min_interval_ms_)`, `time_for_silence = (max_silence_ms_ > 0 && elapsed >= max_silence_ms_)`, `elapsed = (last_tx_ms_ == 0) ? now_ms : (now_ms - last_tx_ms_)`.  
  - So: “position valid” and (min_interval elapsed and app allowed Core) or max_silence elapsed.

- **Alive:**  
  - `self_fields.pos_valid == 0` and `time_for_silence`.  
  - No allow_core; silence alone is enough.

- **Core_Tail:**  
  - Only inside the block where Core_Pos was just enqueued (same pass).  
  - Encoded with `ref_core_seq16 = core_seq` (the Core just enqueued), `tail.seq16 = next_seq16()` (new seq for the Tail packet).  
  - So: generated iff Core_Pos was generated this pass; no separate “min_interval for Tail”.

- **Operational:**  
  - `(time_for_min || time_for_silence)` AND `(telemetry.has_battery || telemetry.has_uptime)`.  
  - No “on change” or snapshot compare in BeaconLogic; no separate min_interval or 60 s timer.

- **Informative:**  
  - `(time_for_min || time_for_silence)` AND `(telemetry.has_max_silence || telemetry.has_hw_profile || telemetry.has_fw_version)`.  
  - No 10 min timer; no “MUST NOT send on every Operational” check in code.

### 3) Exact conditions that suppress generation

- **Core_Pos suppressed:**  
  - `!pos_valid` → no Core (Alive path instead).  
  - `pos_valid` but `!time_for_min && !time_for_silence` → nothing (no Core, no Alive).  
  - `pos_valid` and `time_for_min` but `!allow_core` and `!time_for_silence` → no Core (comment in code: “at min_interval without position commit → no send”).  
  - Core encode fails → seq16 rolled back, no enqueue, last_tx_ms_ not updated.

- **Core_Tail suppressed:**  
  - Whenever Core_Pos is not enqueued this pass (including allow_core false at min_interval, or encode failure).  
  - Tail1 encode fails → Tail not enqueued (Core already enqueued; seq16 already advanced for Core and Tail).

- **Operational / Informative suppressed:**  
  - When `!time_for_min && !time_for_silence` (same clock as Core).  
  - When no has_* flags true (Operational: no battery/uptime; Informative: no max_silence/hw/fw).  
  - Encode failure → that slot not filled.

- **Alive suppressed:**  
  - When `pos_valid` (Core path) or `!time_for_silence`.

### 4) Queue / coalesce / replacement behavior in code

- **Structure:** Five fixed slots (kSlotCore, kSlotAlive, kSlotTail1, kSlotTail2, kSlotInfo). One slot per packet type. No generic queue; no explicit “coalesce_key” field.

- **Replacement:** Enqueue to a slot always overwrites that slot’s frame and updates priority/be_rank/pkt_type/ref_core_seq16/frame_len. If the slot was already present, `replaced_count++` and `created_at_ms` is **preserved**. If was not present, `created_at_ms = now_ms`, `replaced_count = 0`.

- **Coalesce semantics:** Implicit: slot index is the key. So “one pending Core”, “one pending Tail1”, etc. New formation to same slot replaces previous; replaced_count increments (expired_counter-like). No per-packet “key” (e.g. ref_core_seq16) used as coalesce key in code; Tail slot is just one slot, so a new Tail (new ref_core_seq16) replaces the previous Tail.

- **When formation runs:** Only when `!send_policy_.has_pending()`, i.e. after the last dequeued payload has been successfully sent. So we do not re-form every tick; we re-form when we need the next payload. Slots that were not written this pass remain (e.g. previous Tail1, Op, Info still present until dequeued).

- **Drain order:** One dequeue per “need payload”. So after a formation that fills e.g. Core + Tail1 + Op + Info, we send Core first (P0), then next tick we don’t form (has_pending true until send success), then we send; after send success we form again. But we only enqueue when `time_for_min || time_for_silence`; and `last_tx_ms_` was set when we **enqueued** Core. So until min_interval (or max_silence) elapses, next formation will not enqueue Core/Alive/Op/Info again. So we drain the rest (Tail1, Op, Info) one per tick until queue empty or we hit next cadence. So burst: on cadence we fill up to 5 slots; then we send one per tick (with jitter/backoff) until next cadence.

### 5) Priority / arbitration behavior in code

- **Selection** (`dequeue_tx()`):  
  1. TxPriority (lower value first): P0 > P1 > P2 > P3.  
  2. Within same priority, TxBestEffortClass (be_rank): BE_HIGH (0) before BE_LOW (1). Only Tail1 uses P2 and BE_HIGH.  
  3. Then replaced_count descending (more starved first).  
  4. Then created_at_ms ascending (older first).

- **Starvation:** When we dequeue slot i, every other **present** slot j ≠ i gets `replaced_count++`. So slots that stay pending get higher replaced_count and win next time within same priority.

- **Reset on send:** The dequeued slot is cleared (`slot = TxSlot{}`), so its replaced_count is gone; it’s a new slot when next enqueued.

### 6) seq16 / ref_core_seq16 behavior in code

- **TX:**  
  - `next_seq16()` increments `seq_` and returns new value. Called once per enqueued packet (Core, Alive, Tail1, Tail2, Info). So each packet gets a distinct seq16.  
  - For Core_Pos: `core_seq = next_seq16()`; frame carries core_seq.  
  - For Tail1: `tail.ref_core_seq16 = core_seq` (the Core just enqueued); `tail.seq16 = next_seq16()` (Tail’s own seq).  
  - last_tx_ms_ is updated only when we enqueue Core or Alive (so the “cadence” clock is “time since last Core or Alive enqueue”, not “time since any send”).

- **RX:**  
  - Core_Pos: `table.upsert_remote(..., fields.seq, now_ms)`. In NodeTable, when pos_valid, `entry.last_core_seq16 = last_seq` (the Core’s seq16).  
  - Tail1: `table.apply_tail1(node_id, tail1.seq16, tail1.ref_core_seq16, ...)`. apply_tail1: if `!entry.has_core_seq16 || entry.last_core_seq16 != ref_core_seq16` → drop (update last_seen_ms, last_rx_rssi only). If `has_applied_tail_ref_core_seq16 && last_applied_tail_ref_core_seq16 == ref_core_seq16` → drop (duplicate Tail for same Core). Else apply: update last_seq to tail’s seq16, set last_applied_tail_ref_core_seq16 = ref_core_seq16, update pos_flags/sats.

So: ref_core_seq16 is required for Tail apply; at most one Tail per Core sample on RX; seq16 is global per packet for dedupe.

---

## Code vs WIP comparison

| Rule / concept | Code | WIP (packet_sets_v0_1 / tx_priority / field_cadence) | Classification |
|----------------|------|-------------------------------------------------------|----------------|
| Core_Pos trigger | time_for_min && allow_core \|\| time_for_silence; allow_core from app (position commit) | First-fix at next opportunity; then min_interval / max_silence | **Implemented** (first-fix is in app SelfUpdatePolicy; allow_core consumed after formation). |
| Core_Tail trigger | Only when Core_Pos enqueued same pass | One per Core sample; “when Core_Pos was sent and min_interval/coalesce allow” | **Implemented** (Tail only with Core in same pass). WIP “min_interval/coalesce allow” is redundant in code — Tail has no separate gate. |
| Operational trigger | (time_for_min \|\| time_for_silence) && (has_battery \|\| has_uptime) | Changed-gated + min_interval; snapshot; reboot exception | **Partial:** Code has no “on change”, no snapshot compare, no reboot exception. Code uses **same** time gate as Core (not independent 60 s). |
| Informative trigger | (time_for_min \|\| time_for_silence) && (has_max_silence \|\| has_hw \|\| has_fw) | Periodic + changed-boost; min_interval = max_silence10 or configured; MUST NOT send on every Operational | **Partial:** Code has no 10 min timer; no “not on every Operational” rule. Same time gate as Core. |
| Coalesce key | Slot index only; replace-in-place | Per-type key (e.g. ref_core_seq16 for Tail, snapshot for Op) | **Implemented but simplified:** Code uses one slot per type; “key” is implicit (slot). WIP coalesce_key is conceptual; code doesn’t store key, only replaces by slot. |
| replaced_count | +1 on replace, +1 on starvation, reset on send (slot cleared) | expired_counter: +1 replace, +1 starved, reset on send | **Implemented** (matches). |
| Priority P0/P2/P3 | P0 Core+Alive, P2 Tail1, P3 Tail2+Info | Same | **Implemented.** |
| last_tx_ms_ / cadence clock | Updated only when Core or Alive enqueued | — | **Implicit in code;** docs don’t state that Op/Info share Core’s clock. |
| allow_core lifecycle | Set true by app on position commit; set false after formation when true | minDisplacement gate; “allow CORE at next TX” | **Implemented** (app_services sets true; m1_runtime sets false after formation). |
| First-fix / bootstrap | In app: SelfUpdatePolicy.evaluate → commit → set_allow_core_send(true) | field_cadence_v0 §2.1: first Core at next opportunity without min displacement | **Implemented in app layer** (SelfUpdateReason::FIRST_FIX). |
| “At forced Core” (Op) | Not separate: Op enqueued whenever Core/Alive time gate is true and has_* | “On change + at forced Core (every N Core)” | **Doc assumes N Core; code uses same time gate**, so effectively “whenever we’re due for Core and have Op data”. |
| Informative “every 10 min” | No 10 min; same gate as Core | “On change + every 10 min” | **Documented but not implemented** (no separate 10 min in code). |
| “MUST NOT send Info on every Operational” | Not enforced | field_cadence_v0 §2.2 | **Documented but not implemented** (both Op and Info gated by same condition; if both have data, both enqueued same pass). |

---

## Implications for #407 packetization planning

- **PosFull (merge Core + Tail):**  
  - Code today always enqueues Tail in the same pass as Core and never enqueues Tail without Core. So “one position update = one Core + one Tail” is already a fixed pair. Merging into one packet would remove the separate Tail slot and one seq16 per position update; ref_core_seq16 and last_applied_tail_ref_core_seq16 on RX would become unnecessary for that packet.  
  - **Pain:** Tail slot and ref_core_seq16 are baked into formation and RX; any “PosFull” variant would need a distinct formation path and RX path.  
  - **Benefit:** One slot per position update; simpler queue; one airtime per update (ToA win per #360).

- **Ops_Info merge:**  
  - Code today enqueues Operational and Informative under the **same** time gate and does not enforce “Info not on every Operational”. So if both have data, both are enqueued in the same formation pass. Merging into one packet would align with “one low-priority update” and one slot; no separate 10 min or “not on every Op” in code to preserve.  
  - **Pain:** Would need one eligibility rule and one coalesce (one slot).  
  - **Benefit:** Single P3 slot for “ops+info”; simpler than two slots and no doc/code mismatch for 10 min / “MUST NOT”.

- **Explicit rule table (triggers, earliest_at, deadline, etc.):**  
  - Code has no `earliest_at`, `deadline`, `preemptible`, `depends_on`, or `budget_class`. It has: one time gate (elapsed vs min_interval/max_silence), allow_core boolean, and has_* flags. So #407’s “explicit rule table” would be **new specification**; current code would need to be described as the baseline and then extended or refactored to match new concepts.

---

## Open questions

1. Should Operational and Informative have **independent** cadences in a future contract (e.g. Op every 60 s, Info every 10 min), or is “same gate as Core” intentional and docs should be updated to match code?
2. Should “MUST NOT send Informative on every Operational” be enforced in code (e.g. separate timer for Info, or suppress Info when Op was just enqueued)?
3. Is the current “burst then drain” (form when cadence due, fill up to 5 slots, send one per tick until next cadence) the intended product behavior, or should formation run every tick and only enqueue when conditions hold (so queue reflects “what’s due now” only)?
4. For v0.2 packetization: if PosFull is adopted, should Tail-1 (0x03) remain supported for backward compat, or deprecated once all nodes use PosFull?
5. Where should “first-fix” and “min displacement” be documented as the single source of truth: BeaconLogic (which only sees allow_core), or app layer (SelfUpdatePolicy + set_allow_core_send)?
6. replaced_count is preserved on replace and only reset when slot is sent; created_at_ms is preserved on replace. So “old” pending slots get higher replaced_count over time. Is that the intended fairness (most-starved first) or should created_at_ms be updated on replace so “newest” wins within same priority?
