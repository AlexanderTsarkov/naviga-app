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

- **expected_interval_s** = `max_silence_10s * 10`  
  The node’s advertised **max_silence_10s** (from Node_Informative or config) is used as “I must be heard at least this often”; expected_interval_s (seconds) is derived from it. Single numeric source for “expected presence interval.”
- **grace_s:** Hysteresis (seconds) to avoid literal timeout on jitter. **Value TBD;** document as constant or config. Follow-up issue: decide value and where it lives (FW / app / both).

---

## 3) S03 minimal required fields

- **last_seen_age_s** — derived at export; BLE snapshot.
- **is_stale** — derived at export; threshold above.
- **pos_age_s** — position age (independent axis).

**Note:** last_seen_ms is internal and **not** required in the BLE model if last_seen_age_s is exported; the app can derive age from last_seen_age_s for display. Internal FW keeps last_seen_ms as the anchor.

---

## 4) Scope note (WIP-only, not in minimal model)

- **aliveStatus**, **activityState**, **PositionQuality** remain WIP-only / Planned or Derived and are **not** part of this S03 minimal presence model. No promotion to canon in this doc.

---

## 5) Related

- [identity_naming_persistence_eviction_v0_1.md](identity_naming_persistence_eviction_v0_1.md) — S03 Identity/Naming block.
- [packet_sets_v0_1.md](packet_sets_v0_1.md), [tx_priority_and_arbitration_v0_1.md](tx_priority_and_arbitration_v0_1.md) — Packet and TX policy.
- Master table: field_key **is_stale** (supersedes is_grey); **last_seen_age_s**, **pos_age_s** as S03 minimal fields.
