# NodeTable — GNSS Tail completeness and budgets (S03)

**Status:** WIP. S03 planning/spec. **Issue:** [#359](https://github.com/AlexanderTsarkov/naviga-app/issues/359) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351).

This doc defines the **semantics (meaning + encoding rules)** for the GNSS/position quality containers in Core_Pos and Core_Tail, Tail completeness expectations, packet size/budget constraints, and interpretation guidance for UI (S04). It does **not** move fields between Core and Tail or redesign packet formats; container placement is fixed. Sources: [packet_sets_v0_1](../policy/packet_sets_v0_1.md) (pos_quality16), [tail1_packet_encoding_v0](../../../../areas/nodetable/contract/tail1_packet_encoding_v0.md), [beacon_payload_encoding_v0](../../../../areas/nodetable/contract/beacon_payload_encoding_v0.md).

**Locked (S03):** Core_Pos is sent only when fix ≥ 2D; without 2D fix we send Alive (no position). Core/Tail split is fixed; Core is minimal for reliability; Tail carries additional attributes if received. Channel/radio: packets kept small; Tail may be lost at long range; Core must remain self-sufficient for map updates.

---

## 1) pos_fix_type (3 bits)

- **Enum values (S03):**

| Value | Name     | Meaning |
|-------|----------|---------|
| 0     | NO_FIX   | No GNSS fix; position invalid. |
| 1     | FIX_2D   | 2D fix (lat/lon valid). |
| 2     | FIX_3D   | 3D fix (lat/lon/alt valid; alt may be omitted in Core for v0). |
| 3–7   | RESERVED | Reserved for future use. |

- **Rule:** Core_Pos is sent **only** when fix_type ≥ FIX_2D. NO_FIX may exist as “unknown/default” in NodeTable before first Core_Pos (e.g. entry just created); no Core_Pos is transmitted until fix ≥ 2D. Without 2D fix, the node sends **Alive** only (no position). See [beacon_payload_encoding_v0](../../../../areas/nodetable/contract/beacon_payload_encoding_v0.md) §3.1.
- **UI guidance (S04):** 2D vs 3D can influence confidence display (e.g. 3D = higher confidence icon or smaller uncertainty circle). Not implemented in S03.

---

## 2) pos_sats (6 bits)

- **Definition:** “Satellites used in solution.” Range **0..63**. Value 0 = not present / unknown.
- **Source:** From GNSS service/diag (e.g. u-blox NAV-PVT or equivalent “numSV” / satellites used). Filled when Tail is sent with a valid fix.
- **UI interpretation (guidance, not normative):** Recommended thresholds for display quality bands:
  - **&lt; 5:** weak (e.g. dimmed icon or “low accuracy” hint).
  - **5–7:** ok (normal display).
  - **≥ 8:** good (e.g. full confidence icon).
  Exact thresholds are **guidance**; FW may compute; UI may refine in S04.

---

## 3) pos_accuracy_bucket (3 bits)

- **Definition:** Eight buckets mapping horizontal accuracy (e.g. **hAcc** from GNSS, meters) into a 3-bit value. Used as a **trust signal**; UI uses it for circle size/opacity and degraded state.

| Bucket (value) | Horizontal accuracy range (meters) | Notes |
|----------------|------------------------------------|--------|
| 0 | unknown / not available | 0 = not present or hAcc unavailable. |
| 1 | &gt; 50 | Very coarse. |
| 2 | 20–50 | Coarse. |
| 3 | 10–20 | Moderate. |
| 4 | 5–10 | Good. |
| 5 | 2–5 | Very good. |
| 6 | 1–2 | Excellent. |
| 7 | &lt; 1 | Best. |

- **Rule:** Bucket is derived from GNSS hAcc (or equivalent) at TX time. Receiver and UI use bucket for interpretation only; Core position is not invalidated by bucket value. **UI guidance:** Use bucket for accuracy circle radius and/or opacity (e.g. higher bucket = smaller circle, higher opacity).

---

## 4) pos_flags_small (4 bits)

- **Bit layout (normative):**

| Bit | Name | Meaning |
|-----|------|---------|
| 0 | **pos_valid** | 1 = fix_type ≥ FIX_2D at TX time; 0 = no valid fix. |
| 1 | **pos_moving** | 1 = moving above threshold; 0 = below threshold or unknown. |
| 2 | **pos_estimated** | 1 = position not pure GNSS (DR/override/simulated); 0 = pure GNSS. S03 usually 0. |
| 3 | **pos_degraded** | 1 = low trust (accuracy and/or sats indicate degraded); 0 = not degraded. |

- **pos_moving source:** Derived from speed (e.g. ground speed from GNSS) or displacement over time. **Suggested default threshold (guidance):** speed &gt; 0.5 m/s (or equivalent) → pos_moving = 1. FW may use a different threshold; document in FW if different.
- **pos_degraded derivation (guidance):** Suggested default: **degraded = 1** if (accuracy_bucket ≤ 3 **OR** pos_sats &lt; 5). Exact thresholds are **guidance**; FW may compute; UI may refine. Purpose: signal “show with lower confidence” without revoking position.

---

## 5) Tail completeness contract

- **Which GNSS quality attributes live in Tail:** fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small (packed in **pos_quality16** in v0.1 Tail; see [packet_sets_v0_1](../policy/packet_sets_v0_1.md)). Optionally ref_core_seq16 (Core linkage). These are **nice-to-have** for interpretation; they are **not** required for basic map update.
- **Receiver behavior if Tail is missing:** Core_Pos alone is **sufficient** for map update. If Core_Tail is never received or is lost, the receiver keeps the position from Core_Pos; quality/confidence may be shown as “unknown” or default. Tail **improves** interpretation (accuracy circle, icon, degraded state) but does not change the validity of the Core position. See [rx_semantics_v0](../../../../areas/nodetable/policy/rx_semantics_v0.md): Tail MUST NOT revoke or invalidate Core position.
- **Core/Tail pairing:** Tail carries **ref_core_seq16**; receiver applies Tail payload **only if** ref_core_seq16 == last_core_seq16 for that node. On **mismatch** (e.g. stale or reordered Tail): **ignore** Tail payload; keep Core position and do not apply Tail quality fields. See [tail1_packet_encoding_v0](../../../../areas/nodetable/contract/tail1_packet_encoding_v0.md), [packet_to_nodetable_mapping_s03](../policy/packet_to_nodetable_mapping_s03.md) §3.4.

---

## 6) Budgets

- **Principle:** Keep **Core_Pos minimal** (lat/lon + common prefix only). Tail may be dropped at long range or under congestion; **do not add fields to Core** without an explicit product decision. Core_Pos is **budget-critical** for reliability and airtime.
- **Existing constraints:** Core_Pos payload = 15 B (9 common + 6 lat/lon); Tail-1 min 11 B, with posFlags+sats up to 13 B (v0); v0.1 pos_quality16 = 2 B in Tail. See [beacon_payload_encoding_v0](../../../../areas/nodetable/contract/beacon_payload_encoding_v0.md) §4, [tail1_packet_encoding_v0](../../../../areas/nodetable/contract/tail1_packet_encoding_v0.md). Budget constraints exist; Core_Pos size is fixed for v0; Tail remains optional and loss-tolerant.

---

## 7) Interpretation guidance requirement (UI + future logic)

- **Purpose:** Describe how to translate **fix_type** + **accuracy_bucket** + **pos_sats** + **pos_flags_small** into user-visible status. This is a **requirement for S04 UI** (and future FW logic that may expose aggregated “quality”); **not implemented in S03**.
- **Suggested mapping (guidance):**
  - **Icon only vs icon + accuracy circle:** Use fix_type (2D/3D) and accuracy_bucket to choose icon and optional circle radius. e.g. NO_FIX → no position icon; FIX_2D + bucket 0–2 → icon + large circle; FIX_3D + bucket 5–7 → icon + small circle or no circle.
  - **Degraded coloring:** If **pos_degraded = 1** (or derived from bucket/sats), show position with reduced opacity or a “degraded” color (e.g. amber/grey). Do not confuse with **is_stale** (presence): “stale” = no recent packet; “degraded” = low trust in this sample. See [presence_and_age_semantics_v0_1](../policy/presence_and_age_semantics_v0_1.md).
  - **Moving indicator (optional):** If **pos_moving = 1**, UI may show a “moving” indicator (e.g. small arrow or pulse). Optional; not required for S04.
- **Label:** This section is **guidance/requirement for S04 UI**; S03 does not implement UI or BLE protocol.

---

## 8) Cross-links

| Doc / issue | Description |
|-------------|-------------|
| [#359](https://github.com/AlexanderTsarkov/naviga-app/issues/359) | This task (GNSS Tail completeness & budgets). |
| [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) | S03 umbrella planning. |
| [packet_sets_v0_1](../policy/packet_sets_v0_1.md) | pos_quality16 packing (fix_type, sats, accuracy_bucket, pos_flags_small); v0.1 Tail. |
| [tail1_packet_encoding_v0](../../../../areas/nodetable/contract/tail1_packet_encoding_v0.md) | Tail-1 byte layout; ref_core_seq16; v0 posFlags/sats. |
| [beacon_payload_encoding_v0](../../../../areas/nodetable/contract/beacon_payload_encoding_v0.md) | Core_Pos (fix only when valid); Core/Tail split. |
| [packet_to_nodetable_mapping_s03](../policy/packet_to_nodetable_mapping_s03.md) | Packet → NodeTable mapping; Core_Tail fields applied on match. |
| [ootb_autonomous_start_s03](../../firmware/policy/ootb_autonomous_start_s03.md) | When Core_Pos/Tail are sent (fix valid, first-fix bootstrap). |
| [presence_and_age_semantics_v0_1](../policy/presence_and_age_semantics_v0_1.md) | is_stale vs position quality; do not confuse “stale” with “no fix”. |
| [field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md) | Tail-1 cadence; Core/Tail timing. |
| [radio_user_settings_review_s03](../../radio/policy/radio_user_settings_review_s03.md) ([#356](https://github.com/AlexanderTsarkov/naviga-app/issues/356)) | Radio/channel context; packets small for range. |
