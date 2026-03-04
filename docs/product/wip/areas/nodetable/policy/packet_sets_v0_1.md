# NodeTable — Packet sets v0.1 (WIP)

**Status:** WIP. S03 packet definitions and eligibility. **Parent:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352).

This document defines the **v0.1** packet sets and TX eligibility rules for Node_Core_Tail, Node_Operational, and Node_Informative. It extends the v0 canon layout intent with packed fields and eligibility; **canon v0 contracts remain normative** for existing wire formats. Any mismatch with v0 canon is called out in § Discovery and conflicts.

**Identity & naming:** See [identity_naming_persistence_eviction_v0_1.md](identity_naming_persistence_eviction_v0_1.md) for the S03 block (node_id, is_self, short_id, short_id_collision, node_name, authority rule, persistence/eviction scope).

---

## 1) Node_Core_Tail v0.1

- **Priority:** P2 ([tx_priority_and_arbitration_v0_1.md](tx_priority_and_arbitration_v0_1.md)).
- **Useful payload (v0.1):** `ref_core_seq16` (2 B) + **pos_quality16** (2 B, packed).
- **pos_quality16 packing:** Single uint16 LE carrying:
  - **fix_type** (e.g. 2 bits): no_fix / 2d / 3d / reserved.
  - **sats** (e.g. 6 bits): satellite count; 0 = not present.
  - **accuracy_bucket** (e.g. 3 bits): position accuracy bucket.
  - **pos_flags_small** (e.g. 5 bits): small position-quality flags (replaces/extends v0 `posFlags` byte in intent; see § Discovery).
- **Eligibility:** Attached to Core sample; send when Core_Pos was sent and min_interval / coalesce allow. One Tail per Core sample (ref_core_seq16).

---

## 2) Node_Operational v0.1

- **Priority:** P3.
- **Useful payload (v0.1):** 4 bytes total:
  - **battery16:** pct7 (0–100) + charging1 + eta10m8 (e.g. minutes to empty, 0 = N/A).
  - **radio8:** txPower4 (tx power step) + throttle4 (channel throttle step).
  - **uptime10m_u8:** uptime in 10-minute units; masked-for-compare for eligibility (snapshot mask).
- **Eligibility:** Changed-gated + min_interval. Compare snapshot masks for uptime; send when any Operational field changed or min_interval elapsed. **Reboot exception:** After reboot, allow one send even if snapshot unchanged.
- **Coalesce:** By snapshot of (battery16, radio8, uptime10m) for replacement.

*Note: v0 canon Operational uses batteryPercent (1 B) + uptimeSec (4 B). v0.1 is a **new packed layout** (4 B total); canon v0 layout remains for backward compat where applicable.*

---

## 3) Node_Informative v0.1

- **Priority:** P3.
- **Useful payload (v0.1):** `max_silence_10s` (1 B) + `hw_profile_id` + `fw_version_id` + **role_id** (new).
- **Eligibility:** Periodic + changed-boost (NOT changed-only). min_interval = max_silence10 (or configured minimum). Send on change of any Informative field **or** on periodic timer; when changed, boost next send.
- **CONFLICT (field width):** See § Discovery and conflicts. Canon v0 defines **hwProfileId** and **fwVersionId** as **uint16 LE (2 bytes each)**. v0.1 does **not** silently change them.

---

## 4) Discovery and conflicts vs v0 canon

The following v0 canon layouts were read from the **existing contracts** (read-only reference). No silent change to these.

### 4.1 v0 canon field sizes (summary)

| Source | Field(s) | Type / size | Notes |
|--------|----------|-------------|--------|
| [tail1_packet_encoding_v0.md](../../../../areas/nodetable/contract/tail1_packet_encoding_v0.md) §3 | ref_core_seq16 | uint16 LE, 2 B | Core linkage key. |
| [tail1_packet_encoding_v0.md](../../../../areas/nodetable/contract/tail1_packet_encoding_v0.md) §3.2 | posFlags | uint8, 1 B | Bit 0 = position valid; 1–7 reserved. |
| [tail1_packet_encoding_v0.md](../../../../areas/nodetable/contract/tail1_packet_encoding_v0.md) §3.2 | sats | uint8, 1 B | Satellite count; 0 = not present. |
| [tail2_packet_encoding_v0.md](../../../../areas/nodetable/contract/tail2_packet_encoding_v0.md) §3.2 | batteryPercent | uint8, 1 B | 0–100; 0xFF = not present. |
| [tail2_packet_encoding_v0.md](../../../../areas/nodetable/contract/tail2_packet_encoding_v0.md) §3.2 | uptimeSec | uint32 LE, 4 B | 0xFFFFFFFF = not present. |
| [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) §3.2 | maxSilence10s | uint8, 1 B | 0x00 = absent. |
| [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) §3.2 | **hwProfileId** | **uint16 LE, 2 B** | 0xFFFF = not present. |
| [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) §3.2 | **fwVersionId** | **uint16 LE, 2 B** | 0xFFFF = not present. |

### 4.2 CONFLICT: HW/FW field width (Informative)

**Existing v0 canon:** `hwProfileId` and `fwVersionId` are **2 bytes each (uint16 LE)** in [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) §3.2. Informative payload is 9 (common) + 1 + 2 + 2 = **14 bytes** max with these fields.

**v0.1 intent:** Node_Informative v0.1 adds `role_id` and keeps max_silence_10s + hw_profile_id + fw_version_id. **No silent change** to hw/fw width.

**Options (explicit decision required; do not assume):**

| Option | Description |
|--------|-------------|
| **Option 1** | Keep **u16** for hw_profile_id and fw_version_id in Informative v0.1. Payload size and LoRa step thresholds remain as in v0. No mapping table. |
| **Option 2** | Introduce **compact u8 aliases** for v0.1 only (e.g. hw_profile_id_u8, fw_version_id_u8) with a **mapping table** from registry to u8. Wire format would then differ from v0; requires explicit contract update and migration path. |

**Decided:** Keep **hwProfileId** and **fwVersionId** as **uint16 LE (2 B each)** in Informative layout. Option 1. Conflict closed. [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#364](https://github.com/AlexanderTsarkov/naviga-app/issues/364).

### 4.3 Naming and supersession (Tail-1 / position quality)

- **posFlags (v0):** 1-byte position-quality flags. In v0.1, **pos_quality16** packs **pos_flags_small** (fewer bits). Document mapping: pos_flags_small is the v0.1 packed form; posFlags remains in canon v0. Master table: mark **posFlags** as superseded by pos_flags_small for v0.1; keep posFlags row for v0.
- **sats (v0):** 1-byte satellite count. In v0.1 packed inside **pos_quality16**. Canonical field_key remains **sats**; **pos_sats** is alias (superseded-by sats) if used elsewhere. Master table: one canonical sats row; optional pos_sats alias row marked superseded-by sats.

---

## 5) Implementation (FW alignment)

Firmware matches this policy: **Core_Pos** and **Alive** use P0; **Node_Core_Tail** (Tail1) uses P2; **Node_Operational** and **Node_Informative** use P3. The hybrid **expired_counter** is implemented as `replaced_count` in the TX queue: +1 on replacement by same coalesce_key, +1 when due/eligible but not sent (starvation), reset on send, preserved across replacement. Intra-priority ordering uses `replaced_count` DESC. Implementation status: implemented in FW; tabletop validation deferred. [#364](https://github.com/AlexanderTsarkov/naviga-app/issues/364), [#365](https://github.com/AlexanderTsarkov/naviga-app/issues/365).

---

## 6) Related

- [tx_priority_and_arbitration_v0_1.md](tx_priority_and_arbitration_v0_1.md) — P0–P3, coalesce_key, expired_counter.
- [beacon_payload_encoding_v0.md](../../../../areas/nodetable/contract/beacon_payload_encoding_v0.md) — Canon payload layouts.
- [tail1_packet_encoding_v0.md](../../../../areas/nodetable/contract/tail1_packet_encoding_v0.md), [tail2_packet_encoding_v0.md](../../../../areas/nodetable/contract/tail2_packet_encoding_v0.md), [info_packet_encoding_v0.md](../../../../areas/nodetable/contract/info_packet_encoding_v0.md) — Canon v0 contracts.
- [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352).
