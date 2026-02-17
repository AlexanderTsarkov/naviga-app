# NodeTable — Fields inventory v0 (owner decision worksheet)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This document is an **owner-reviewable worksheet** that lists all NodeTable-related fields **currently implied by existing WIP docs** (no new fields or semantics). It prepares columns for the owner to decide Tier, default cadence, and packet placement. It drives Core/Tail split and alignment with [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) and [field_cadence_v0.md](field_cadence_v0.md).

---

## 1) Normative Coupling rules (v0)

- **Attached-to-Core:** Some fields **qualify a specific Core sample** (e.g. position qualifies “where” for that node at that freshness). If they arrive later without a Core reference, they can corrupt meaning (wrong node or wrong time).
- **CoreRef framing:** v0 does **NOT** define CoreRef framing unless a later follow-up explicitly adds it.
- **Rule:** Any **Attached-to-Core** field **MUST** ride with the Core sample in the **same BeaconCore frame**. If sent separately, it **MUST** be ignored in v0 (no reassociation).
- **Uncoupled** fields may be delivered later without reference; they do not require the same frame as Core.

**Tail-1 (BeaconTail-1):** In v0, Attached-to-Core content sent in a **separate** Tail-1 frame **MUST** include **core_seq16** (CoreRef-lite) referencing the Core sample it qualifies; receiver applies only if core_seq16 == lastCoreSeq. See [field_cadence_v0.md](field_cadence_v0.md) §2.

---

## 2) Main table

All rows cite at least one source doc. **Proposed Tier**, **Default cadence**, and **Packet placement** are for owner fill-in or “(owner)” placeholder.

| Field name | Meaning / why it exists | Source doc(s) | Source category | Sent vs Derived | Coupling | CoreRef required if sent separately | Proposed Tier | Default cadence | Packet placement | Size guess (bytes) / encoding hint | Notes / open decision |
|------------|-------------------------|---------------|-----------------|-----------------|----------|-------------------------------------|---------------|-----------------|------------------|-------------------------------------|------------------------|
| **payloadVersion** | Payload format version (v0 = 0x00). | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3.1 | Other | Sent | Uncoupled | No | (owner) | (owner) | (owner) | 1 | — |
| **nodeId** | DeviceId (e.g. ESP32-S3 MCU ID); primary key, WHO. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3.1, [index.md](../index.md) §3 | Identity | Sent | Uncoupled | No | (owner) | (owner) | (owner) | 8 | — |
| **positionLat** | Latitude × 1e7 (WGS84); WHERE. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3.1, [index.md](../index.md) §6 | Position | Sent | Attached-to-Core | Yes | (owner) | (owner) | (owner) | 4 | Must ride with Core in v0 if sent. |
| **positionLon** | Longitude × 1e7 (WGS84); WHERE. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3.1, [index.md](../index.md) §6 | Position | Sent | Attached-to-Core | Yes | (owner) | (owner) | (owner) | 4 | Must ride with Core in v0 if sent. |
| **posFlags** | Position-valid / quality flags (uint8); 0 = not present or no fix. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §4.2 | Position | Sent | Attached-to-Core | Yes (Tail-1: core_seq16) | B | Every Tail-1 | BeaconTail-1 (optional) | 1 | Position quality (Tail-1); attached to Core sample. See [position_quality_v0.md](position_quality_v0.md). |
| **sats** | Satellite count when position valid (uint8); 0 = not present. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §4.2 | Position | Sent | Attached-to-Core | Yes (Tail-1: core_seq16) | B | Every Tail-1 | BeaconTail-1 (optional) | 1 | Position quality (Tail-1); attached to Core sample. See [position_quality_v0.md](position_quality_v0.md). |
| **freshness marker** (seq or equivalent) | Ordering and staleness; required for Core. **seq16** canonical. | [field_cadence_v0.md](field_cadence_v0.md) §2, [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §4.1 | Freshness | Sent | Attached-to-Core | Yes | (owner) | (owner) | (owner) | 2 | seq16 in Core; Tail-1 core_seq16 references it. |
| **uptimeSec** | Node uptime in seconds; timing. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3.1, [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table B | Power/Health | Sent | Uncoupled | No | (owner) | (owner) | (owner) | 4 | 0xFFFFFFFF = not present. |
| **batteryPercent** | Primary battery indicator 0–100. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3.1, [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table B | Power/Health | Sent | Uncoupled | No | (owner) | (owner) | (owner) | 1 | 0xFF = not present. |
| **hwProfileId** / **hwProfile** | Hardware model/profile id; capability lookup. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3.1, [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table B, [index.md](../index.md) §7 | Identity | Sent | Uncoupled | No | (owner) | (owner) | (owner) | 2 | 0xFFFF = not present. |
| **fwVersionId** / **fwVersion** | Firmware version id (opaque); mapping deferred. | [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) §3.1, [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table B, [index.md](../index.md) §7 | Identity | Sent | Uncoupled | No | (owner) | (owner) | (owner) | 2 | 0xFFFF = not present. |
| **maxSilence indicator** (or equivalent) | Helps activity/staleness; exact placement TBD. | [field_cadence_v0.md](field_cadence_v0.md) §3 | Freshness | Sent | Uncoupled | No | (owner) | (owner) | (owner) | — | Open decision. |
| **txPowerStep hint** | Node-side tx power hint; stub. | [field_cadence_v0.md](field_cadence_v0.md) §3 | Radio/Link | Sent | Uncoupled | No | (owner) | (owner) | (owner) | — | Stub; optional. |
| **batteryVoltage** | Raw voltage (V); optional when batteryPercent present. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table B | Power/Health | Sent | Uncoupled | No | (owner) | (owner) | (owner) | — | Encoding not in beacon layout yet. |
| **temperature** | Extended telemetry (°C); not required for v0. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table B | Power/Health | Sent | Uncoupled | No | (owner) | (owner) | (owner) | — | Include only if product needs. |
| **telemetryAt** | Receiver-side time when telemetry last updated/observed. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table B | Quality | Derived | Uncoupled | No | (owner) | (owner) | (owner) | — | Receiver-side only. |
| **lastRxAt** | Receiver-side time of last RX from this node; ordering and lastSeenAge. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table A | Quality | Derived | Uncoupled | No | (owner) | (owner) | — | — | Not sent in beacon; updated on every RX. |
| **rxRate** | RX count per window; implementation-defined window. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table A | Radio/Link | Derived | Uncoupled | No | (owner) | (owner) | — | — | Not sent; observed/rollup. |
| **rssiLast** | Last received signal strength from this node. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table A, [index.md](../index.md) §6 | Radio/Link | Derived | Uncoupled | No | (owner) | (owner) | — | — | Receiver-side; not in beacon. |
| **snrLast** | Last SNR from this node. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table A, [index.md](../index.md) §6 | Radio/Link | Derived | Uncoupled | No | (owner) | (owner) | — | — | Receiver-side; not in beacon. |
| **rssiAvg** | EWMA of RSSI; recomputed after restore. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table A | Radio/Link | Derived | Uncoupled | No | (owner) | (owner) | — | — | Derived; not persisted as source. |
| **snrAvg** | EWMA of SNR; recomputed after restore. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table A | Radio/Link | Derived | Uncoupled | No | (owner) | (owner) | — | — | Derived; not persisted as source. |
| **linkQuality** | Derived score from RSSI/SNR/rate; recomputed after restore. | [link-telemetry-minset-v0.md](../contract/link-telemetry-minset-v0.md) Table A | Radio/Link | Derived | Uncoupled | No | (owner) | (owner) | — | — | Derived; not persisted as source. |
| **lastOriginSeqSeen** | Last sequence number seen from that origin; stream-level. | [index.md](../index.md) §9, §10 | Freshness | Derived | Uncoupled | No | (owner) | (owner) | — | — | NodeTable stream metadata; not per-packet. |
| **ShortId** | Display id derived from DeviceId (e.g. CRC16); may collide. | [index.md](../index.md) §3, Glossary | Identity | Derived | Uncoupled | No | — | — | — | — | Derived local display only; not sent in v0. |
| **networkName** | Concept: broadcast name; optional; display precedence when present. | [index.md](../index.md) §4 | Identity | Sent (concept) | Uncoupled | No | — | — | — | — | NOT sent in v0 beacon; MUST NOT be used on OOTB public; future session-join exchange. |
| **localAlias** | User-defined on phone for a DeviceId. | [index.md](../index.md) §4 | Identity | Other | Uncoupled | No | — | — | — | — | Local-only; not sent over radio. |
| **activityState** | Derived: Online / Uncertain / Stale / Archived. | [index.md](../index.md) §5 | Quality | Derived | Uncoupled | No | (owner) | (owner) | — | — | Derived from lastSeenAge vs policy boundary. |
| **lastSeenAge** | Time since last RX; used for activity derivation. | [index.md](../index.md) §5, Glossary | Quality | Derived | Uncoupled | No | (owner) | (owner) | — | — | Derived from lastRxAt. |
| **PositionQuality** (age, satCount, hdop, etc.) | Position quality; age required; source-tagged. | [index.md](../index.md) §6 | Position | Derived | — | — | — | — | — | — | Derived from Core position + Tail-1 posFlags/sats + optional receiver-side age. Sent components in v0: posFlags, sats (Tail-1 only). See [position_quality_v0.md](position_quality_v0.md). |
| **lastSeenRadioProfile** / **RadioProfileId** / **RadioProfileKind** | Radio profile and txPower when available. | [index.md](../index.md) §8 | Radio/Link | Derived | Uncoupled | No | (owner) | (owner) | — | — | From RX context; not in beacon layout. |

---

## 3) Owner review checklist

- [ ] **Tier A is strict:** WHO + WHERE + freshness (+ required liveness signal if decided); no extra telemetry in Core.
- [ ] **Attached-to-Core fields** are not placed in Tail unless CoreRef exists (not in v0); position/freshness ride in same BeaconCore frame.
- [ ] **ProductMaxFrameBytes=96** respected; no fragmentation; Core fits within profile-class payload budgets.
- [ ] **OOTB public remains beacon-only;** extras (UserMessage, GeoObject, GeoObjectActivation) session-only per [traffic_model_v0](../../radio/policy/traffic_model_v0.md).
- [ ] **Mesh-lite beacon-only forwarding v0;** extras direct-link only (no relays).
- [ ] **Proposed Tier / cadence / packet placement** filled for all fields that are sent in beacon; receiver-only fields need no packet placement.
- [ ] **Encoding alignment:** Once owner decisions are stable, [beacon_payload_encoding_v0.md](../contract/beacon_payload_encoding_v0.md) and [field_cadence_v0.md](field_cadence_v0.md) updated as needed (no layout change in this worksheet step).
- [ ] **v0 role:** Role (human dongle vs dog collar vs infra) is derived from **hwProfileId** in v0; hunting subroles are session-only future.
