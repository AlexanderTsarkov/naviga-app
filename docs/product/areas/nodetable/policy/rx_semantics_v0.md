# NodeTable — RX semantics v0 (Policy)

**Status:** Canon (policy).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines how the receiver interprets and applies incoming packets (`Node_OOTB_Core_Pos`, `Node_OOTB_Core_Tail`, `Node_OOTB_Operational`, `Node_OOTB_Informative`, `Node_OOTB_I_Am_Alive`) for NodeTable updates: accepted vs duplicate vs out-of-order, seq16 wrap handling, and rules for lastRxAt, link metrics, Activity, and position. Contract truth is [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md), [alive_packet_encoding_v0](../contract/alive_packet_encoding_v0.md), [tail2_packet_encoding_v0](../contract/tail2_packet_encoding_v0.md), and [info_packet_encoding_v0](../contract/info_packet_encoding_v0.md); this doc is the receiver behaviour policy.

---

## 1) Definitions

| Term | Meaning |
|------|---------|
| **Accepted packet** | A packet that passes version and nodeId checks and any payload checks the receiver applies; it is used to update NodeTable (at least lastRxAt and, where applicable, position/telemetry/link metrics). All `Node_*` packet types can be accepted. |
| **Duplicate** | A packet whose `(nodeId48, seq16)` pair matches one already seen for that node. Receiver may refresh lastRxAt but MUST NOT treat it as a new sample for position/telemetry. For `Node_OOTB_Core_Tail`, a second packet referencing the same `ref_core_seq16` is also treated as a duplicate (unexpected; TX MUST NOT generate them — see [tail1_packet_encoding_v0 §1](../contract/tail1_packet_encoding_v0.md)). |
| **Out-of-order (ooo)** | A packet whose `seq16` is older than the last accepted seq for that node (by the wrap rule). Receiver MUST NOT use it to overwrite newer position or telemetry ("no time travel"); MAY accept only for lastRxAt within a small tolerance (implementation-defined). For `Node_OOTB_Core_Tail`, OOO is handled by the `ref_core_seq16 == lastCoreSeq` gate — a stale Core_Tail is dropped silently. |
| **seq16 wrap** | seq16 is a 16-bit counter; it wraps. Ordering is defined by modulo subtraction: `delta = (incoming − last) mod 2¹⁶`. If `delta = 0` → Duplicate. If `delta ∈ [1, 32767]` → Newer. If `delta ∈ [32768, 65535]` → Older (out-of-order). Implementations MUST use this rule consistently for all `Node_*` packet types. |

**seq16 scope — sender counter:** The sender maintains **one global seq16 counter per node**, incremented on every transmitted packet regardless of type. This counter is shared across all `Node_*` packet types during uptime.

**seq16 scope — payload presence:** `seq16` is part of the **Common prefix** of every `Node_*` packet (see [ootb_radio_v0.md §3.0](../../../../protocols/ootb_radio_v0.md#30-packet-anatomy--invariants)). All packet types embed the current counter value as `seq16` at payload bytes 7–8. **Dedupe key: `(nodeId48, seq16)` — payload-agnostic.** Exception: `Node_OOTB_Core_Tail` additionally carries `ref_core_seq16` in its Useful payload (bytes 9–10) as a Core linkage key; this is distinct from `seq16`.

**Duplicate / OOO handling per packet type:**
- **`Node_OOTB_Core_Pos` / `Node_OOTB_I_Am_Alive`:** Use the seq16 wrap rule (modulo subtraction) for duplicate and OOO detection. The seen seq16 set is per-node global across all packet types.
- **`Node_OOTB_Core_Tail`:** Primary dedupe is `(nodeId48, seq16)` from Common. Payload application is additionally gated by `ref_core_seq16 == lastCoreSeq`. **One tail per Core sample (normative):** TX MUST generate at most one Core_Tail per Core_Pos sample (see [tail1_packet_encoding_v0 §1](../contract/tail1_packet_encoding_v0.md)). If a receiver sees a second Core_Tail referencing the same `ref_core_seq16`, it MUST treat it as an unexpected duplicate: ignore payload, optionally update lastRxAt/link metrics. A stale Core_Tail (referencing an older Core) is dropped by the `ref_core_seq16 == lastCoreSeq` gate.
- **`Node_OOTB_Operational` / `Node_OOTB_Informative`:** Dedupe by `(nodeId48, seq16)`. When version and nodeId are valid and `seq16` is new (not a duplicate by the wrap rule), apply carried fields to NodeTable. No CoreRef gate.

---

## 2) Which packets update NodeTable

- **`Node_OOTB_Core_Pos`:** Accepted if version and nodeId valid. Updates lastRxAt, lastCoreSeq, position (lat/lon), seq16. Link metrics updated on acceptance.
- **`Node_OOTB_Core_Tail`:** Applied **only if** `ref_core_seq16` equals the last Core seq16 received from that node (**CoreRef-lite**); otherwise **MUST** ignore for payload. When applied: updates lastRxAt; may update posFlags/sats for that Core sample. Link metrics updated on acceptance. **MUST NOT revoke or invalidate position already received in Core_Pos** (see §4).
- **`Node_OOTB_Operational`:** No CoreRef. Accepted if version and nodeId valid. Updates lastRxAt and any carried Operational fields (`batteryPercent`, `uptimeSec`). Link metrics updated on acceptance.
- **`Node_OOTB_Informative`:** No CoreRef. Accepted if version and nodeId valid. Updates lastRxAt and any carried Informative fields (`maxSilence10s`, `hwProfileId`, `fwVersionId`). Link metrics updated on acceptance.
- **`Node_OOTB_I_Am_Alive`:** Accepted if version and nodeId valid. Updates lastRxAt and seq16 for ordering (same per-node counter as all `Node_*` packets; §1). **Does not** carry or update position. Link metrics updated on acceptance. **Alive satisfies the "alive within maxSilence window"** for Activity derivation when the node has no fix (see [activity_state_v0](activity_state_v0.md), [field_cadence_v0](field_cadence_v0.md)).

---

## 3) Update rules on RX

### 3.1 lastRxAt / telemetryAt

- On **any** accepted `Node_*` packet, the receiver **MUST** update **lastRxAt** for that node to the current reception time.
- **telemetryAt** (receiver-side time when telemetry was last observed) is updated when the accepted packet carries or implies telemetry (Core_Pos position, Core_Tail/Operational/Informative fields); for `Node_OOTB_I_Am_Alive`-only, lastRxAt is updated but telemetryAt need not change if no telemetry is present.

### 3.2 Link metrics (rssiLast, snrLast, etc.)

- Link metrics are updated on **accepted** packets. If Tail-1 is **not** applied (`ref_core_seq16` ≠ lastCoreSeq), the receiver may still update lastRxAt and link metrics from that packet, or may treat the packet as ignored for all purposes; policy choice is implementation-defined. Recommended: update lastRxAt and link metrics even when Tail-1 payload is ignored, so "last heard" remains accurate.

### 3.3 Activity derivation and maxSilence

- Activity (Active / Stale / Lost) is derived from **lastSeenAge** (time since lastRxAt) and policy-supplied thresholds. The **maxSilence MUST** guarantee (sender will transmit at least one alive-bearing packet within the maxSilence window) is the primary input for that expectation.
- **Alive packet** is **alive-bearing**. When the node has no valid fix, it sends Alive instead of BeaconCore; receiving an Alive packet within the window **satisfies** the "alive within window" condition for Activity. The receiver does not require BeaconCore for liveness when Alive is received.

---

## 4) Core_Tail and position: no revocation of Core

- **Baseline** (first Core sent) is defined in [field_cadence_v0](field_cadence_v0.md) §2.1; once a node has sent at least one `Node_OOTB_Core_Pos`, it has a baseline. `Node_OOTB_Core_Pos` is position-bearing; when a receiver has accepted a Core_Pos with valid lat/lon, that position is **authoritative** for that node until a **new** Core_Pos with a newer seq16 updates it.
- **`Node_OOTB_Core_Tail`** carries posFlags/sats that **qualify** the **same** Core sample (same `ref_core_seq16`). Core_Tail **MUST NOT** be interpreted as revoking or invalidating position already received in Core_Pos. In particular: if the receiver already has position from Core_Pos for that node, a subsequent Core_Tail with posFlags = 0 (or "no fix") **does not** clear or invalidate that position; it only qualifies the metadata for that sample. Position is overwritten only by a newer `Node_OOTB_Core_Pos`.

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
