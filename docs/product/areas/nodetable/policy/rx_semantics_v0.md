# NodeTable — RX semantics v0 (Policy)

**Status:** Canon (policy).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines how the receiver interprets and applies incoming packets (BeaconCore, BeaconTail-1, BeaconTail-2, Alive) for NodeTable updates: accepted vs duplicate vs out-of-order, seq16 wrap handling, and rules for lastRxAt, link metrics, Activity, and position. Contract truth is [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) and [alive_packet_encoding_v0](../contract/alive_packet_encoding_v0.md); this doc is the receiver behaviour policy.

---

## 1) Definitions

| Term | Meaning |
|------|---------|
| **Accepted packet** | A packet that passes version and nodeId checks and any payload checks the receiver applies; it is used to update NodeTable (at least lastRxAt and, where applicable, position/telemetry/link metrics). BeaconCore, BeaconTail-1, BeaconTail-2, and Alive can all be accepted. |
| **Duplicate** | A packet that carries the same logical sample as one already applied (e.g. same nodeId and seq16 for Core or Alive). Receiver may refresh lastRxAt but MUST NOT treat it as a new sample for position/telemetry (no overwrite with same data). |
| **Out-of-order (ooo)** | A packet whose seq16 (or core_seq16 for Tail-1) is older than the last accepted seq for that node. Receiver MUST NOT use it to overwrite newer position or telemetry ("no time travel"); MAY ignore for payload or accept only for lastRxAt within a small tolerance (implementation-defined). |
| **seq16 wrap** | seq16 is a 16-bit counter; it wraps. Ordering is defined by modulo subtraction: `delta = (incoming − last) mod 2¹⁶`. If `delta = 0` → Duplicate. If `delta ∈ [1, 32767]` → Newer. If `delta ∈ [32768, 65535]` → Older (out-of-order). Implementations MUST use this rule consistently for all duplicate / new / ooo decisions. |

**seq16 scope:** seq16 is a **single per-node counter** across all transmitted packet types during uptime (BeaconCore, Tail-1/2, Alive). Receivers **MUST** treat seq16 ordering and deduplication across packet types accordingly. The seen seq16 set (or ordering window) is **per-node global**, not per packet type.

---

## 2) Which packets update NodeTable

- **BeaconCore:** Accepted if version and nodeId valid. Updates lastRxAt, lastCoreSeq, position (lat/lon when not sentinel), seq16. Link metrics (rssiLast, snrLast, etc.) updated on acceptance.
- **BeaconTail-1:** Applied **only if** core_seq16 equals the last Core seq16 received from that node (**CoreRef-lite**); otherwise **MUST** ignore for payload. When applied: updates lastRxAt; may update posFlags/sats and other attached fields for that Core sample. Link metrics updated on acceptance. **Tail-1 MUST NOT revoke or invalidate position already received in BeaconCore** for that node (see §4).
- **BeaconTail-2:** No CoreRef. Accepted if version and nodeId valid. Updates lastRxAt and any carried Tail-2 fields (maxSilence, batteryPercent, etc.). Link metrics updated on acceptance.
- **Alive:** Accepted if version and nodeId valid. Updates lastRxAt and seq16 for ordering (same per-node counter as Core/Tails; §1). **Does not** carry or update position. Link metrics updated on acceptance. **Alive satisfies the "alive within maxSilence window"** for Activity derivation when the node has no fix and therefore does not send BeaconCore (see [activity_state_v0](activity_state_v0.md), [field_cadence_v0](field_cadence_v0.md)).

---

## 3) Update rules on RX

### 3.1 lastRxAt / telemetryAt

- On **any** accepted packet (BeaconCore, BeaconTail-1, BeaconTail-2, or Alive), the receiver **MUST** update **lastRxAt** for that node to the current reception time.
- **telemetryAt** (receiver-side time when telemetry was last observed) is updated when the accepted packet carries or implies telemetry (Core position, Tail-1/Tail-2 fields); for Alive-only, lastRxAt is updated but telemetryAt need not change if no telemetry is present.

### 3.2 Link metrics (rssiLast, snrLast, etc.)

- Link metrics are updated on **accepted** packets. If Tail-1 is **not** applied (core_seq16 ≠ lastCoreSeq), the receiver may still update lastRxAt and link metrics from that packet, or may treat the packet as ignored for all purposes; policy choice is implementation-defined. Recommended: update lastRxAt and link metrics even when Tail-1 payload is ignored, so "last heard" remains accurate.

### 3.3 Activity derivation and maxSilence

- Activity (Active / Stale / Lost) is derived from **lastSeenAge** (time since lastRxAt) and policy-supplied thresholds. The **maxSilence MUST** guarantee (sender will transmit at least one alive-bearing packet within the maxSilence window) is the primary input for that expectation.
- **Alive packet** is **alive-bearing**. When the node has no valid fix, it sends Alive instead of BeaconCore; receiving an Alive packet within the window **satisfies** the "alive within window" condition for Activity. The receiver does not require BeaconCore for liveness when Alive is received.

---

## 4) Tail-1 and position: no revocation of Core

- **Baseline** (first Core sent) is defined in [field_cadence_v0](field_cadence_v0.md) §2.1; once a node has sent at least one BeaconCore, it has a baseline. **BeaconCore** is position-bearing; when a receiver has accepted a BeaconCore with valid lat/lon, that position is **authoritative** for that node until a **new** BeaconCore with a newer seq16 updates it.
- **Tail-1** carries posFlags/sats that **qualify** the **same** Core sample (same core_seq16). Tail-1 **MUST NOT** be interpreted as revoking or invalidating position already received in BeaconCore. In particular: if the receiver already has position from Core for that node, a subsequent Tail-1 with posFlags = 0 (or "no fix") **does not** clear or invalidate that position; it only qualifies the metadata for that sample. Position is overwritten only by a newer BeaconCore.

---

## 5) Reboot behaviour (V1-A)

### 5.1 TX side: seq16 resets to 0 on reboot

In V1-A the sender's seq16 counter starts at 0 on every power-on / reboot (no persistence). The first outgoing packet carries seq16 = 1.

### 5.2 Receiver-side tolerance

A receiver that has `last_seq = N` for a node will, after that node reboots, receive packets starting at seq16 = 1. By the wrap rule (§1), values in `[N+1, N+32767] mod 2^16` are treated as Newer; values in `[N+32768, N+65535] mod 2^16` are treated as Older. This means:

- If `N ≤ 32767`: all post-reboot packets (seq 1 … N+32767) are treated as Newer immediately. No dropped updates.
- If `N > 32767`: post-reboot packets with seq in `[1, N−32768]` are treated as Older and will not overwrite position/telemetry until the counter advances past `N − 32767`.

**Recommended receiver heuristic (V1-A):** If `lastRxAt` age for a node exceeds `maxSilence × 3` (i.e. the node has been silent for at least 3× its declared silence budget), the receiver **SHOULD** reset its stored `last_seq` for that node to the incoming seq16 and treat the packet as a fresh start. This handles the common case where a node reboots after a prolonged absence.

### 5.3 Post-V1-A upgrade path

The following strategies may be adopted in a future iteration; they are **out of scope for V1-A**:

- **Persisted counter (recommended next step):** Store seq16 in NVS; restore on boot. First TX after reboot uses `restored_seq + 1`. Eliminates the tolerance problem entirely with no protocol change.
- **Boot nonce / session epoch:** Add a `boot_nonce` field (e.g. 8-bit random or incrementing) to the packet. Dedup key becomes `(nodeId, boot_nonce, seq16)`. Requires a protocol change (new field in payload).

---

## 6) Related

- **Beacon encoding:** [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) — Core/Tail layouts; §3.1 position-bearing vs alive-bearing.
- **Alive encoding:** [alive_packet_encoding_v0](../contract/alive_packet_encoding_v0.md).
- **Field cadence:** [field_cadence_v0](field_cadence_v0.md) — Core only with valid fix; maxSilence via Alive when no fix.
- **Activity state:** [activity_state_v0](activity_state_v0.md) — lastRxAt, thresholds, duplicate/ooo notes.
- **NodeTable hub:** [../index.md](../index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
