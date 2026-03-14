# #407 cross-WIP consistency pass (post–PR #408 merge)

**Date:** 2025-03. **Goal:** Minimal alignment of adjacent WIP docs so stale wording does not present old packetization semantics as equally current with the agreed #407 snapshot.

---

## 1) Authoritative snapshot read

**Source:** `docs/product/wip/areas/radio/policy/packet_context_tx_rules_v0_1.md` (merged via PR #408).

- **Current code baseline (§1):** Five slots (Core, Alive, Tail1, Tail2, Info); Core+Tail coupled; Op+Info same gate.
- **Agreed v0.2 direction (§2, §2a):** Node_Pos_Full (default); split Core+Tail only as fallback/calibration hypothesis; Node_Status single packet, full snapshot; lifecycle (bootstrap, 30s/300s); **hitchhiking not allowed**; Pos_Quality external [#359]; protocol numbers out of scope for #407 closure.

---

## 2) What was aligned

### packet_sets_v0_1.md

- **Issue:** Doc describes Node_Core_Tail, Node_Operational, Node_Informative without framing them as v0.1/current-code; readers could treat Op/Info as the active target.
- **Change:** Intro now states this doc defines the **v0.1 / current-code** packet set and remains for baseline and transition reference. Added one sentence: *Agreed v0.2 planning direction (Node_Pos_Full, Node_Status, no hitchhiking) is in packet_context_tx_rules_v0_1.md (#407).*
- **Reference:** Pointer to packet_context_tx_rules_v0_1.md (#407) was already present in §6 Related; left as is.

### tx_priority_and_arbitration_v0_1.md

- **Issue:** Priority table lists Node_Core_Pos, Alive, Node_Core_Tail, Node_Operational, Node_Informative with no indication that v0.2 replaces this with PosFull, Alive, Node_Status.
- **Change:** Added one italic line immediately after the table: *Table above describes **v0.1 / current-code** slots. Agreed v0.2 planning: three slots (Node_Pos_Full, Alive, Node_Status) and priority rules in packet_context_tx_rules_v0_1.md (#407).*
- **Reference:** Pointer in §5 Related already present; left as is.

### channel_utilization_budgeting_and_profile_strategy_s03_v0_1.md

- **Issue:** Definitions and merge/split table presented Split vs PosFull as equal choices; Ops/Info row used “OpsInfo” without tying to Node_Status or #407; no explicit “no hitchhiking” alignment.
- **Change:**  
  - **§2 Definitions (Packetization mode):** Split annotated as “current S03 / fallback”; Merged/PosFull annotated as “agreed v0.2 default per #407”.  
  - **§4 Merge vs split table (Ops/Info row):** Added “(v0.2: single **Node_Status** packet per #407, packet_context_tx_rules_v0_1); … No hitchhiking of status into PosFull.”
- **Reference:** packet_context_tx_rules_v0_1.md (#407) already in §9 artifact table; left as is.

---

## 3) What remains intentionally untouched

- **packet_sets_v0_1.md:** Full content of §1–§5 (Node_Core_Tail, Node_Operational, Node_Informative definitions, eligibility, discovery/conflicts, FW alignment) unchanged. No removal of v0.1 content; it is explicitly retained for baseline and transition reference.
- **tx_priority_and_arbitration_v0_1.md:** Full P0–P3 table and coalesce/expired_counter semantics unchanged. v0.1 policy remains documented; only framing added.
- **channel_utilization_budgeting:** Profile matrix (§3), throttling (§5), adaptive policy (§6), mesh piggyback (§7), role cadence (§8), TBD list (§10) unchanged. No change to mesh “piggyback/attach” (that is mesh-forwarding same-class attach, not status-into-PosFull hitchhiking).
- **Other WIP docs:** No scan of the rest of the repo; only the three requested adjacent docs were edited. Any other WIP that references split/merged packetization or Ops/Info was left for a later pass if needed.
- **#407 snapshot itself:** No edits; it remains the single authoritative source for packet context and TX rules.

---

## 4) References to packet_context_tx_rules_v0_1.md

- **packet_sets_v0_1.md:** §6 Related (existing) + new intro sentence (link to same doc).
- **tx_priority_and_arbitration_v0_1.md:** §5 Related (existing) + new sentence after §1 table (link to same doc).
- **channel_utilization_budgeting:** §9 artifact table (existing) + new inline links in §2 and §4.

All three docs now point to the #407 snapshot and frame v0.2 as the agreed direction where relevant.

---

## 5) Consistency conclusion

- **Misleading stale wording:** Addressed by (1) labelling v0.1/current-code in packet_sets and tx_priority, (2) labelling PosFull as agreed v0.2 default and split as fallback in channel_utilization, (3) tying Ops/Info to Node_Status and “no hitchhiking” in channel_utilization.
- **Split Core+Tail:** Now explicitly “fallback” / “current S03 / fallback”; not presented as an equally current planning direction.
- **Operational / Informative:** Now explicitly accompanied by a pointer to Node_Status and #407 where packet types are discussed (packet_sets intro, tx_priority table note, channel_utilization Ops/Info row).
- **Hitchhiking / packet mixing:** Not left open; channel_utilization Ops/Info row now states “No hitchhiking of status into PosFull,” consistent with #407.
- **Protocol numbering:** Not reintroduced as part of #407; no changes in adjacent docs to protocol/wire numbering.

**The WIP set is now internally consistent enough to leave #407 alone and move on.** The authoritative snapshot is the single reference for packet purpose, contents, creation, and send rules; adjacent docs frame v0.1 vs v0.2 clearly and point to that snapshot without rewriting or creating new semantics.
