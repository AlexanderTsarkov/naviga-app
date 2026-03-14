# S03 packetization & TX policy — current state (research for #407)

**Purpose:** Research-only consolidation for planning issue [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407) (V0.1 radio packet context, formation & TX rules; packetization redesign v0.2). No code or canon changes.  
**Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351). **Related:** [#360](https://github.com/AlexanderTsarkov/naviga-app/issues/360), [#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358), [#359](https://github.com/AlexanderTsarkov/naviga-app/issues/359), [#356](https://github.com/AlexanderTsarkov/naviga-app/issues/356), [#363](https://github.com/AlexanderTsarkov/naviga-app/issues/363).

---

## 1) Current packet types and documented field membership

### 1.1 Canon (contracts + field_cadence_v0)

| Packet | msg_type | Purpose | Fields (canon) | Source |
|--------|----------|---------|----------------|--------|
| **Alive** | 0x02 | No-fix liveness | Common (9 B) + optional aliveStatus | [alive_packet_encoding_v0](docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md), [beacon_payload_encoding_v0](docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md) §3 |
| **Node_Core_Pos** | 0x01 | WHO + WHERE + freshness | Common + lat_u24(3) + lon_u24(3); 15 B payload, 17 B on-air | [beacon_payload_encoding_v0](docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md) §4.1 |
| **Node_Core_Tail** | 0x03 | Qualifies one Core sample | Common + ref_core_seq16(2) + optional posFlags(1) + optional sats(1); min 11 B payload, 13–15 B on-air | [tail1_packet_encoding_v0](docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md) §3 |
| **Node_Operational** | 0x04 | Dynamic runtime | Common + optional batteryPercent(1), uptimeSec(4); 9–14 B payload | [tail2_packet_encoding_v0](docs/product/areas/nodetable/contract/tail2_packet_encoding_v0.md) §3.2 |
| **Node_Informative** | 0x05 | Static/config | Common + optional maxSilence10s(1), hwProfileId(2), fwVersionId(2); 9–14 B payload | [info_packet_encoding_v0](docs/product/areas/nodetable/contract/info_packet_encoding_v0.md) §3.2 |

**Common prefix (all Node_*):** payloadVersion(1) + nodeId48(6) + seq16(2) = 9 B. [beacon_payload_encoding_v0](docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md) §3.

**Field → tier (canon):** [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §3 maps fields to Tier A (Core), B (Tail-1), C (Tail-2). Tier B includes posFlags, sats (Core_Tail) and uptimeSec (Operational packet); Tier C includes batteryPercent, maxSilence10s, hwProfileId, fwVersionId.

### 1.2 WIP (packet_sets_v0_1 + master table)

- **Node_Core_Tail v0.1:** ref_core_seq16(2) + **pos_quality16**(2) packed (fix_type, sats, accuracy_bucket, pos_flags_small). One Tail per Core sample; eligibility: when Core_Pos was sent and min_interval/coalesce allow. [packet_sets_v0_1](docs/product/wip/areas/nodetable/policy/packet_sets_v0_1.md) §1.
- **Node_Operational v0.1:** 4 B useful: battery16, radio8, uptime10m_u8 (packed). Eligibility: changed-gated + min_interval; snapshot (battery16, radio8, uptime10m) for coalesce; reboot exception. [packet_sets_v0_1](docs/product/wip/areas/nodetable/policy/packet_sets_v0_1.md) §2.
- **Node_Informative v0.1:** max_silence_10s(1) + hw_profile_id(2) + fw_version_id(2) + role_id (new). Eligibility: periodic + changed-boost; min_interval = max_silence10 or configured; MUST NOT send on every Operational. [packet_sets_v0_1](docs/product/wip/areas/nodetable/policy/packet_sets_v0_1.md) §3.

**Master table:** [packets_v0_1.csv](docs/product/wip/areas/nodetable/master_table/packets_v0_1.csv), [fields_v0_1.csv](docs/product/wip/areas/nodetable/master_table/fields_v0_1.csv). fields_v0_1 ties each field to packet_name_canon, tier, default_cadence, trigger_type.

### 1.3 Code reality (formation)

- **Core_Pos:** Formed when pos_valid and (time_for_min_interval or time_for_silence); one seq16 per TX; enqueued at P0. [firmware/src/domain/beacon_logic.cpp](firmware/src/domain/beacon_logic.cpp) ~L134–152.
- **Core_Tail:** Formed immediately after Core_Pos for that sample; tail.seq16 = next_seq16(), tail.ref_core_seq16 = core_seq; enqueued at P2. Same file ~L152–163.
- **Operational / Informative:** Formed when telemetry/informative data present; each gets own next_seq16(); enqueued at P3. Same file ~L186–221.

---

## 2) Send rules (Core_Pos, Core_Tail, Operational, Informative)

### 2.1 Canon (field_cadence_v0)

- **Core_Pos:** Only with valid GNSS fix. Every beacon tick when position valid. First-fix: first BeaconCore at next opportunity without min-displacement gate; then normal cadence. [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §2, §2.1.
- **Alive:** When no fix, within maxSilence. [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §2.
- **Core_Tail:** Every Tail-1 when position valid; one per Core sample; ref_core_seq16 links to that Core’s seq16. [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §2; [tail1_packet_encoding_v0](docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md) §1 (at most one Tail per Core_Pos).
- **Operational (0x04):** On change + at forced Core (every N Core beacons, N impl-defined). [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §2.2.
- **Informative (0x05):** On change + every 10 min; MUST NOT be sent on every Operational. [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §2.2.

### 2.2 WIP (packet_sets_v0_1, tx_priority_and_arbitration_v0_1)

- **Core_Tail:** Attached to Core sample; send when Core_Pos was sent and min_interval/coalesce allow; one Tail per Core sample (ref_core_seq16). [packet_sets_v0_1](docs/product/wip/areas/nodetable/policy/packet_sets_v0_1.md) §1.
- **Operational:** Changed-gated + min_interval; snapshot compare; reboot exception (one send even if snapshot unchanged). [packet_sets_v0_1](docs/product/wip/areas/nodetable/policy/packet_sets_v0_1.md) §2.
- **Informative:** Periodic + changed-boost; min_interval = max_silence10 or configured. [packet_sets_v0_1](docs/product/wip/areas/nodetable/policy/packet_sets_v0_1.md) §3.

### 2.3 Code (beacon_logic)

- Core/Alive: enqueue when “time_for_min” (with allow_core gate) or “time_for_silence” (cadence/silence deadline). [beacon_logic.cpp](firmware/src/domain/beacon_logic.cpp) ~L136–179.
- Operational/Informative: enqueue when telemetry/informative data present; no explicit “earliest_at” or “deadline” in code — gating is upstream (caller/role/cadence).

---

## 3) seq16 ownership, ref_core_seq16, coalesce keys, priority, changed-gated vs periodic

### 3.1 seq16

- **Canon:** One global per-node counter across all Node_* packet types; dedupe key (nodeId48, seq16). Common prefix in every packet. [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §8; [beacon_payload_encoding_v0](docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md) §3.
- **RX:** last_seq updated on any RX; last_core_seq16 updated only on Core_Pos RX. [seq_ref_version_link_metrics_v0_1](docs/product/wip/areas/nodetable/policy/seq_ref_version_link_metrics_v0_1.md) §1, §4.

### 3.2 ref_core_seq16

- **Meaning:** Tail-only; back-reference in Node_Core_Tail payload to the seq16 of the Core_Pos sample this Tail qualifies. [tail1_packet_encoding_v0](docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md) §0.1, §3.1; [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §2.
- **TX:** Sender sets ref_core_seq16 = core_seq when forming Tail right after that Core_Pos. One Tail per Core sample; Tail’s own seq16 = next_seq16() (distinct from ref_core_seq16). [beacon_logic.cpp](firmware/src/domain/beacon_logic.cpp) ~L152–158.
- **RX:** Apply Tail only if ref_core_seq16 == last_core_seq16. last_applied_tail_ref_core_seq16 used to dedupe (at most one Tail applied per Core sample). [seq_ref_version_link_metrics_v0_1](docs/product/wip/areas/nodetable/policy/seq_ref_version_link_metrics_v0_1.md); [packet_to_nodetable_mapping_s03](docs/product/wip/areas/nodetable/policy/packet_to_nodetable_mapping_s03.md) §3.4.

### 3.3 Coalesce keys (WIP)

- **tx_priority_and_arbitration_v0_1:** Coalesce key identifies “same logical update”; new payload with same key replaces pending. Per packet type: e.g. Operational = snapshot (battery, radio, uptime); Core_Tail = ref_core_seq16. [tx_priority_and_arbitration_v0_1](docs/product/wip/areas/nodetable/policy/tx_priority_and_arbitration_v0_1.md) §2.
- **Code:** One slot per packet type (kSlotCore, kSlotTail1, kSlotTail2, kSlotInfo). Enqueue to same slot = replace; replaced_count++. No explicit “coalesce_key” field in slot; key is implicit (slot index). New Tail replaces previous Tail in slot; new Operational/Info replace previous in their slots.

### 3.4 Priority classes

- **Canon/WIP:** P0 = Core_Pos, Alive; P1 = (reserved); P2 = Node_Core_Tail; P3 = Node_Operational, Node_Informative. [tx_priority_and_arbitration_v0_1](docs/product/wip/areas/nodetable/policy/tx_priority_and_arbitration_v0_1.md) §1.
- **Code:** P0_MUST_PERIODIC (Core, Alive), P2_BEST_EFFORT (Tail1), P3_THROTTLED (Tail2, Info). Ordering within P2 by TxBestEffortClass (BE_HIGH for Tail1); within P3 by replaced_count DESC then created_at_ms ASC. [beacon_logic.h](firmware/src/domain/beacon_logic.h) L31–59, L238–266; [beacon_logic.cpp](firmware/src/domain/beacon_logic.cpp).

### 3.5 Changed-gated vs periodic

- **Core_Pos / Alive:** Position / liveness; effectively periodic (min_interval, max_silence). First-fix is one-time bootstrap then periodic.
- **Core_Tail:** Tied to Core sample (one per Core); not “on change” of Tail fields alone — triggered by Core_Pos send.
- **Operational:** Changed-gated + min_interval (and at forced Core in canon). WIP adds snapshot mask compare and reboot exception.
- **Informative:** Periodic (e.g. 10 min) + on change; MUST NOT send on every Operational.

---

## 4) Contradictions and gaps (canon vs WIP vs code)

- **Stale TBD:** [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §5.4 was reported as having “seq8 vs seq16 TBD” in [seq_audit_s02](docs/product/wip/research/seq_audit_s02.md); canon §8 now states seq16. If §5.4 still exists, it’s a residual contradiction.
- **uptime tier:** field_cadence_v0 §3 table puts uptimeSec in Tier B but packet Node_Operational (0x04); Tail-1 tier is Core_Tail. So “Tier B” is used for both “Core_Tail packet” and “Operational packet fields that are higher priority than Tier C”. Terminology is mixed (tier vs packet).
- **v0 vs v0.1 payload:** Canon Tail-1 has posFlags(1)+sats(1); packet_sets_v0_1 has pos_quality16(2) packed. Canon Operational has batteryPercent(1)+uptimeSec(4); v0.1 has battery16+radio8+uptime10m (4 B). Doc states “canon v0 layout remains for backward compat”; v0.1 is “new packed layout” — no wire-format cutover doc in scope here.
- **earliest_at / deadline / preemptible / depends_on / budget_class:** Not defined in current canon or WIP. [#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407) asks for these; they are **gaps** for the “explicit rule table” DoD.
- **Coalesce key in code:** Docs say “coalesce_key” per type (e.g. ref_core_seq16 for Tail); code has one slot per type and replacement by re-enqueue. For Tail, the “key” is effectively the slot; the payload’s ref_core_seq16 identifies the Core sample. When a new Core_Pos is sent, a new Tail is enqueued and replaces the previous (one Tail slot). So semantics align (newest Tail wins; ref_core_seq16 in payload).

---

## 5) What merge Pos+Tail would impact

- **ref_core_seq16:** Today Tail carries ref_core_seq16 to link to a Core sample; RX applies Tail only if ref_core_seq16 == last_core_seq16. If Core and Tail content are merged into one “PosFull” packet (e.g. Core_Pos + ~4 B Tail essentials), there is no separate Tail packet. Then:
  - **ref_core_seq16** as a separate wire field becomes unnecessary for that packet (position and quality in same frame; seq16 in Common is the sample id).
  - **last_core_seq16** on RX still needed only when accepting position-bearing data (the single merged packet updates position + quality; one seq16 per update).
  - **last_applied_tail_ref_core_seq16** and “at most one Tail per Core” RX rule become obsolete for the merged format (no separate Tail to apply).
- **Seq ownership:** One seq16 per TX remains; merged packet would still consume one seq16. No change to global counter semantics.
- **TX queue / coalesce:** One slot for “position update” (Core+Tail merged) instead of two (Core + Tail). Coalesce: one pending “position” update per node; replacement when new position sample is produced. Current “Tail slot” and “one Tail per Core sample” TX invariant go away.
- **Priority:** Single P0 (or equivalent) “position” packet; no separate P2 Tail. Degrade-under-load: still preserve this packet over P3 (Operational/Informative).
- **Canon contracts:** [tail1_packet_encoding_v0](docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md), [beacon_payload_encoding_v0](docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md) would need a new variant or v0.2 for “PosFull” layout; Tail-1 as separate msg_type 0x03 could remain for backward compat or be deprecated by design.

---

## 6) What merge Operational+Informative would impact

- **Two packets today:** Node_Operational (0x04), Node_Informative (0x05); independent TX paths; different cadence (on-change+forced Core vs periodic+changed; MUST NOT send Info on every Operational). [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §2.2.
- **If merged into “Node_Ops_Info”:** One packet carrying both operational (battery, uptime, tx power, …) and informative (maxSilence10s, hwProfileId, fwVersionId, role_id) with a single cadence/rule set.
  - **Eligibility:** Would need a single rule: e.g. on any Op or Info field change + periodic floor (e.g. max of 10 min and min_interval), or two logical triggers combined (send when Op due OR Info due, with coalesce).
  - **Coalesce key:** One slot; key could be snapshot of (Op fields, Info fields) or type-level (one Ops_Info slot).
  - **MUST NOT send on every Operational:** Current constraint that Informative is not sent on every Operational disappears as a separate constraint; merged packet would define one schedule (e.g. “send when Op changed or Info changed or period elapsed” with one replacement policy).
  - **Priority:** Still P3 (or same low priority); drop under load first. [channel_utilization_budgeting_and_profile_strategy_s03_v0_1](docs/product/wip/areas/radio/policy/channel_utilization_budgeting_and_profile_strategy_s03_v0_1.md) already mentions “Ops/Info merge into OpsInfo” as option; P3 skip/expiry under load.
  - **Wire format:** New layout (e.g. common + op fields + info fields); [tail2_packet_encoding_v0](docs/product/areas/nodetable/contract/tail2_packet_encoding_v0.md) and [info_packet_encoding_v0](docs/product/areas/nodetable/contract/info_packet_encoding_v0.md) would need a combined contract or v0.2.

---

## 7) Traffic model (#360) and ToA

- **traffic_model_s03_v0_1:** Split (Core_Pos + Core_Tail) gives two packets per position update → U_pos ≈ 2 × rate × airtime. Merged (one packet per update) ≈ 218 ms vs 198 ms each for Core and Tail → lower total U_pos. [traffic_model_s03_v0_1](docs/product/wip/areas/radio/policy/traffic_model_s03_v0_1.md) §2.1, §4.
- **Conclusion there:** Split with p_tail=1 overloads channel (e.g. hunt 25 nodes U≈0.78; 10 dogs U≈0.95). Merged halves position load; merge is documented as future option. No format change in S03 yet; #407 is to capture “packetization v0.2 proposal” and TX rules.

---

## 8) Open questions (no solutions as facts)

1. **PosFull (Core + ~4 B Tail essentials):** Exact field set for “Tail essentials” in merged packet (e.g. fix_type, sats, accuracy_bucket, pos_flags_small or subset); impact on RX last_core_seq16 / last_applied_tail semantics; backward compat with existing 0x01 and 0x03.
2. **Node_Ops_Info:** Single cadence/eligibility rule and coalesce key; whether “periodic + on Op change + on Info change” can be one rule without over-sending Info; impact on “MUST NOT send on every Operational” and provisioning/baseline.
3. **TX rule table:** Explicit triggers, earliest_at, deadline, coalesce_key, preemptible, depends_on, budget_class — currently only priority (P0–P3), coalesce_key (per type), and replaced_count semantics exist; remaining concepts are not yet defined.
4. **Stale text:** field_cadence_v0 §5.4 “seq8 vs seq16 TBD” — confirm if still present and remove or align with §8.
5. **Tier vs packet naming:** Clear mapping Tier A/B/C → packet types and field membership to avoid B meaning both “Core_Tail packet” and “Operational packet’s uptime”.

---

## 9) File and issue reference index

| Topic | Canon | WIP | Code |
|-------|-------|-----|------|
| Packet types, byte layout | beacon_payload_encoding_v0, tail1/tail2/info_packet_encoding_v0, alive_packet_encoding_v0 | packet_sets_v0_1, packets_v0_1.csv, fields_v0_1.csv | — |
| Cadence, tiers, first-fix | field_cadence_v0 | ootb_autonomous_start_s03 | beacon_logic.cpp (time_for_min, time_for_silence) |
| TX priority, coalesce, expired_counter | — | tx_priority_and_arbitration_v0_1 | beacon_logic.h (.cpp): TxPriority, replaced_count, slot replace, starvation +1 |
| Seq/ref, link metrics | — | seq_ref_version_link_metrics_v0_1 | beacon_logic: next_seq16(), ref_core_seq16 in Tail |
| Packet→NodeTable mapping | — | packet_to_nodetable_mapping_s03 | — |
| Traffic / ToA / merge | — | traffic_model_s03_v0_1, channel_utilization_budgeting_and_profile_strategy_s03_v0_1 | — |

All paths above are relative to repo root. Issues: #351 (umbrella), #407 (this planning), #360 (traffic), #358 (mapping), #359 (GNSS Tail), #356 (radio settings), #363 (if applicable).
