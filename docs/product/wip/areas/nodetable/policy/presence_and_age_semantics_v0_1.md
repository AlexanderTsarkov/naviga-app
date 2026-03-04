# NodeTable — Presence and age semantics (S03 minimal model v0.1)

**Status:** WIP. Frozen S03 minimal presence/age model. **Parent:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352).

This document defines the **S03 minimal** presence and age model for NodeTable: internal anchors, derived BLE-exported values, threshold source-of-truth, and self special-case. Naming: **is_stale** (replaces **is_grey** in docs and master table). No firmware code or behavior change; documentation only.

---

## 1) Definitions

### A) FW internal

- **last_seen_ms:** Monotonic uptime (ms) of the last **presence event** for this entry. Single source of truth for “when we last had a presence signal” from this node.
- **Presence event:**
  - **Remote entries:** RX-only updates (any accepted Node_* packet from that node updates last_seen_ms).
  - **Self entry:** TX heartbeat updates are **allowed** (e.g. Core_Pos, Alive, or other presence-bearing TX) so that self does not appear stale in the UI when the node is transmitting but not receiving its own RX. Exact which TX counts as presence is implementation-defined; follow-up issue may align.

### B) BLE exported (derived at snapshot time)

- **last_seen_age_s:** `floor((now_ms - last_seen_ms) / 1000)` computed at snapshot/export time. Exported in BLE for convenience; **not** a second source of truth (derived from last_seen_ms).
- **is_stale:** `last_seen_age_s > (expected_interval_s + grace_s)`. Boolean; exported in BLE (e.g. flags bit 0x04). Replaces legacy name **is_grey** in S03.

### C) Position axis (independent from presence)

- **pos_age_s:** Age of the last known **position** sample (seconds). Independent from presence; position freshness is a separate axis from “last heard from” presence.

---

## 2) Threshold source-of-truth

- **expected_interval_s** = `max_silence_10s * 10` (seconds).  
  The node’s advertised **max_silence_10s** (from Node_Informative or role profile) is used as “I must be heard at least this often”; expected_interval_s is derived from it. Single numeric source for “expected presence interval.” See [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) (maxSilence10s), [field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md).
- **grace_s** (seconds): Hysteresis to avoid false stale due to airtime/jitter; mandatory Core_Pos/Alive may arrive slightly late. **Canonical formula (FW-owned, not user-configurable):**
  - `grace_s = clamp(3, 30, round(expected_interval_s * 0.10))`  
  Units: seconds. Bounds: minimum 3 s, maximum 30 s. Derived from expected_interval_s only; embedded-first policy constant.
- **is_stale** = `(last_seen_age_s > expected_interval_s + grace_s)`.

**Why FW-owned:** grace_s is a **firmware policy constant** derived from expected_interval_s. It is **not** user-configurable; embedded devices own presence/staleness policy so that BLE snapshot and future UI/tx power escalation/activity state can rely on a single source of truth without app/FW drift. Future actions after max_silence (e.g. UI “node missing” or tx power escalation) can build on this threshold; this doc does not implement them.

---

## 3) S03 minimal required fields

- **last_seen_age_s** — derived at export; BLE snapshot.
- **is_stale** — derived at export; threshold above.
- **pos_age_s** — position age (independent axis).

**Note:** last_seen_ms is internal and **not** required in the BLE model if last_seen_age_s is exported; the app can derive age from last_seen_age_s for display. Internal FW keeps last_seen_ms as the anchor.

---

## 4) Not in S03 (scope exclusions)

The following are **not** promoted to S03 and remain WIP-only / policy-only:

- **aliveStatus:** In Alive packet contract (optional 1 B) but **unused/stubbed** in S03 (not sent; not in NodeTable or BLE snapshot). No S03 behaviour depends on it.
- **activityState:** Policy-only derived concept (Unknown/Active/Stale/Lost from activity_state_v0). Not implemented as an enum in FW or app; not on-air; not in BLE snapshot.
- **PositionQuality:** Policy-only derived concept (position_quality_v0: Core + Tail-1 + age). Not implemented as a composite type; sent *components* are posFlags/sats in Tail-1.

**S03 minimal presence model** is covered by: **is_stale**, **last_seen_age_s**, **pos_age_s**, and Tail-1 **posFlags** / **sats**. UI can rely on these for presence and position-quality display; no need for aliveStatus, activityState, or PositionQuality as first-class fields in S03.

---

## 5) Related

- [identity_naming_persistence_eviction_v0_1.md](identity_naming_persistence_eviction_v0_1.md) — S03 Identity/Naming block.
- [packet_sets_v0_1.md](packet_sets_v0_1.md), [tx_priority_and_arbitration_v0_1.md](tx_priority_and_arbitration_v0_1.md) — Packet and TX policy.
- [field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md) — Core/Alive cadence; expected_interval_s source (max_silence_10s).
- [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) — maxSilence10s per role; OOTB defaults.
- [ootb_autonomous_start_s03](../../firmware/policy/ootb_autonomous_start_s03.md) — OOTB trigger/sequence; is_stale/last_seen_age_s derived from presence.
- Master table: field_key **is_stale** (supersedes is_grey); **last_seen_age_s**, **pos_age_s** as S03 minimal fields. grace_s defined in this doc §2.
