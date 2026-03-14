# #420 Contract and TX Truth Table

## 1) #420 contract (from issue + repo)

**Goal:** Implement packet formation so that **trigger**, **earliest_at**, **deadline**, **coalesce**, **preemptible**, and **budget_class** (or equivalent) follow the canon TX rules table in `packet_context_tx_rules_v0.md`. Current v0.1 table and agreed v0.2 table are the source of truth.

**Scope decision (A vs B):**
- **A)** Make current v0.1 packet behavior **truthful, explicit, and testable** (canon-aligned).
- **B)** Implement agreed v0.2 direction (Node_Pos_Full + standalone Node_Status) now.

**Chosen: A.** Issue does not require wire-format migration; v0.2 implies new packet types and timing (min_status_interval_ms, T_status_max) that belong to packetization redesign (#422). #420 = formation logic mirrors canon **v0.1** table; single source of rules (code + comment block); tests prove trigger/earliest/deadline/coalesce; no ad hoc triggers; payload field names aligned in comments/docs where applicable. **No hitchhiking** is a v0.2 rule; current v0.1 has Op and Info on the same cadence gate as Core/Alive, which is the documented v0.1 behavior.

**Definition of done (from issue):**
- Formation logic reads from a single source of rules (or code clearly mirrors the canon table).
- For each packet type: trigger conditions, earliest send time, deadline (e.g. max_silence), coalesce behavior (one slot, snapshot replace where applicable), replaceability under load match canon.
- No ad hoc triggers that contradict canon.
- Payload field names align with canon (uptime_10m etc.) — wire remains v0.1; comments document canon names.

**Touched paths:** TX/formation (beacon_logic.cpp), M1Runtime wiring (unchanged), tests (beacon_logic tests). Canon: packet_context_tx_rules_v0 §1.

**Boundary:**
- **#421** — RX semantics/apply rules: no change.
- **#422** — Packetization redesign / v0.2 migration: no change; v0.2 table is reference only.

---

## 2) TX truth table

| Packet       | Trigger (canon §1) | Current code | Match | Notes |
|-------------|--------------------|--------------|-------|--------|
| Core_Pos    | pos_valid AND (time_for_min AND allow_core) OR time_for_silence | `(time_for_min && allow_core) \|\| time_for_silence` when pos_valid | ✓ | |
| Alive       | !pos_valid AND time_for_silence | `time_for_silence` when !pos_valid | ✓ | |
| Core_Tail   | Only when Core_Pos enqueued same pass | Enqueued immediately after Core in same pass; ref_core_seq16 | ✓ | |
| Operational | (time_for_min \|\| time_for_silence) AND (has_battery \|\| has_uptime) | Same guard | ✓ | |
| Informative | (time_for_min \|\| time_for_silence) AND (has_max_silence \|\| has_hw_profile \|\| has_fw_version) | Same guard | ✓ | |

**Cadence:** Single clock `elapsed = now_ms - last_tx_ms_`; `last_tx_ms_` updated only when Core or Alive enqueued. Code: ✓.

**allow_core:** Set true by app on position commit; consumed after one formation pass. Code: M1Runtime set_allow_core_send(true) from app; cleared after update_tx_queue. ✓.

**Queue:** Five slots; replace-in-place by slot index; selection P0 > P2 > P3; within priority replaced_count DESC, created_at_ms ASC. Code: ✓.

**Active values:** TX uses SelfTelemetry (and self_fields) populated by app from device/runtime; no profile objects. ✓.

---

## 3) Minimum #420 delta

1. **Document** in `beacon_logic.cpp` that formation rules mirror `packet_context_tx_rules_v0.md` §1 (v0.1 table). Add comment block mapping each branch to the table row.
2. **Test** (optional but recommended): assert Op/Info are **not** enqueued when `elapsed < min_interval` and `elapsed < max_silence` (cadence gate).
3. **Docs:** No change to policy doc unless we add a one-liner "Implementation: firmware/src/domain/beacon_logic.cpp (update_tx_queue) mirrors this table as of #420."

---

## 4) Out of scope for #420

- **role_id** in Informative: packet_sets_v0 mentions role_id; current wire (info_codec) has no role_id. Adding it would be wire/contract change (#422 or separate).
- **Node_Pos_Full / Node_Status:** v0.2 packet types and timing; #422.
- **min_status_interval_ms / T_status_max:** Node_Status lifecycle; #422.
- **RX apply rules:** #421.
