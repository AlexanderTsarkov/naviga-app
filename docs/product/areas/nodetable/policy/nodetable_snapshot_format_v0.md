# NodeTable persistence snapshot format (S03 / #418)

**Status:** Canon (implementation-aligned).  
**Issue:** #418 — NodeTable persistence snapshot format + restore policy.

This doc describes the **firmware** NVS snapshot format and restore semantics. Classification (persisted vs runtime vs derived) follows [product_truth_s03_v1](../../../wip/areas/nodetable/product_truth_s03_v1.md) and [seq_ref_version_link_metrics_v0](seq_ref_version_link_metrics_v0.md). BLE export uses a different record layout (26-byte); see BLE/snapshot bridge.

---

## 1) Blob layout

- **Header:** 5 bytes — magic `'N'` `'T'`, format **version** (3 or 4), entry count (uint16 LE). Accepted: **v3** (35-byte record), **v4** (68-byte record, adds node_name). Rejected: v1, v2 (legacy).
- **Record v3:** 35 bytes — node_id (8), short_id (2), flags (1), lat_e7 (4), lon_e7 (4), pos_age_s (2), flags2 + pos_flags/sats (3), flags3 + battery/uptime/max_silence/hw/fw (10). No node_name.
- **Record v4:** 68 bytes — v3 layout + node_name (1 byte length + 32 bytes; canonical single name field, #419).
- **Not in blob:** last_seen_ms, last_seq, last_rx_rssi, snr_last, legacy ref-state (canon §7).

---

## 2) Restore policy

- **Persisted fields:** Loaded from blob into NodeEntry.
- **Derived:** is_self = (node_id == self_node_id); set after unpack.
- **Runtime / not persisted:** last_seen_ms = 0, last_seq = 0, last_rx_rssi = 0, snr_last = NA, legacy ref* = 0/false, short_id_collision = false. Restored entries **start stale**; last_seen_age_s / is_stale derived until first RX/TX.

---

## 3) Related

- [product_truth_s03_v1](../../../wip/areas/nodetable/product_truth_s03_v1.md) §3–§7 — receiver-injected, derived, legacy removed.
- [seq_ref_version_link_metrics_v0](seq_ref_version_link_metrics_v0.md) — last_seq, legacy ref fields not in canon.
- [identity_naming_persistence_eviction_v0](identity_naming_persistence_eviction_v0.md) — identity and eviction.
- Implementation: `firmware/src/domain/nodetable_snapshot.cpp`, `naviga_storage` NVS keys `nt_snap_len` / `nt_snap`.
