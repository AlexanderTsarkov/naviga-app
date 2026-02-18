# NodeTable — RX semantics v0 (Policy)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines how the receiver interprets and applies incoming packets (BeaconCore, BeaconTail-1, BeaconTail-2, Alive) for NodeTable updates: accepted vs duplicate vs out-of-order, seq16 wrap handling, and rules for lastRxAt, link metrics, Activity, and position. Contract truth is [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) and [alive_packet_encoding_v0](../contract/alive_packet_encoding_v0.md); this doc is the receiver behaviour policy.

---

## 1) Definitions

| Term | Meaning |
|------|---------|
| **Accepted packet** | A packet that passes version and nodeId checks and any payload checks the receiver applies; it is used to update NodeTable (at least lastRxAt and, where applicable, position/telemetry/link metrics). BeaconCore, BeaconTail-1, BeaconTail-2, and Alive can all be accepted. |
| **Duplicate** | A packet that carries the same logical sample as one already applied (e.g. same nodeId and seq16 for Core or Alive). Receiver may refresh lastRxAt but MUST NOT treat it as a new sample for position/telemetry (no overwrite with same data). |
| **Out-of-order (ooo)** | A packet whose seq16 (or core_seq16 for Tail-1) is older than the last accepted seq for that node. Receiver MUST NOT use it to overwrite newer position or telemetry (“no time travel”); MAY ignore for payload or accept only for lastRxAt within a small tolerance (implementation-defined). |
| **seq16 wrap** | seq16 is a 16-bit counter; it wraps. Ordering and “newer vs older” MUST be defined in a way that respects wrap (e.g. modulo arithmetic or signed difference within a window). Exact wrap handling is implementation-defined but MUST be consistent for duplicate vs new vs ooo decisions. |

**seq16 scope:** seq16 is a **single per-node counter** across all transmitted packet types during uptime (BeaconCore, Tail-1/2, Alive). Receivers **MUST** treat seq16 ordering and deduplication across packet types accordingly. The seen seq16 set (or ordering window) is **per-node global**, not per packet type.

---

## 2) Which packets update NodeTable

- **BeaconCore:** Accepted if version and nodeId valid. Updates lastRxAt, lastCoreSeq, position (lat/lon when not sentinel), seq16. Link metrics (rssiLast, snrLast, etc.) updated on acceptance.
- **BeaconTail-1:** Applied **only if** core_seq16 equals the last Core seq16 received from that node (**CoreRef-lite**); otherwise **MUST** ignore for payload. When applied: updates lastRxAt; may update posFlags/sats and other attached fields for that Core sample. Link metrics updated on acceptance. **Tail-1 MUST NOT revoke or invalidate position already received in BeaconCore** for that node (see §4).
- **BeaconTail-2:** No CoreRef. Accepted if version and nodeId valid. Updates lastRxAt and any carried Tail-2 fields (maxSilence, batteryPercent, etc.). Link metrics updated on acceptance.
- **Alive:** Accepted if version and nodeId valid. Updates lastRxAt and seq16 for ordering (same per-node counter as Core/Tails; §1). **Does not** carry or update position. Link metrics updated on acceptance. **Alive satisfies the “alive within maxSilence window”** for Activity derivation when the node has no fix and therefore does not send BeaconCore (see [activity_state_v0](activity_state_v0.md), [field_cadence_v0](field_cadence_v0.md)).

---

## 3) Update rules on RX

### 3.1 lastRxAt / telemetryAt

- On **any** accepted packet (BeaconCore, BeaconTail-1, BeaconTail-2, or Alive), the receiver **MUST** update **lastRxAt** for that node to the current reception time.
- **telemetryAt** (receiver-side time when telemetry was last observed) is updated when the accepted packet carries or implies telemetry (Core position, Tail-1/Tail-2 fields); for Alive-only, lastRxAt is updated but telemetryAt need not change if no telemetry is present.

### 3.2 Link metrics (rssiLast, snrLast, etc.)

- Link metrics are updated on **accepted** packets. If Tail-1 is **not** applied (core_seq16 ≠ lastCoreSeq), the receiver may still update lastRxAt and link metrics from that packet, or may treat the packet as ignored for all purposes; policy choice is implementation-defined. Recommended: update lastRxAt and link metrics even when Tail-1 payload is ignored, so “last heard” remains accurate.

### 3.3 Activity derivation and maxSilence

- Activity (Active / Stale / Lost) is derived from **lastSeenAge** (time since lastRxAt) and policy-supplied thresholds. The **maxSilence MUST** guarantee (sender will transmit at least one alive-bearing packet within the maxSilence window) is the primary input for that expectation.
- **Alive packet** is **alive-bearing**. When the node has no valid fix, it sends Alive instead of BeaconCore; receiving an Alive packet within the window **satisfies** the “alive within window” condition for Activity. The receiver does not require BeaconCore for liveness when Alive is received.

---

## 4) Tail-1 and position: no revocation of Core

- **BeaconCore** is position-bearing; when a receiver has accepted a BeaconCore with valid lat/lon, that position is **authoritative** for that node until a **new** BeaconCore with a newer seq16 updates it.
- **Tail-1** carries posFlags/sats that **qualify** the **same** Core sample (same core_seq16). Tail-1 **MUST NOT** be interpreted as revoking or invalidating position already received in BeaconCore. In particular: if the receiver already has position from Core for that node, a subsequent Tail-1 with posFlags = 0 (or “no fix”) **does not** clear or invalidate that position; it only qualifies the metadata for that sample. Position is overwritten only by a newer BeaconCore.

---

## 5) Related

- **Beacon encoding:** [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) — Core/Tail layouts; §3.1 position-bearing vs alive-bearing.
- **Alive encoding:** [alive_packet_encoding_v0](../contract/alive_packet_encoding_v0.md).
- **Field cadence:** [field_cadence_v0](field_cadence_v0.md) — Core only with valid fix; maxSilence via Alive when no fix.
- **Activity state:** [activity_state_v0](activity_state_v0.md) — lastRxAt, thresholds, duplicate/ooo notes.
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
