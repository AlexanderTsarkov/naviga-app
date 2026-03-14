# NodeTable — Identity, Naming, Persistence & Eviction (S03 block v0)

**Status:** Canon (policy).  
**Parent:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351), [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352), [#364](https://github.com/AlexanderTsarkov/naviga-app/issues/364).

This document is the **canonical reference** for the S03 "Identity + Naming + Persistence/Eviction" block. It consolidates intent so the block is not revisited during S03. No firmware code changes; documentation and master-table alignment only.

---

## 1) Identity fields (current + S03)

| Field | Model / BLE | On-air | Notes |
|-------|-------------|--------|--------|
| **node_id** | uint64 (lower 48 bits) | nodeId48, 6 B LE | Primary key; domain and BLE use u64; on-air 6-byte LE MAC48. |
| **is_self** | bool | — | Local-only; true for the node that matches local device identity. Exported in BLE snapshot (flags bit0). |
| **short_id** | uint16 | — | CRC16-CCITT over node_id LE; display only; not protocol key. Reserved 0x0000/0xFFFF handled. |
| **short_id_collision** | bool | — | UI safeguard when two nodes share the same short_id; BLE snapshot (flags bit3). |

---

## 2) Human-readable node name (node_name)

- **node_name:** Short display label for a node (map/list/discovery).
  - **S03 scope:** Self-only. Set via BLE, persisted in NVS on the device. Remote entries usually empty (no over-air propagation in S03).
  - **Sxx (future):** Remote node_name may be populated from over-air propagation (e.g. session or Informative); when present, it is the **authoritative** value.
- **Label constraints (S04 #466):** **Product/UI limit:** max 12 characters. **Storage/transport ceiling:** max 24 bytes UTF-8. **Allowed:** Latin letters, Cyrillic letters, digits `0`–`9`, symbols `-`, `_`, `#`, `=`, `@`, `+`. **Disallowed:** emoji/pictograms, control characters, symbols outside the allowlist. Rationale: compact readable label, predictable BLE/storage footprint, future radio-friendly upper bound.
- **Authority rule:** Node-reported name is **authoritative** and **replaces** any previously stored local phone name for that node. If the node (or self via BLE) provides a name, that value wins.
- **UI fallback order:** `node_name` (authoritative) → local phone name (only if no authoritative name) → short_id.

*Out of scope S03:* **networkName** as a separate broadcast/session concept and **ownership/claim** (device binding, BLE write-protect, PIN, lock) are **not** part of this block. See §6.

---

## 3) Local phone name (fallback)

- Optional app-side label per node when **node_name** is empty. Stored on phone only; not sent over radio in S03. When over-air **node_name** appears later (Sxx), it replaces this fallback for display (authority rule above).

---

## 4) Persistence v1 + Eviction v1 (firmware NodeTable) — planned work

- **Current reality:** NodeTable in firmware is **RAM-only**. No snapshot to NVS. Eviction is **oldest-grey-first** (non-self, grey entries only). No tiers (T0/T1/T2) or pin in firmware.
- **Target (v1, planned):** Persistence of NodeTable snapshot to NVS; restore on boot; merge rules with incoming RX per policy (e.g. [restore-merge-rules-v0](../../../wip/areas/nodetable/policy/restore-merge-rules-v0.md)). Eviction policy aligned with tiers (T0/T1/T2) and pin where applicable. Implemented via dedicated issues (see §7).

---

## 5) Out-of-scope S03 (explicit)

- **networkName:** Broadcast/session name concept; NOT in S03 scope. Future session-join or similar.
- **Ownership / claim / pairing (security):** Binding phone account or user identity to node_id; preventing others from overwriting settings over BLE; claimed/paired/owner state; PIN, key, or lock. All deferred; not part of this block.

---

## 6) Related docs (no duplication)

- [packet_sets_v0.md](packet_sets_v0.md) — Packet definitions v0.
- [tx_priority_and_arbitration_v0.md](tx_priority_and_arbitration_v0.md) — TX priority P0–P3.
- [source-precedence-v0.md](../../../wip/areas/nodetable/policy/source-precedence-v0.md), [restore-merge-rules-v0.md](../../../wip/areas/nodetable/policy/restore-merge-rules-v0.md), [persistence-v0.md](../../../wip/areas/nodetable/policy/persistence-v0.md), [snapshot-semantics-v0.md](../../../wip/areas/nodetable/policy/snapshot-semantics-v0.md) — Precedence, merge, retention tiers, snapshot content (reference only; no duplication here).

---

## 7) Issues (linked to #351, #352, #364)

- **[#367](https://github.com/AlexanderTsarkov/naviga-app/issues/367) — FW: NodeTable persistence v1 + eviction v1** — Snapshot to NVS, restore, tier-aware eviction.
- **[#368](https://github.com/AlexanderTsarkov/naviga-app/issues/368) — FW/BLE: self node_name read/write + NVS persistence (self-only)** — BLE read/write of self name; persist in NVS.
- **[#369](https://github.com/AlexanderTsarkov/naviga-app/issues/369) — App: local phone name storage + authoritative-replaces-local rule** — Optional; can be future if app not in S03.
