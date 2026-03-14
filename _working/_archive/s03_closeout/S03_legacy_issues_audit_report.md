# S03 Legacy coding issues — audit report

**Date:** 2026-03-10  
**Context:** New execution plan [#416](https://github.com/AlexanderTsarkov/naviga-app/issues/416) + sub-issues #417–#426.  
**Legacy issues audited:** #364, #365, #367, #368, #369, #375, #376, #391, #392, #393, #394.

---

## Summary

| Disposition       | Count | Issues |
|-------------------|-------|--------|
| ALREADY DONE      | 1     | #365 (PR #366 merged) |
| MERGE DETAILS     | 7     | #367, #368, #375, #376, #391, #392, #393, #394 |
| SUPERSEDED        | 2     | #364 (umbrella), #369 (app/mobile out of scope) |

---

## Report table

| Legacy | Title | State | Disposition | Covered by | Unique notes to preserve | Action taken |
|--------|--------|--------|-------------|------------|---------------------------|--------------|
| [#364](https://github.com/AlexanderTsarkov/naviga-app/issues/364) | S03 NodeTable producers & packers v0.1 (dev umbrella) | CLOSED | **SUPERSEDED** | #420, #422, #419 | (Umbrella only; scope split across packet formation, packetization, NodeTable fields.) | Comment added; closed |
| [#365](https://github.com/AlexanderTsarkov/naviga-app/issues/365) | FW TX queue P3 throttling + hybrid expired_counter | CLOSED | **ALREADY DONE** | #420 | Implementation merged (PR #366); tabletop validation deferred. | Comment added; closed |
| [#367](https://github.com/AlexanderTsarkov/naviga-app/issues/367) | NodeTable persistence v1 + eviction v1 | CLOSED | **MERGE DETAILS** | #418 | restore-merge-rules-v0; eviction T0/T1/T2 tiers, pin/oldest-grey-first. | Notes copied to #418; comment added; closed |
| [#368](https://github.com/AlexanderTsarkov/naviga-app/issues/368) | self node_name read/write + NVS persistence | CLOSED | **MERGE DETAILS** | #418 | Self node_name persisted in NVS (self-only); BLE read/write is S04. | Notes copied to #418; comment added; closed |
| [#369](https://github.com/AlexanderTsarkov/naviga-app/issues/369) | App: local phone name + authority rule | CLOSED | **SUPERSEDED** | — | Out of S03 execution scope (#416); app/mobile deferred to S04. | Comment added; closed |
| [#375](https://github.com/AlexanderTsarkov/naviga-app/issues/375) | Parse E22 RSSI append, update last_rx_rssi | CLOSED | **MERGE DETAILS** | #421 | Parse E22 RX RSSI append/API; write last_rx_rssi on any accepted RX. | Notes copied to #421; comment added; closed |
| [#376](https://github.com/AlexanderTsarkov/naviga-app/issues/376) | Store snrLast as NA/UNSUPPORTED sentinel for E22 | CLOSED | **MERGE DETAILS** | #421 | E22 has no SNR; use sentinel (e.g. int8 127 or -128); plumb through BLE snapshot. | Notes copied to #421; comment added; closed |
| [#391](https://github.com/AlexanderTsarkov/naviga-app/issues/391) | RadioProfile pointer persistence + USER profile skeleton | CLOSED | **MERGE DETAILS** | #424 | NVS rprof_ver, rprof_cur, rprof_prev; USER profile record skeleton (key pattern rprof_u_<id>_*); FACTORY id 0 virtual. | Notes copied to #424; comment added; closed |
| [#392](https://github.com/AlexanderTsarkov/naviga-app/issues/392) | Apply tx_power_baseline_step for E220 at boot (OOTB MIN) | CLOSED | **MERGE DETAILS** | #423, #424 | step 0 = 21 dBm OOTB; T30D 0..3, T33D 0..4 clamp; Phase A/B apply; FACTORY default step 0. | Notes copied to #424; comment added; closed |
| [#393](https://github.com/AlexanderTsarkov/naviga-app/issues/393) | Expose observable fault state (boot_config_result) | CLOSED | **MERGE DETAILS** | #423 | Ok/Repaired/RepairFailed for Phase A verify-and-repair; internal status/telemetry; no brick on RepairFailed. | Notes copied to #423; comment added; closed |
| [#394](https://github.com/AlexanderTsarkov/naviga-app/issues/394) | Tests for RadioProfile mapping and TX power contract | CLOSED | **MERGE DETAILS** | #424 | channel_slot 0..83, slot 0 reserved; rate_tier LONG clamp; tx_power step0=MIN, level count; baseline not overwritten by runtime. | Notes copied to #424; comment added; closed |

---

## Mapping to new execution issues

| New issue | Legacy issues merged |
|-----------|----------------------|
| #418 (NodeTable persistence snapshot + restore) | #367, #368 |
| #420 (Packet formation per TX rules) | #364 (partial), #365 (historical) |
| #421 (RX semantics apply rules) | #375, #376 |
| #423 (Boot A/B/C + OOTB) | #392 (partial), #393 |
| #424 (Radio profile baseline) | #391, #392, #394 |

---

## Canon reference note

- **restore-merge-rules-v0:** [#367](https://github.com/AlexanderTsarkov/naviga-app/issues/367) references `docs/product/wip/areas/nodetable/policy/restore-merge-rules-v0.md`. That doc exists under WIP; canon snapshot/restore policy is in `identity_naming_persistence_eviction_v0` and related. No doc edit in this audit; execution #418 should reference restore/merge rules (canon or WIP) as needed.

---

## Actions performed

1. Comment added to each legacy issue with disposition and link(s) to covering #41x.
2. All legacy issues closed as superseded (or completed where applicable).
3. Unique notes copied into #418, #421, #423, #424 under "Legacy inputs" (see below).
