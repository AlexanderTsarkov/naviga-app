# S03 tail: #452 and #453 outcome notes (for issue comments)

**Paste the “Issue comment” blocks below into #452 and #453 after PR #455 is merged (or when closing).**

---

## Issue comment for #452 (paste when closing)

**Resolution (PR #455):** Investigated increment sites and semantics.  
- **tx_sent_*** = this node’s packets of that type successfully sent (M1Runtime after `radio_->send()` ok). **Aggregate.**  
- **rx_ok_*** = this node’s accepted-and-applied packets of that type (M1Runtime when `on_rx` returns updated). **Aggregate (any peer).**  
- Counters are **not** peer-specific, so e.g. PosRx > single peer’s PosTx is expected with multiple peers.  
**Fix:** Docs/code comments only (`traffic_counters.h`, `oled_status.cpp`, `debug_playbook.md`). No counter logic or label string change; #447 unchanged.

---

## Issue comment for #453 (paste when closing)

**Resolution (PR #455):** OOTB defaults locked per issue.  
- **Canon:** `role_profiles_policy_v0` §3.1 — Person 22/30/11, Dog 11/15/5.  
- **Firmware:** `get_ootb_role_profile` + storage defaults aligned; tests updated.  
- Source of truth: canon; runtime/OLED readback unchanged (already use resolved profile).

---

## #452 — PosTx/StTx/PosRx/StRx counter semantics

**Investigation result:**
- **tx_sent_***: Incremented in `M1Runtime::handle_tx()` when `radio_->send()` returns true. Meaning: *this node’s* packets of that type successfully handed to the radio (send ok). **Aggregate** (this node’s total by type).
- **rx_ok_***: Incremented in `M1Runtime::handle_rx()` when `beacon_logic_.on_rx()` returns `updated == true`. Meaning: *this node* accepted and applied a packet of that type (decode + NodeTable update). **Aggregate** (accepted from any peer).
- Counters are **not** peer-specific. So e.g. Node B’s PosRx can exceed Node A’s PosTx when B receives from A and other peers; “PosRx > peer PosTx” is expected.

**Fix applied (smallest correct):**
- **Docs/code comments only.** No counting logic changed; #447 HW validation meaning unchanged.
- `traffic_counters.h`: Documented that TX counters are “this node’s totals”, RX counters “accepted from any peer”, and that one node’s PosRx can exceed a single peer’s PosTx.
- `oled_status.cpp`: Comments at Lines 4–5 state that PosTx/StTx are “this node’s sent”, PosRx/StRx are “accepted from any peer”.
- `docs/dev/debug_playbook.md`: One sentence added under traffic validation noting #452 aggregate semantics.

**Intentionally not changed:** Counter increment sites, peer-specific metrics, or OLED label strings (compact labels kept per S03).

---

## #453 — OOTB default User Profile values

**Intended defaults (from issue):**
- **Dog:** Min_Interval=11 s, Min_Distance=15 m, Max_Silence=50 s → max_silence_10s=5
- **Human (Person):** Min_Interval=22 s, Min_Distance=30 m, Max_Silence=110 s → max_silence_10s=11

**Changes:**
- **Canon:** `docs/product/areas/domain/policy/role_profiles_policy_v0.md` §3.1 table updated to Person 22/30/11, Dog 11/15/5 (Infra unchanged).
- **Firmware:** `role_profile_ootb.cpp` — Person 22/30/11, Dog 11/15/5; default fallback 22/30/11.
- **Storage defaults:** `naviga_storage.cpp` and `naviga_storage.h` — kDefault* and `RoleProfileRecord` defaults set to Person OOTB (22, 11, 30.0f).
- **Tests:** `test_role_profile_registry` expectations updated for Person and Dog.

**Source of truth:** Canon (role_profiles_policy_v0 §3.1); firmware and NVS fallbacks align to it. Runtime/OLED readback unchanged (already use resolved profile from Phase B).

**Hardware verification:** Not required for this PR; values are deterministic and exercised by existing unit tests and devkit build.

---

## S03 readiness (do not close #351 here)

With #452 and #453 resolved:
- Counter semantics are documented; no misleading peer-vs-aggregate implication.
- OOTB User Profile defaults are locked in canon and firmware (Person 22/30/110, Dog 11/15/50).

S03 is in a good state for **final master closeout** in a later step. Do **not** update or close #351 in this pass.
