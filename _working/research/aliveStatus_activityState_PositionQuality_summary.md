# aliveStatus, activityState, PositionQuality — summary (pasteable)

---

## 1) Docs say

**aliveStatus**
- **Canon contract:** `docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md` §3.2 — optional 1-byte in Alive payload; "V1-A may use only **0x00 = alive_no_fix**. All other values **reserved** for future use; receiver MUST treat as 0x00 for liveness." When present, payload 10 B; omission allowed (9 B).
- **Beacon hub:** `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` — Alive carries "optional aliveStatus(1)".
- **Master table:** Stubbed, HW; "Alive packet optional byte."
- **Meaning (docs):** Liveness / no-fix indicator for Alive packet only; not a NodeTable row field.

**activityState**
- **Canon policy:** `docs/product/areas/nodetable/policy/activity_state_v0.md` — derived activity/presence state: **Unknown** (no packet yet), **Active** (ageSec ≤ T_active), **Stale** (T_active < ageSec ≤ T_stale), **Lost** (ageSec > T_stale). Inputs: seq16, lastRxAt, ageSec (lastSeenAge); thresholds T_active, T_stale, T_lost parameterized (no numeric defaults in doc). "Domain exposes **activityState** and **lastSeenAge**; UI chooses how to render." "Presenting Stale / Lost: UI may present … dimmed, or 'stale' label … struck through, grey, or 'lost')."
- **Inventory:** `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md` — "Activity/presence state (derived). Defined by policy activity_state_v0. … Not sent (derived)."
- **source-precedence-v0 / snapshot-semantics-v0 / restore-merge-v0:** activityState listed as **Derived** (recomputed from lastSeen + policy); not persisted; "isGrey or equivalent" for UI.
- **presence_and_age_semantics_v0_1.md:** "**activityState** … remain WIP-only / Planned or Derived and are **not** part of this S03 minimal presence model."
- **Mobile audit (s02_230):** "Activity state (Unknown/Active/Stale/Lost) not derived or displayed. … App only shows **isGrey** (from FW) and **lastSeenAgeS** (raw). … **In V1-A this is intentional:** UX relies on firmware-derived **isGrey**."

**PositionQuality**
- **Canon policy:** `docs/product/areas/nodetable/policy/position_quality_v0.md` — "**PositionQuality** is a **derived** notion for UI and consumers: it is not a separate beacon field." Inputs: Core position, Tail-1 posFlags/sats, optional receiver-side age. Derived state: no position / position valid / optional quality band. "Only sent position-quality fields in v0 are Tail-1 **posFlags** and **sats**."
- **Inventory:** "Derived from Core position + Tail-1 posFlags/sats + optional receiver-side age. Sent components in v0: posFlags, sats (Tail-1 only)."
- **presence_and_age_semantics_v0_1:** "**PositionQuality** … remain WIP-only … not part of this S03 minimal presence model."

---

## 2) Code today

**Firmware**
- **aliveStatus:** Not in `NodeEntry`. Alive payload encoder/decoder in `firmware/protocol/alive_codec.h`: `AliveFields` has `has_status`, `alive_status`; `beacon_logic.cpp` always sets `alive.has_status = false` when encoding → **aliveStatus not sent on air**. Decoder supports optional byte (payload[9]) but value is not stored in NodeTable.
- **activityState:** **Not found.** No enum or field. Presence/staleness is a single **is_grey** (doc-renamed to **is_stale**) derived at BLE export from `last_seen_age_s > expected_interval_s + grace_s` (`node_table.cpp` `is_grey()`). No T_active/T_stale/T_lost or Unknown/Active/Stale/Lost in code.
- **PositionQuality:** **Not found.** No composite type or field. NodeEntry has **pos_flags**, **sats**, **pos_age_s** (and lat/lon, pos_valid); BLE snapshot exports these. No "PositionQuality" struct or enum.

**App**
- **aliveStatus:** Not in `NodeRecordV1` or BLE parser. **Not found.**
- **activityState:** Not in `NodeRecordV1`. App shows **isGrey** (BLE flags bit 0x04) and **lastSeenAgeS** (`connect_controller.dart` NodeRecordV1, `node_details_screen.dart` "Is grey", "Last seen age (s)"). No Active/Stale/Lost enum or threshold-based derivation.
- **PositionQuality:** Not in `NodeRecordV1`. App has **posAgeS**, **posValid**, lat/lon; no "PositionQuality" type. **Not found.**

---

## 3) Contract presence

| Field            | On-air encoding                    | BLE snapshot (26-byte)     |
|------------------|------------------------------------|----------------------------|
| **aliveStatus**  | **Yes** (Alive payload optional 1 B per alive_packet_encoding_v0). FW currently does **not** send it (has_status=false). | **No.** Not in NodeEntry or BLE record. |
| **activityState**| **No.** Policy-only derived state; not a wire field.        | **No.** BLE exports **is_grey** (flags bit 0x04) and **lastSeenAgeS**; no enum. |
| **PositionQuality** | **No.** Derived notion; sent *components* are posFlags, sats in Tail-1. | **No.** BLE exports pos_age_s, pos_valid, lat/lon, etc.; no composite PositionQuality. |

---

## 4) Decision relevance

- **aliveStatus:** Would only affect "liveness" interpretation of Alive (e.g. 0x00 = alive_no_fix). Not used in eviction, gating, or NodeTable logic today. **No concrete action found** that requires it (Alive is accepted and update lastRxAt without it).
- **activityState:** Policy says it drives **UI presentation** (Stale/Lost: dim, strike, grey). **Concrete action:** UI presentation of presence. Today that is done via **isGrey** (FW) + **lastSeenAgeS**; no separate activityState enum. Eviction (persistence-v0, FW) uses "stale" in the sense of lastSeenAge / is_grey, not the activityState enum.
- **PositionQuality:** Policy says derived for **UI/consumer** (show position, quality, "N sats"). **Concrete action:** UI display of fix quality. Today UI can derive from **posValid**, **posAgeS**, and (if exposed) posFlags/sats; no first-class PositionQuality type required.

**Conclusion (decision relevance):** No concrete **required** action today that depends on these three as first-class types. Current behavior uses **is_stale** (née is_grey) + **last_seen_age_s** + **pos_age_s** + Tail-1 **posFlags/sats** for presence and position-quality behavior.

---

## Recommendation

**Keep WIP only.** Do not promote aliveStatus, activityState, or PositionQuality to S03 implementation scope. Evidence:
- **aliveStatus:** In on-air contract but stubbed (not sent); no NodeTable or BLE use; no decision depends on it.
- **activityState:** Canon policy exists but implementation intentionally uses **is_stale** + lastSeenAgeS only (V1-A); presence_and_age_semantics_v0_1 explicitly leaves it out of the minimal model.
- **PositionQuality:** Canon policy defines derivation; sent parts (posFlags, sats) and pos_age_s are implemented; no need for a separate composite type for current behavior.

If a future change needs "Active/Stale/Lost" labels or aliveStatus on-air or a PositionQuality enum, that can be added then; current docs and master table already mark them as Derived/Stubbed/WIP-only.
