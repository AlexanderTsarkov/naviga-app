# NodeTable Field Map v0 — single entry for #352 / S03 slice

**Status:** Canon (policy).  
**Issue:** [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352) · **Parent:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351).

This doc is the **field-level index** only. It does not duplicate the field table; it declares the master table as source of truth, explains how to read it, and links to policy docs and S03 slice. No semantic or field changes; documentation and links only.

---

## 1) Purpose

- **Single entry** for #352 and the S03 NodeTable slice: one place to find the field-level source of truth and how it relates to policy and execution.
- Enables closing #352 without losing traceability: master table + this index + slice doc + execution issues form the chain.

---

## 2) Source of truth (MUST be explicit)

- **fields_v0_1.csv** and **nodetable_master_table_v0_1.xlsx** are the **field-level source of truth** for NodeTable fields (and packets/sources in the same artifact set).
- They are **generated** by **build_master_table.py** (deterministic). The generator inputs live in that script; do not edit the CSV/XLSX by hand for field definitions.

| Artifact | Role |
|----------|------|
| [master_table/fields_v0_1.csv](../../../wip/areas/nodetable/master_table/fields_v0_1.csv) | Field rows (machine-readable) |
| [master_table/nodetable_master_table_v0_1.xlsx](../../../wip/areas/nodetable/master_table/nodetable_master_table_v0_1.xlsx) | Same data + packets + sources (human-friendly sheets) |
| [master_table/tools/build_master_table.py](../../../wip/areas/nodetable/master_table/tools/build_master_table.py) | Generator; edit this to change fields, then regenerate |

---

## 3) How to read the master table (brief)

Key columns in the **Fields** sheet / **fields_v0_1.csv**:

| Column | Meaning |
|--------|---------|
| **field_key** | Canonical field name (e.g. node_id, seq16, last_rx_rssi). |
| **scope_class** | Core / Injected / Derived / etc. |
| **status** | Canon, CodeNow, WIP, Superseded, Idea, etc. |
| **owner** | NodeOwned, ReceiverInjected, Derived, Config, etc. |
| **producer_status** / **source** | Implemented | Stubbed | Planned; HW | Derived | Injected | Config — who produces the value. |
| **sent_on_air** / **packet_name_canon** / **wire_bytes_fixed** | On-air: whether sent, in which packet, fixed byte size. |
| **exported_over_ble** | Whether the field is in the BLE snapshot (and **ble_record_offset** / **ble_bytes** if so). |
| **notes** | Semantics, update rules, contract refs, S03 scope notes. |

Use **field_key** + **status** + **notes** for implementation and traceability; use **on_air_contract_ref** and **packet_name_canon** for encoding contracts.

---

## 4) S03 scope lock

- The bounded S03 NodeTable scope (what is in/out, execution issues, promotion rule) is defined in the **S03 slice** doc.
- **Link:** [s03_nodetable_slice_v0.md](s03_nodetable_slice_v0.md).

---

## 5) Policy references

Field semantics and update rules are defined in policy docs; the master table rows reference them via **on_air_contract_ref** and **notes**. Key policy docs:

| Policy | Content |
|--------|---------|
| [identity_naming_persistence_eviction_v0.md](identity_naming_persistence_eviction_v0.md) | Identity, node_name, persistence/eviction scope. |
| [presence_and_age_semantics_v0.md](presence_and_age_semantics_v0.md) | last_seen_ms, last_seen_age_s, is_stale, pos_age_s. |
| [seq_ref_version_link_metrics_v0.md](seq_ref_version_link_metrics_v0.md) | seq16/ref_core_seq16, last_payload_version_seen, last_rx_rssi, snrLast. |
| [packet_sets_v0.md](packet_sets_v0.md) | Packet types, payloads, eligibility. |
| [tx_priority_and_arbitration_v0.md](tx_priority_and_arbitration_v0.md) | P0–P3, expired_counter, TX queue. |
| [packet_to_nodetable_mapping_v0.md](packet_to_nodetable_mapping_v0.md) ([#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358)) | Packet → NodeTable mapping (S03): which packet updates which fields, gating, presence (#371, #372). |

---

## 6) Out-of-scope reminders (short)

- **networkName**, **ownership/claim**, **over-air name propagation** — not in S03.
- **activityState**, **PositionQuality**, **aliveStatus** — policy-derived; not promoted to S03 first-class; UI uses is_stale, last_seen_age_s, pos_age_s, posFlags/sats.

---

## 7) Execution mapping

Issues that must be completed (or explicitly deferred) for S03; one-line purpose each:

| Issue | Purpose |
|-------|---------|
| [#367](https://github.com/AlexanderTsarkov/naviga-app/issues/367) | FW: NodeTable persistence v1 + eviction v1 (NVS snapshot, restore, tier-aware eviction). |
| [#368](https://github.com/AlexanderTsarkov/naviga-app/issues/368) | FW/BLE: self node_name read/write + NVS persistence (self-only). |
| [#369](https://github.com/AlexanderTsarkov/naviga-app/issues/369) | App: local phone name storage + authoritative-replaces-local rule (optional). |
| [#371](https://github.com/AlexanderTsarkov/naviga-app/issues/371) | grace_s value and placement (FW/app/both). |
| [#372](https://github.com/AlexanderTsarkov/naviga-app/issues/372) | Self presence TX update points (which TX counts as presence for self). |
| [#375](https://github.com/AlexanderTsarkov/naviga-app/issues/375) | FW: Parse E22 RSSI append, update last_rx_rssi in NodeTable. |
| [#376](https://github.com/AlexanderTsarkov/naviga-app/issues/376) | FW: snrLast NA/UNSUPPORTED sentinel, plumb through snapshots. |

---

## 8) Promotion to canon rule

**WIP → canon after:** (1) docs merged to canon paths, (2) linked execution issues implemented (or explicitly deferred), (3) tabletop checklist satisfied where applicable. Align with [s03_nodetable_slice_v0.md](s03_nodetable_slice_v0.md) §5.

---

## 9) How to update

- **Do not** edit the CSV or XLSX by hand for field definitions.
- **Do:** change the generator inputs in **build_master_table.py** (FIELDS_ROWS, FIELD_DEFAULTS, SOURCES_ROWS as needed) → run the script → commit the regenerated **fields_v0_1.csv**, **nodetable_master_table_v0_1.xlsx**, **sources_v0_1.csv** (and **packets_v0_1.csv** if changed).
- Update **references** (e.g. policy doc links, this field map, slice) only when paths or scope change; no manual table edits in markdown.
