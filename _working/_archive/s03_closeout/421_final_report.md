# #421 RX semantics alignment — final report

## Contract (restated)

**Goal:** Ensure RX semantics and apply rules match corrected canon: presence/age (last_seen_ms, last_seen_age_s, is_stale, pos_age_s node-owned); Tail–Core correlation is **runtime-local decoder logic only** (ref fields not canonical NodeTable truth); apply Tail to normalized Position block when decoder correlation succeeds. Accepted/duplicate/ooo per rx_semantics_v0 and packet_to_nodetable_mapping_v0.

**Scope:** Current v0.1 RX/apply truth-lock and explicit canon alignment. No packetization redesign (#422).

---

## Changes made

| Item | Change |
|------|--------|
| **node_table.cpp** | Comment block above `upsert_remote`: RX/apply semantics mirror rx_semantics_v0 and packet_to_nodetable_mapping_v0; Newer vs Same/Older behavior; ref fields (last_core_seq16, last_applied_tail_ref_core_seq16) are runtime-local decoder state only (not BLE, not persisted). Short comment above `apply_tail1`: ref match and one-per-ref; MUST NOT touch position; references rx_semantics_v0 §4. |
| **rx_semantics_v0.md** | New §6 "Implementation and related": implementation in node_table.cpp (upsert_remote, apply_tail1, apply_tail2, apply_info); ref fields runtime-local per master table §7. |
| **_working/421_contract_and_truth_table.md** | Contract, RX/apply truth table (current code vs canon), minimum delta, out-of-scope. |

**No behavior change.** Apply logic already matched canon; ref fields were already excluded from BLE/persistence in #419. This pass makes the mapping explicit in code and docs.

---

## RX/apply truth table (summary)

- **Core_Pos / Alive:** upsert_remote — seq16 Newer → full update + last_core_seq16 when pos_valid; Same/Older → last_seen_ms + last_rx_rssi only. ✓
- **Tail-1:** apply_tail1 — dedupe by (nodeId, seq16); ref_core_seq16 == last_core_seq16; at most one Tail per ref; apply pos_flags/sats only; never touch position; always update last_seen_ms + last_rx_rssi. ✓
- **Operational / Informative:** apply_tail2 / apply_info — dedupe; create if missing; Same/Older → last_seen_ms + last_rx_rssi only. ✓
- **Ref fields:** In NodeEntry for decoder only; not in BLE (get_page); not in persistence (snapshot); master table §7. ✓

---

## Validation

- **`pio run -e devkit_e220_oled`** — PASSED.
- **Existing tests:** test_beacon_logic (apply_tail1 match/mismatch/duplicate, Tail never overwrites position); test_node_table_domain (seq/ooo, restore exclusions for last_core_seq16 / last_applied_tail_ref*). No new test added; existing coverage suffices.

---

## Boundary

- **#420 (TX):** No change.
- **#422 (packetization / v0.2):** No change; RX/apply remains v0.1 wire and apply rules.

---

## Remaining for #422 only

Packetization redesign, v0.2 packet types, throttle/queueing. No further RX/apply scope in #421.

---

## Recommendation

#421 is **complete** for the chosen scope (v0.1 RX/apply explicit and canon-aligned). Ready for branch/PR: one commit (node_table.cpp comments + rx_semantics_v0 implementation reference). Ref fields remain runtime-local only; no BLE/persistence leak.
