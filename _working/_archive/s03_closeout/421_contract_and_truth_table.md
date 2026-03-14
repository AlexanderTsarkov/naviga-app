# #421 Contract and RX/Apply Truth Table

## 1) #421 contract (from issue)

**Goal:** Ensure RX semantics and apply rules match **corrected canon**: presence/age (last_seen_ms, last_seen_age_s, is_stale, **pos_age_s** node-owned); **Tail–Core correlation is runtime-local decoder logic only** — canonical NodeTable does **not** store last_core_seq16 or last_applied_tail_ref_core_seq16 as product truth; apply Tail payload to normalized Position block when decoder correlation succeeds. Accepted/duplicate/ooo per rx_semantics_v0 and packet_to_nodetable_mapping_v0.

**DoD:**
- On RX: apply logic for each packet type matches canon (which fields updated; Tail apply uses **runtime-local** correlation only).
- Presence and age: last_seen_ms updated on accepted packets; is_stale/pos_age_s derived per canon (pos_age_s = node-owned).
- Coalesce/replace: no overwriting newer with older. Ref fields not stored as canonical NodeTable truth (runtime-local only; not BLE, not persisted).

**Scope:** Current v0.1 RX/apply truth-lock and explicit canon alignment. No packetization redesign (#422).

**Boundary:** #420 done; #422 = packetization/v0.2 (no change).

---

## 2) RX/apply truth table

| Packet      | Canon behavior | Current code | Match |
|-------------|----------------|--------------|-------|
| Core_Pos    | Accept by seq16; update position, last_seen_ms, last_rx_rssi, last_seq; update last_core_seq16 only on Core (for Tail gate). Duplicate/Older: refresh last_seen_ms + last_rx_rssi only. | upsert_remote: Newer → full update + last_core_seq16 when pos_valid; Same/Older → last_seen_ms + last_rx_rssi only. | ✓ |
| Alive       | Accept; update last_seen_ms, last_rx_rssi, last_seq; no position. | upsert_remote with pos_valid=false. | ✓ |
| Tail-1      | Apply only if ref_core_seq16 == last_core_seq16; at most one Tail per ref; dedupe (nodeId, seq16). When applied: pos_flags/sats only; never touch position. On drop: still update last_seen_ms + last_rx_rssi. | apply_tail1: seq16 order first; ref match; one-per-ref; apply only pos_flags/sats; always update last_seen_ms + last_rx_rssi. | ✓ |
| Operational | Dedupe (nodeId, seq16); apply battery/uptime; create entry if missing. Same/Older → last_seen_ms + last_rx_rssi only. | apply_tail2: same. | ✓ |
| Informative | Dedupe (nodeId, seq16); apply max_silence/hw/fw; create if missing. Same/Older → last_seen_ms + last_rx_rssi only. | apply_info: same. | ✓ |

**Ref fields:** last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16 — used only inside apply path for Tail–Core correlation. Not in BLE (per #419). Not in persistence (per #418). Master table §7: runtime-local only.

**last_seen_ms:** Updated on any accepted RX (all paths). ✓  
**pos_age_s:** Node-owned; updated only with position (Core); not overwritten by Tail/Op/Info. ✓  
**is_stale / last_seen_age_s:** Derived at export (is_grey, age_s from last_seen_ms). ✓  

---

## 3) Minimum #421 delta

1. **Document** in node_table.cpp that RX/apply semantics mirror rx_semantics_v0 and packet_to_nodetable_mapping_v0; ref fields are runtime-local decoder state only (not BLE, not persisted).
2. **Doc sync:** Add implementation reference in rx_semantics_v0 or packet_to_nodetable_mapping_v0.
3. **Tests:** Existing tests (test_beacon_logic apply_tail1 match/mismatch/duplicate; test_node_table_domain seq/ooo, restore exclusions) already cover apply and ref/restore. No new test required unless we add an explicit “presence/age” assertion.

---

## 4) Out of scope for #421

- Packetization / v0.2 packet types (#422).
- TX formation (#420).
- Adding SNR to RX path when radio supports it (E22 → snr_last stays 127).
