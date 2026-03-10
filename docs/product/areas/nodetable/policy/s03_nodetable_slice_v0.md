# S03 NodeTable slice v0 (scope lock)

**Status:** Canon (policy).  
**Parent:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352).

This document is the **index/slice** for the S03 NodeTable scope. It consolidates what was finalized in WIP and points to exact sources and the master table so promotion and chunk-by-chunk implementation can proceed without losing scope. No new field decisions; documentation and links only.

---

## 1) Purpose — what "slice" means

- **Slice** = the bounded set of NodeTable behaviour and data that is **in scope for S03**: decisions are frozen in policy docs, master table is the single field/packet reference, and execution is tracked in linked issues.
- This doc does **not** duplicate policy content; it **links** to the canon policy docs and the generated master table. Update links when paths or scope change.

---

## 2) Source of truth / Entry points

| What | Location |
|------|----------|
| **Field-level entry** (master table index) | [nodetable_field_map_v0.md](nodetable_field_map_v0.md) — single entry for #352; declares CSV/XLSX as field truth. |
| Identity, naming, persistence, eviction | [identity_naming_persistence_eviction_v0.md](identity_naming_persistence_eviction_v0.md) |
| Presence and age (is_stale, last_seen_age_s, pos_age_s) | [presence_and_age_semantics_v0.md](presence_and_age_semantics_v0.md) |
| Seq, ref, payload version, link metrics | [seq_ref_version_link_metrics_v0.md](seq_ref_version_link_metrics_v0.md) |
| Packet sets (Core/Tail/Operational/Informative) | [packet_sets_v0.md](packet_sets_v0.md) |
| TX priority and arbitration (P0–P3, expired_counter) | [tx_priority_and_arbitration_v0.md](tx_priority_and_arbitration_v0.md) |
| Master table (fields, packets, sources) | [master_table/nodetable_master_table_v0_1.xlsx](../../../wip/areas/nodetable/master_table/nodetable_master_table_v0_1.xlsx) · [master_table/fields_v0_1.csv](../../../wip/areas/nodetable/master_table/fields_v0_1.csv) |

---

## 3) In-scope S03 (bullets)

- Persistence v1 + eviction v1 (NodeTable snapshot NVS, restore, tier-aware eviction) — [#367](https://github.com/AlexanderTsarkov/naviga-app/issues/367)
- Self **node_name** over BLE + NVS (read/write, self-only) — [#368](https://github.com/AlexanderTsarkov/naviga-app/issues/368)
- RSSI append parse and update **last_rx_rssi** in NodeTable — [#375](https://github.com/AlexanderTsarkov/naviga-app/issues/375)
- SNR NA sentinel plumbing (E22 UNSUPPORTED; store/export **snrLast** with sentinel) — [#376](https://github.com/AlexanderTsarkov/naviga-app/issues/376)
- **grace_s** decision (value and where it lives: FW / app / both) — [#371](https://github.com/AlexanderTsarkov/naviga-app/issues/371) (or equivalent)
- Self presence TX update points (which TX counts as presence for self) — [#372](https://github.com/AlexanderTsarkov/naviga-app/issues/372) (or equivalent)
- App: local phone name storage + authoritative-replaces-local rule (optional; can be future) — [#369](https://github.com/AlexanderTsarkov/naviga-app/issues/369)

---

## 4) Out-of-scope S03

- **networkName** (broadcast/session name)
- **Ownership / claim / pairing** (device binding, BLE write-protect, PIN, lock)
- **Over-air name propagation** (remote node_name from session/Informative in Sxx)
- **activityState**, **PositionQuality**, **aliveStatus** as first-class promoted fields — remain policy-derived; UI uses is_stale, last_seen_age_s, pos_age_s, posFlags/sats

---

## 5) Promotion to canon rule

**WIP → canon after:** (1) docs merged to canon paths, (2) linked execution issues implemented (or explicitly deferred), (3) tabletop checklist satisfied where applicable.

Update this slice doc to point to canon policy paths and to close or re-label execution issues when promoting.

---

## 6) Execution mapping (issue links)

| Issue | Scope |
|-------|--------|
| [#367](https://github.com/AlexanderTsarkov/naviga-app/issues/367) | FW: NodeTable persistence v1 + eviction v1 |
| [#368](https://github.com/AlexanderTsarkov/naviga-app/issues/368) | FW/BLE: self node_name read/write + NVS (self-only) |
| [#369](https://github.com/AlexanderTsarkov/naviga-app/issues/369) | App: local phone name + authoritative-replaces-local (optional) |
| [#371](https://github.com/AlexanderTsarkov/naviga-app/issues/371) | grace_s value and placement (FW/app/both) |
| [#372](https://github.com/AlexanderTsarkov/naviga-app/issues/372) | Self presence TX update points |
| [#375](https://github.com/AlexanderTsarkov/naviga-app/issues/375) | FW: Parse E22 RSSI append, update last_rx_rssi |
| [#376](https://github.com/AlexanderTsarkov/naviga-app/issues/376) | FW: snrLast NA/UNSUPPORTED sentinel, plumb through snapshots |

(Others may be added as S03 execution is broken down; link from this slice and from [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) / [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352).)
