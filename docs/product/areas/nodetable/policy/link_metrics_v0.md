# NodeTable — Link metrics v0 (Policy)

**Status:** Canon (policy).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy canonizes **receiver-derived** link metrics for NodeTable v0: last-sample and recent-window estimates that reflect **current** link state, not lifetime history. All semantics are derived from accepted packets; no new sent fields or beacon changes. Domain truth is this doc and the referenced contracts; no dependency on OOTB or UI as normative source.

---

## 1) Goal / Non-goals

**Goal:** Define canonical receiver-derived link metrics (RSSI/SNR last + recent-window) and update rules so NodeTable and UI can reflect *current* link state consistently, without implying lifetime averages.

**Non-goals:**
- No code or implementation in this doc.
- No new sent fields; no beacon or encoding changes.
- No mesh or power-control algorithms.
- No dependency on OOTB or UI as product truth.

---

## 2) Scope (v0)

### 2.1 Receiver-derived fields (canon)

| Field | Meaning | Notes |
|-------|---------|--------|
| **rssiLast** | RSSI from the **last accepted** packet from this node (e.g. dBm). | From RX; overwritten on each accepted packet. See [link-telemetry-minset-v0](../contract/link-telemetry-minset-v0.md) Table A. |
| **snrLast** | SNR from the **last accepted** packet from this node (e.g. dB). | From RX; overwritten on each accepted packet. |
| **rssiRecent** | **Recent-window** estimate of RSSI (bounded window). **NOT** lifetime average. | Window definition is policy-defined (TBD). May be expressed relative to cadence / forced Core / max silence; no numeric defaults in v0. |
| **snrRecent** | **Recent-window** estimate of SNR (bounded window). **NOT** lifetime average. | Same as rssiRecent; window is parameterized. |

**Naming:** v0 uses **Recent** (not Avg) to avoid “lifetime average” confusion. Metrics should reflect current link state over a bounded recent window, not full history.

### 2.2 Optional diagnostics (derived)

| Field | Meaning | Notes |
|-------|---------|--------|
| **rxDuplicatesCount** | Count of packets treated as duplicates (same seq16) in the current or recent window. | Policy-defined; no thresholds in v0. Diagnostic only. |
| **rxOutOfOrderCount** | Count of packets treated as out-of-order (older seq16) in the current or recent window. | Policy-defined; no thresholds in v0. Diagnostic only. |

---

## 3) Update rules (high-level)

- **Only on accepted packets:** Link metrics (rssiLast, snrLast, and inputs to rssiRecent/snrRecent) are updated only when a packet is **accepted** (passes version/node checks and any receiver policy). Discarded packets do not update metrics.
- **Duplicate seq16:** When a packet has the same seq16 as the last accepted Core seq for that node, it may refresh **lastRxAt** and **rssiLast / snrLast** (receiver “heard” the node again), but must **not** be treated as a new position sample. See [activity_state_v0](activity_state_v0.md) §4.2 (seq16 duplicates).
- **Out-of-order:** Per [activity_state_v0](activity_state_v0.md) §4.3, conservative handling applies; optional refresh of lastRxAt/rssiLast/snrLast for slightly delayed packets is implementation-defined. Do not use older seq to overwrite newer position or telemetry.
- **Recent window:** rssiRecent and snrRecent are derived from a **bounded** recent window (e.g. last N packets or last T seconds). Window size and aggregation (e.g. mean, EWMA over window) are **parameterized**; no magic numbers in v0.

---

## 4) Open questions

- **Window definition:** Exact window for rssiRecent/snrRecent (time-based vs count-based; relationship to expected cadence / forced Core / maxSilence) to be defined by policy or implementation; document when stable.
- **Persistence:** Whether rssiRecent/snrRecent are persisted or recomputed after restore (e.g. from lastRxAt + stored rssiLast/snrLast history) is implementation-defined; v0 does not require persistence of the recent estimate.
- **Relationship to minset:** [link-telemetry-minset-v0](../contract/link-telemetry-minset-v0.md) Table A lists rssiAvg/snrAvg (EWMA). For v0 **current link** semantics, this policy canonizes **rssiRecent/snrRecent** (bounded window). Minset may retain Avg for backward compatibility or other use; NodeTable v0 “current link” state uses Recent per this doc.

---

## 5) Related

- **Activity state:** [activity_state_v0](activity_state_v0.md) — lastRxAt update, seq16 duplicate/out-of-order rules (no new position sample on duplicate).
- **Field cadence:** [field_cadence_v0](field_cadence_v0.md) — Core/Tail tiers; receiver-side metrics not sent in beacon.
- **Fields inventory:** [nodetable_fields_inventory_v0](nodetable_fields_inventory_v0.md) — rssiLast, snrLast, rssiRecent, snrRecent.
- **Minset:** [link-telemetry-minset-v0](../contract/link-telemetry-minset-v0.md) Table A — Link/Metrics field set and ownership.
