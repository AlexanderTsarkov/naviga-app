# NodeTable — Link/Metrics & Telemetry/Health minimal set (Contract v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158)

This contract defines the **minimal field set** for Link/Metrics and Telemetry/Health in NodeTable (product contract v0): ownership, persisted vs derived, and update semantics. It does **not** define beacon packet formats, byte budgets, encoding, or airtime constraints — those are out-of-scope and will be handled in a future policy/spec.

---

## 1) Scope guardrails

- **In scope:** Minimal field set for Link/Metrics and Telemetry/Health; ownership and source (per [#156](../policy/source-precedence-v0.md)); persisted vs derived (align with [#157](../policy/snapshot-semantics-v0.md) Step 2); high-level update semantics (align with restore/merge policy). Cadence **hints** (frequent vs infrequent) are informational only; no encoding or byte budget.
- **Out of scope (strict):** Beacon payload budget, encoding, packing, protobuf, or airtime constraints. Full Mesh/JOIN routing. RadioProfile registries or HW capability mappings ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)). This contract **MUST NOT** imply that all fields are carried in the frequent beacon; the frequent beacon will carry a subset; encoding is deferred.

---

## 2) Definitions

- **Link (receiver-side):** Metrics observed by the receiver when a packet from the node is received: RSSI, SNR, reception time. Per-node; not per-packet cache.
- **Mesh (v0):** For this contract, “link” and “metrics” refer to the receiver’s view of the link to that node (e.g. last RSSI/SNR, lastRxAt). Multi-hop routing state (hops/via, forwarding tables) is out of scope for this minset.
- **Telemetry / Health:** Data about the node’s own state that the node may broadcast or send (battery, uptime, fw version, hw identifier). Node-owned; observed via ObservedRx or BroadcastClaim when present in payload.
- **Observed metrics:** Raw values from RX (e.g. rssiLast, snrLast, lastRxAt). Stored as last-known; overwritten when newer value arrives.
- **Derived scores:** Computed from observed metrics (e.g. EWMA of RSSI/SNR, or a linkQuality score). Recomputed on restore; not persisted as source of truth.

---

## 3) Minimal field sets

### Table A: Link/Metrics minset

| Field | Type/shape | Optional/Required | Owner/source | Persisted/Derived | Notes |
|-------|------------|-------------------|--------------|-------------------|--------|
| **lastRxAt** | Timestamp (receiver-side) | Required | ObservedRx | Persisted | Receiver-side time of last RX from this node; used for ordering and lastSeenAge. Time base is implementation-defined (monotonic or wall-clock) but must be consistent for ordering comparisons. Cadence: updated on every RX. |
| **rxRate** | Integer (RX count per window) | Optional | ObservedRx | Persisted | RX count per window; window length is implementation-defined. Cadence: can be infrequent (periodic rollup). |
| **rssiLast** | Integer (e.g. dBm) | Optional | ObservedRx | Persisted | Last received signal strength from this node. Cadence: typically every RX. |
| **snrLast** | Numeric (e.g. dB) | Optional | ObservedRx | Persisted | Last SNR from this node. Cadence: typically every RX. |
| **rssiAvg** | Numeric (EWMA) | Optional | Derived | Derived | Exponential moving average of RSSI; recomputed after restore. |
| **snrAvg** | Numeric (EWMA) | Optional | Derived | Derived | Exponential moving average of SNR; recomputed after restore. |
| **linkQuality** | Score (e.g. 0–1 or enum) | Optional | Derived | Derived | Derived score from RSSI/SNR/rate; recomputed after restore. |

### Table B: Telemetry/Health minset

| Field | Type/shape | Optional/Required | Owner/source | Persisted/Derived | Notes |
|-------|------------|-------------------|--------------|-------------------|--------|
| **batteryPercent** | 0–100 (integer) or null | Optional | ObservedRx / BroadcastClaim | Persisted | Primary battery indicator. Cadence: may be frequent or infrequent; encoding deferred. |
| **batteryVoltage** | Numeric (V) or null | Optional | ObservedRx / BroadcastClaim | Persisted | Raw voltage; optional when batteryPercent is present. Cadence: often infrequent. |
| **uptimeSec** | Non-negative integer | Optional | ObservedRx / BroadcastClaim | Persisted | Node uptime in seconds. Cadence: infrequent. |
| **fwVersion** | Opaque string or version id | Optional | BroadcastClaim / ObservedRx | Persisted | Firmware version; mapping/display deferred. Cadence: infrequent. |
| **hwProfile** | Opaque id | Optional | BroadcastClaim / ObservedRx | Persisted | Hardware model/profile id; mapping to human-readable form deferred to [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159). Cadence: infrequent. |
| **temperature** | Numeric (°C) or null | Optional | ObservedRx / BroadcastClaim | Persisted | Extended/rare telemetry; not required for v0. Include only if product needs it. |
| **telemetryAt** | Timestamp (receiver-side) | Optional | ObservedRx | Persisted | Receiver-side time when telemetry was last updated/observed; helps UI/debug freshness. Missing does not erase prior telemetry values. |

---

## 4) Ownership & precedence rules

- **Link and Telemetry/Health are node-owned.** Source is ObservedRx (from packet reception) or BroadcastClaim (when carried in node’s payload). LocalUser **never** overrides these node facts; LocalUser may annotate “important node” or similar for display/filtering, but that is separate from the node-owned field values.
- Precedence follows [source-precedence-v0](../policy/source-precedence-v0.md) ([#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)): for Mesh/Link and Telemetry, **ObservedRx overwrites**; RestoredSnapshot holds last-known until newer ObservedRx arrives.

---

## 5) Persisted vs derived (align with #157)

- **Persist:** lastRxAt, rxRate (if present), rssiLast, snrLast; batteryPercent, batteryVoltage (if present), uptimeSec, fwVersion, hwProfile, temperature (if present); telemetryAt (if present). Persist last-known raw values plus timestamps (lastRxAt, telemetryAt when applicable) so lastSeenAge and derived metrics can be recomputed after restore.
- **Derived (do not persist as source of truth):** rssiAvg, snrAvg, linkQuality. Recomputed after restore from persisted raw values (and from first ObservedRx after restore for link metrics per snapshot-semantics).

---

## 6) Update semantics (high-level)

- **Incoming overwrites if newer:** When an RX (or BroadcastClaim) provides a value for a node-owned field, the new value overwrites the stored value when the update is newer (reception time or payload timestamp). Align with [restore-merge-rules-v0](../policy/restore-merge-rules-v0.md): node-owned fields, incoming present → incoming wins.
- **Missing values do not erase known values:** If an incoming payload does not carry a given field (e.g. no battery in this packet), do **not** overwrite or clear the existing stored value. Only overwrite when the incoming update **carries a value** for that field (or explicit reset/delete per policy).

---

## 7) Beacon budget future note

**Explicit:** The **frequent beacon** will carry only a **subset** of these fields; byte budget and encoding are **out-of-scope** for this contract. Additional fields (e.g. telemetry, fw version) may be sent **less frequently** or in **separate message types**. Encoding, packing, and airtime constraints will be defined in a future **Beacon minset & encoding** policy/spec. This document defines the **product contract** (what NodeTable stores and how it is updated); it does **not** define what goes on the air or in which packet.

---

## 8) Open questions / TODO

- **Beacon minset & encoding:** Future policy/spec for which fields appear in the frequent beacon, payload layout, and byte budget.
- **Registries mapping (#159):** hwProfile / fwVersion mapping to human-readable labels and capability flags.
- **Mesh routing:** Future definition of hops/via, forwarding, and multi-hop metrics if needed.
- **Window for rxRate (count-per-window):** Implementation-defined window length and rollover; document when stable.

---

## 9) Related

- **Source precedence:** [policy/source-precedence-v0.md](../policy/source-precedence-v0.md) — [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)
- **Snapshot semantics (persisted/derived):** [policy/snapshot-semantics-v0.md](../policy/snapshot-semantics-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 2
- **Restore/merge rules:** [policy/restore-merge-rules-v0.md](../policy/restore-merge-rules-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 3
- **NodeTable contract (WIP):** [../index.md](../index.md)
- **HW/Radio registries:** [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) (out of scope for this contract)
