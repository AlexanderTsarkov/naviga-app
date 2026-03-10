# NodeTable — Packet → NodeTable mapping (S03)

**Status:** Canon (policy).  
**Issue:** [#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351).

This doc is the **canonical S03 reference** for which packets update which NodeTable fields, with gating/conditions and ties to presence rules (#371 grace_s, #372 self TX). It uses the [NodeTable master table](../../../wip/areas/nodetable/master_table/README.md) ([fields_v0_1.csv](../../../wip/areas/nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../../wip/areas/nodetable/master_table/packets_v0_1.csv)) as **source of truth** and points to encoding contracts without redefining them. No new fields or packet semantics; documentation only.

---

## 1) Scope, sources, non-goals

- **Scope (S03):** Map the five on-air packet types (Alive, Node_Core_Pos, Node_Core_Tail, Node_Operational, Node_Informative) to NodeTable field updates; include gating (GNSS fix, self vs remote, Core/Tail pairing); tie in presence/staleness rules. Doc-only; no firmware/code changes.
- **Sources of truth:** [master_table/README.md](../../../wip/areas/nodetable/master_table/README.md), [fields_v0_1.csv](../../../wip/areas/nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../../wip/areas/nodetable/master_table/packets_v0_1.csv). Every mapping claim in this doc traces to a packet or field row in those artifacts (or a linked contract).
- **Non-goals:** No BLE/UI protocol; no JOIN/Mesh/CAD/LBT; no new fields or packets; no redefinition of encoding semantics — only summarize and link.

---

## 2) Packet inventory (from packets_v0_1.csv)

| Packet (canon) | Legacy | Purpose | Gating (from packets_v0_1) | Encoding contract |
|----------------|--------|---------|----------------------------|-------------------|
| **Alive** | BEACON_ALIVE | No-fix liveness; identity + seq16 only. Alive-bearing, non-position-bearing. | When no fix, within maxSilence | [alive_packet_encoding_v0](../contract/alive_packet_encoding_v0.md); [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) §3 |
| **Node_Core_Pos** | BEACON_CORE | Minimal WHO+WHERE. Core only with valid fix. | Only with valid GNSS fix | [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) §4.1 |
| **Node_Core_Tail** | BEACON_TAIL1 | Core-attached extension; qualifies one Core sample. | ref_core_seq16 must match lastCoreSeq; when position valid | [tail1_packet_encoding_v0](../contract/tail1_packet_encoding_v0.md) §3 |
| **Node_Operational** | BEACON_TAIL2 | Dynamic runtime: battery, uptime. | No CoreRef; on change + at forced Core | [tail2_packet_encoding_v0](../contract/tail2_packet_encoding_v0.md) §3.2 |
| **Node_Informative** | BEACON_INFO | Static/config: maxSilence, hwProfileId, fwVersionId. | On change + every 10 min; MUST NOT send on every Operational | [info_packet_encoding_v0](../contract/info_packet_encoding_v0.md) §3.2 |

---

## 3) Mapping: packet → fields updated

For each packet, **fields updated** when the payload is applied (RX for remote entry; for **self**, see §4 — only presence-bearing TX update self last_seen_ms; payload fields for self are updated when we **send** that packet and write to self entry). **Source** = RX (receiver applies from wire), **NodeOwned** (from payload), **Injected** (receiver-side on RX). **Gating** from packets_v0_1.csv and field notes. Consumer location: NodeEntry (in_node_entry_struct), BLE snapshot (exported_over_ble), on-air (sent_on_air) per fields_v0_1.csv.

### 3.1 Common (all Node_* packets)

On **any** accepted Node_* packet (Alive, Core_Pos, Core_Tail, Operational, Informative), the receiver updates:

| Field | Source | Consumer | Gating |
|-------|--------|----------|--------|
| node_id | RX (payload) | NodeEntry, BLE, on-air | All packets carry node_id (beacon_payload_encoding_v0 §3). |
| payloadVersion | RX (payload) | Debug/injected only (last_payload_version_seen) | First byte of payload; not persisted as user-facing. |
| seq16 | RX (payload) | NodeEntry (last_seq), BLE | Dedupe key; last_seq updates on any RX. |

**Receiver-injected on any RX** (not from payload; from RX event): **last_seen_ms** (presence anchor), **last_rx_rssi** (when RSSI available). See [presence_and_age_semantics_v0.md](presence_and_age_semantics_v0.md) §1; [seq_ref_version_link_metrics_v0.md](seq_ref_version_link_metrics_v0.md).

### 3.2 Alive

| Field | Source | Consumer | Gating |
|-------|--------|----------|--------|
| (common: node_id, seq16) | RX | NodeEntry, BLE, on-air | — |
| aliveStatus | RX (payload, optional) | — | S03: stubbed/unused; not in NodeTable/BLE (fields_v0_1: Stubbed). |

**Gating:** Sent when **no GNSS fix**, within maxSilence. **Remote:** RX updates last_seen_ms (presence). **Self:** TX of Alive **updates self last_seen_ms** (presence-bearing per §4). Does **not** update position or pos_age_s (non-position-bearing).

### 3.3 Node_Core_Pos

| Field | Source | Consumer | Gating |
|-------|--------|----------|--------|
| (common: node_id, seq16) | RX | NodeEntry, BLE, on-air | — |
| positionLat, positionLon | RX (payload) | NodeEntry (lat_e7, lon_e7), BLE | **Only with valid GNSS fix.** Core-only sample. |
| last_core_seq16 (stored) | RX (payload seq16) | NodeEntry; used for Tail ref match | Updated **only** on Core_Pos RX (Tail apply uses ref_core_seq16 match). |

**Gating:** **Only with valid GNSS fix**; every beacon tick when position valid. **Remote:** RX updates last_seen_ms, position, last_core_seq16. **Self:** TX of Core_Pos updates **self** position fields and **pos_age_s** (only position-bearing packet for self); TX **updates self last_seen_ms** (presence-bearing per §4). Encoding: [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) §4.1.

### 3.4 Node_Core_Tail

| Field | Source | Consumer | Gating |
|-------|--------|----------|--------|
| (common: node_id, seq16) | RX | NodeEntry, BLE, on-air | — |
| ref_core_seq16 | RX (payload) | NodeEntry (link to Core sample) | **ref_core_seq16 must match lastCoreSeq** (Tail applies to that Core). |
| posFlags, sats | RX (payload, optional) | NodeEntry | Qualifies Core sample; Tail-1 only. |

**Receiver-injected on Tail RX when applied:** **last_applied_tail_ref_core_seq16** (dedupe; at most one Tail per Core per seq_ref_version_link_metrics_v0).

**Gating:** Every Tail-1 when position valid; **ref_core_seq16 must match lastCoreSeq**. **Remote:** RX updates last_seen_ms; payload updates ref_core_seq16, posFlags, sats. **Self:** TX of Core_Tail **updates self last_seen_ms** (presence-bearing, paired with Core per §4); does **not** update pos_age_s (no new lat/lon). Encoding: [tail1_packet_encoding_v0](../contract/tail1_packet_encoding_v0.md) §3.

### 3.5 Node_Operational

| Field | Source | Consumer | Gating |
|-------|--------|----------|--------|
| (common: node_id, seq16) | RX | NodeEntry, BLE, on-air | — |
| batteryPercent, uptimeSec | RX (payload, optional) | NodeEntry, BLE | On change + at forced Core; no CoreRef. |

**Gating:** No CoreRef; cadence on-change + at forced Core. **Remote:** RX updates last_seen_ms (any packet) and Operational fields from payload. **Self:** TX of Operational does **not** update self last_seen_ms (non–presence-bearing per §4); payload updates self NodeEntry Operational fields when we send. Encoding: [tail2_packet_encoding_v0](../contract/tail2_packet_encoding_v0.md) §3.2.

### 3.6 Node_Informative

| Field | Source | Consumer | Gating |
|-------|--------|----------|--------|
| (common: node_id, seq16) | RX | NodeEntry, BLE, on-air | — |
| maxSilence10s, hwProfileId, fwVersionId | RX (payload, optional) | NodeEntry | On change + every 10 min; MUST NOT send on every Operational. |

**Gating:** Static/config; cadence on-change + every 10 min. **Remote:** RX updates last_seen_ms and Informative fields. **Self:** TX of Informative does **not** update self last_seen_ms (non–presence-bearing per §4); payload updates self NodeEntry Informative fields when we send. **maxSilence10s** is the source for **expected_interval_s** (presence/staleness). Encoding: [info_packet_encoding_v0](../contract/info_packet_encoding_v0.md) §3.2.

---

## 4) Presence and staleness summary

- **expected_interval_s and grace_s (#371):**  
  `expected_interval_s = max_silence_10s * 10`; `grace_s = clamp(3, 30, round(expected_interval_s * 0.10))` (seconds); `is_stale = (last_seen_age_s > expected_interval_s + grace_s)`. FW-owned; not user-configurable. Source: [presence_and_age_semantics_v0.md](presence_and_age_semantics_v0.md) §2.

- **Self presence TX update set (#372):**  
  **Update self last_seen_ms:** Node_Alive, Node_Core_Pos, Node_Core_Tail (Tail when sent — paired with Core). **Do NOT update self last_seen_ms:** Node_Operational, Node_Informative. **pos_age_s for self:** Updated **only** on Node_Core_Pos TX (position-bearing). Source: [presence_and_age_semantics_v0.md](presence_and_age_semantics_v0.md) §1.D.

- **Interaction:** For **remote** entries, any accepted Node_* RX updates last_seen_ms. For **self**, only presence-bearing TX (Alive, Core_Pos, Core_Tail) update last_seen_ms. **last_seen_age_s** and **is_stale** are derived at export from last_seen_ms and expected_interval_s + grace_s. max_silence liveness is satisfied by sending Alive or Core_Pos (and Core_Tail when sent with Core); Operational/Informative do not satisfy liveness.

---

## 5) Persistence notes (from master table)

- **Persisted (Y in fields_v0_1):** node_id, positionLat/Lon (lat_e7, lon_e7), seq16/last_seq, ref_core_seq16/last_core_seq16, fwVersionId, hwProfileId, maxSilence10s, batteryPercent, uptimeSec, sats, posFlags, last_seen_ms (internal; BLE exports last_seen_age_s derived), last_rx_rssi, and other NodeEntry members per CSV **persistence_notes** (e.g. BLE snapshot, in NodeEntry). See fields_v0_1.csv columns **persisted**, **persistence_notes**.
- **Not persisted / RAM-only (or derived at export):** last_seen_age_s, is_stale (computed at export); last_payload_version_seen (debug only); some Injected/Derived per CSV. **WIP/Planned** fields may not yet have persistence implemented; master table **persistence_notes** and **producer_status** indicate current state. This doc does not add new persistence; only reflects the table.

---

## 6) Cross-links

| Doc / issue | Description |
|-------------|-------------|
| [#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358) | This task (Packet → NodeTable mapping). |
| [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) | S03 umbrella planning. |
| [NodeTable master table](../../../wip/areas/nodetable/master_table/README.md) | [fields_v0_1.csv](../../../wip/areas/nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../../wip/areas/nodetable/master_table/packets_v0_1.csv) — source of truth. |
| [presence_and_age_semantics_v0.md](presence_and_age_semantics_v0.md) | grace_s (§2), self presence TX (§1.D); #371, #372. |
| [ootb_autonomous_start_v0](../../firmware/policy/ootb_autonomous_start_v0.md) | Boot-time packet start; when we start sending what (§5). |
| [provisioning_baseline_v0](../../firmware/policy/provisioning_baseline_v0.md) | Phase A/B inputs (role, radio profile); what feeds NodeTable config. |
| [nodetable_field_map_v0.md](nodetable_field_map_v0.md) | Field-level index for #352; master table as field truth. |
| [gnss_tail_completeness_and_budgets_s03.md](../../../wip/areas/nodetable/contract/gnss_tail_completeness_and_budgets_s03.md) ([#359](https://github.com/AlexanderTsarkov/naviga-app/issues/359)) | GNSS Tail semantics (WIP); Tail completeness; budgets; UI guidance. |
