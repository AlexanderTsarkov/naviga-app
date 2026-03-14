# Preflight research: #418 (NodeTable persistence) vs #419 (field alignment)

**Scope:** Decide whether #418 can proceed on current NodeTable fields with a narrow snapshot boundary, or needs a preparatory slice from #419 first.

**Sources:** firmware `domain/node_table.h` + `node_table.cpp`, issues #418/#419, canon restore-merge-rules-v0, snapshot-semantics-v0, identity_naming_persistence_eviction_v0, persistence-v0, presence_and_age_semantics_v0, fields_v0_1.csv, s03_nodetable_slice_v0.

---

## Decision

**A) #418 can proceed safely now on current fields with a narrow snapshot boundary.**

---

## Why

1. **NodeEntry already has all S03-relevant fields** needed for snapshot/restore: `node_id`, `short_id`, `is_self`, `pos_valid`, `short_id_collision`, `lat_e7`, `lon_e7`, `pos_age_s`, `last_rx_rssi`, `last_seq`, `last_seen_ms`, `last_core_seq16`, `has_core_seq16`, `last_applied_tail_ref_core_seq16`, `has_applied_tail_ref_core_seq16`, `pos_flags`, `sats`, battery/uptime, `max_silence_10s`, `hw_profile_id`, `fw_version_id`. No missing canon field for the S03 NodeTable slice that would block defining a persisted snapshot.

2. **Tier/pin are explicitly out of current firmware scope.** identity_naming_persistence_eviction_v0 §4: "Current reality: NodeTable in firmware is RAM-only. No tiers (T0/T1/T2) or pin in firmware. Target (v1, planned): eviction policy aligned with tiers and pin." So #418 can define snapshot/restore for **current** NodeEntry; tier/pin stay out of the persisted record and eviction remains oldest-grey-first until eviction v1 (or #419 if it adds tier/pin).

3. **Self node_name** is S03 self-only NVS per #368 and identity doc §2; it is a **separate** concern (single key, self-only). It does not belong in the NodeTable snapshot format. No need to add `node_name` to NodeEntry for #418.

4. **Current 26-byte BLE record is an export subset**, not the persistence schema. get_page/get_snapshot_page today write: node_id, short_id, flags (is_self, pos_valid, grey, short_id_collision), **age_s** (derived at snapshot time), lat/lon, pos_age_s, last_rx_rssi, last_seq. They do **not** write `last_seen_ms`, `last_core_seq16`, `has_core_seq16`, or telemetry. For NVS, #418 must define a **persistence-specific** format that includes at least `last_seen_ms` (canon: "lastSeen so lastSeenAge can be derived after restore") and `last_core_seq16`/`has_core_seq16` for Tail-1 ref gate on restore. That is a **schema/implementation choice inside #418** (new or extended record layout), not a struct change. The fields to persist are already in NodeEntry.

5. **Canon snapshot-semantics** (persisted / derived / prohibited) can be satisfied with current struct: persist identity, position, lastSeen (last_seen_ms), last_seq, last_core_seq16, telemetry; derive lastSeenAge and is_stale on restore; do not persist per-packet/dedup state. `last_applied_tail_ref_core_seq16` is derived/injected per CSV (persisted N); can be cleared or not persisted on restore.

6. **#419** is about aligning NodeTable to the full canon field map and master table (missing fields, stubs, naming, layout). That is cleanup and completeness. It does not add any field that is **required** for a minimal, correct snapshot/restore satisfying restore-merge-rules and snapshot-semantics. So no hard dependency of #418 on #419.

7. **Narrow boundary:** #418 should document the snapshot format (what is persisted / derived / prohibited), implement save/restore and merge rules per canon, and **not** persist tier/pin or node_name in the NodeTable snapshot. That keeps the scope clear and avoids baking in wrong semantics.

---

## Current NodeTable vs canon (S03-relevant only)

| Field / group | In NodeEntry | Canon (S03) | Classification |
|---------------|--------------|-------------|----------------|
| node_id (DeviceId) | ✓ | Primary key | **aligned** |
| short_id | ✓ | Optional, last-known | **aligned** |
| is_self | ✓ | Local-only | **aligned** |
| short_id_collision | ✓ | UI safeguard | **aligned** |
| pos_valid, lat_e7, lon_e7, pos_age_s | ✓ | lastKnownPosition | **aligned** |
| last_seen_ms | ✓ | lastSeen (persist so age derivable) | **aligned** |
| last_seq | ✓ | seq16 / dedupe | **aligned** |
| last_core_seq16, has_core_seq16 | ✓ | ref_core_seq16 / Tail-1 gate | **aligned** |
| last_applied_tail_ref_core_seq16 | ✓ | Variant 2 dedupe | **usable with caveat** (derived; clear or don’t persist on restore) |
| pos_flags, sats | ✓ | Tail-1 position quality | **aligned** |
| battery, uptime, max_silence_10s, hw_profile_id, fw_version_id | ✓ | Telemetry/Informative | **aligned** |
| last_rx_rssi | ✓ | Link metric | **aligned** |
| in_use | ✓ | Internal slot | **aligned** (not persisted; slot occupancy) |
| tier (T0/T1/T2), pin | ✗ | Target v1, not current FW | **missing** — intentional; out of scope for #418 |
| node_name | ✗ | Self-only NVS (#368) | **missing** — separate from NodeTable snapshot |

---

## Snapshot viability for #418

**Persisted now (from NodeEntry, in NVS snapshot):**

- Identity: node_id, short_id (optional last-known).
- Position: pos_valid, lat_e7, lon_e7, pos_age_s.
- Time: last_seen_ms (so lastSeenAge can be derived on restore).
- Activity/seq: last_seq, last_core_seq16, has_core_seq16.
- Telemetry (policy): battery_percent, uptime_sec, max_silence_10s, hw_profile_id, fw_version_id (with has_*).
- Tail-1 quality: pos_flags, sats (with has_*).
- Link: last_rx_rssi.
- Flags: is_self, pos_valid, short_id_collision (and slot presence for restore).

**Derived on restore:**

- last_seen_age_s, is_stale (from last_seen_ms + expected_interval_s + grace_s).
- last_applied_tail_ref_core_seq16 / has_applied_tail_ref_core_seq16: clear or leave zero until first Tail-1 apply after restore.
- short_id_collision (recompute if desired).

**Prohibited from persistence:**

- Per-packet / dedup state (not in NodeEntry).
- in_use as “authoritative” (restore sets slot occupancy from “record present in snapshot”).
- tier/pin (not in struct; out of scope S03 FW).

---

## Blocking mismatches

**None.** All S03-relevant fields for snapshot/restore exist in NodeEntry. Tier/pin and node_name are explicitly out of scope or handled elsewhere (#368, eviction v1).

---

## Recommended path

**Proceed with #418 now.**

1. Implement snapshot format (new or extended record layout for NVS) that includes the fields listed under “Persisted now” above; do **not** use the 26-byte BLE export as the NVS format (it omits last_seen_ms and core_seq/telemetry).
2. Implement restore that fills NodeEntry from the persisted snapshot and recomputes derived fields (last_seen_age_s, is_stale, tail ref as above).
3. Wire save/restore in app/platform with bounded write cadence (per #418 DoD).
4. Document format and policy (persisted / derived / prohibited) in canon or ADR; reference restore-merge-rules and snapshot-semantics.
5. Do **not** add tier/pin or node_name to the NodeTable snapshot in #418; leave eviction as oldest-grey-first and self node_name to #368.

---

## If tiny preparatory alignment is needed

Not required for the decision above. If during #418 implementation we discover a single field that is wrongly typed or named and blocks a clean schema (e.g. one has_* missing for a telemetry field), that can be a **tiny** prep change (one field, domain + tests). That would not be “doing all of #419,” which is full field-map/master-table alignment, stubs, and layout.

---

## Risks to avoid

1. **Using the 26-byte BLE record as the NVS format** — it lacks last_seen_ms and core_seq/telemetry; restore would be incorrect for Tail-1 gate and presence/age.
2. **Persisting last_seen_age_s instead of last_seen_ms** — canon requires lastSeen (timestamp) so lastSeenAge is derived on restore.
3. **Adding tier/pin or node_name into NodeTable snapshot in #418** — tier/pin are not in struct and are eviction v1; node_name is self-only and #368.
4. **Baking “eviction v1” or “tier-aware restore” into #418** — keep restore semantics “restore last-known state; eviction stays oldest-grey-first.”
5. **Scope creep into #419** — do not refactor all fields to match master table in #418; only what is needed for a correct snapshot/restore schema.
6. **Deriving snapshot semantics from current 26-byte export** — snapshot semantics come from canon (snapshot-semantics-v0, restore-merge-rules-v0), not from the existing BLE page layout.

---

## Exit criteria checklist

- [x] #418 analyzed against current code
- [x] #419 analyzed only as needed to judge readiness/blockers
- [x] Canon-vs-code mismatch identified for S03-relevant fields
- [x] Explicit recommendation: A (proceed with #418 now)
- [x] Smallest safe next step: implement #418 with a narrow snapshot boundary and persistence-specific format; no preparatory #419 slice required
