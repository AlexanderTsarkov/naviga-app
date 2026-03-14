# Research: last_seen_ms and #418 persistence policy

**Goal:** Validate whether `last_seen_ms` should be persisted in the #418 NodeTable snapshot and determine correct restore behavior. Research-only; no code changes.

---

## Canon definition

- **What `last_seen_ms` means in S03**  
  From `docs/product/areas/nodetable/policy/presence_and_age_semantics_v0.md` §1.A:  
  **"last_seen_ms: Monotonic uptime (ms) of the last presence event for this entry."**  
  It is the single source of truth for “when we last had a presence signal” from this node. Presence event: for **remote** entries = any accepted Node_* RX from that node; for **self** = TX heartbeat updates per §1.D / [#372](https://github.com/AlexanderTsarkov/naviga-app/issues/372) (Alive, Core_Pos, Core_Tail; not Operational/Informative).

- **Reboot-safety**  
  **Not reboot-safe.** It is explicitly **monotonic uptime** (ms). After reboot, uptime resets; a value stored before reboot belongs to a different epoch and cannot be meaningfully compared to post-reboot `now_ms`. Persisting it would either (a) produce nonsensical ages or (b) require a separate “wall-clock” or “time-since-boot” interpretation that canon does not define. Canon does not provide a reboot-safe “timestamp or equivalent” for S03; the only internal anchor is uptime.

- **Relationship to `last_seen_age_s` and `is_stale`**  
  - **last_seen_age_s:** §1.B — derived at snapshot/export time: `floor((now_ms - last_seen_ms) / 1000)`. Not a stored field; computed from `last_seen_ms`.  
  - **is_stale:** §1.B and §2 — `last_seen_age_s > (expected_interval_s + grace_s)`. Boolean; replaces legacy **is_grey**.  
  So both are **derived** from `last_seen_ms` and policy (expected_interval_s, grace_s). Restore-merge-rules-v0 §5 explicitly: *"Activity / stale: **Derived** (activityState, lastSeenAge, staleness). Recomputed after merge from lastSeen and policy-supplied boundary; **not** persisted."*

---

## Current code reality

- **Where `last_seen_ms` is produced**  
  - **Written:** Always as `now_ms` (current tick/runtime time) at presence events:  
    - Remote: `node_table.cpp` in `init_self`, `upsert_remote`, `apply_tail1`, `apply_tail2`, `apply_info` (every branch that updates an entry).  
    - Self: `init_self`, `update_self_position`, `touch_self`.  
  - So in code, `last_seen_ms` is **always** “uptime of last presence event”; no wall-clock or persistent timestamp.

- **Where it is consumed**  
  - **Age:** `age_ms = now_ms >= entry.last_seen_ms ? (now_ms - entry.last_seen_ms) : 0` in `get_page`, `get_peer_dump_line`, `is_grey`, `get_snapshot_page` (BLE export), eviction (`evict_oldest_grey`).  
  - **Staleness:** `is_grey(entry, now_ms)` uses that age and `expected_interval_s_ + grace_s_`.  
  - **PR #429:** `nodetable_snapshot.cpp` **packs** `last_seen_ms` at offset 21–24 and **unpacks** it into `e->last_seen_ms` on restore.

- **Runtime-only vs durable**  
  Current firmware treats it as **runtime-only**: it is only ever set from `now_ms` in the current boot. No code today “survives reboot” for it except the new #418 snapshot path, which persists and restores it — and that is exactly what is under review. So the only place that currently treats it as durable is the #418 snapshot format in PR #429; canon and the rest of the code treat it as an uptime anchor.

---

## Recommended #418 policy

**Choose: A) Exclude `last_seen_ms` entirely from persistence; restore with no persisted presence anchor.**

**Why:**

1. **Canon:** S03 defines `last_seen_ms` as **monotonic uptime** of the last presence event. That is inherently boot-relative; there is no S03-defined “timestamp or equivalent” that survives reboot.
2. **Snapshot-semantics (WIP)** says persist “lastSeen (timestamp or equivalent) so lastSeenAge can be derived after restore.” In S03 we have **no** reboot-safe equivalent; the only internal anchor is uptime. So the correct S03 behavior is **not** to persist lastSeen; lastSeenAge/staleness are then derived from “no anchor” until the first RX/TX after restore.
3. **Restore-merge-rules:** Activity/staleness are **derived**, not persisted; they are “recomputed from lastSeen and policy.” If we do not persist lastSeen, we have no stored anchor; recomputation means “no anchor until first presence event.”
4. **Smallest canon-correct behavior:** Do not persist a field whose semantics are boot-relative. Re-establish presence from fresh RX (remote) and TX (self) after boot. No new product semantics; no new persisted field.

---

## Restore behavior after reboot

- **Remote entries**  
  - **last_seen_ms:** Set to **0** (no presence anchor). Do not load from snapshot.  
  - **last_seen_age_s:** Derived as `floor((now_ms - 0) / 1000)` = current uptime in seconds → large.  
  - **is_stale:** Derived: `last_seen_age_s > expected_interval_s + grace_s` → **true** until first RX from that node.  
  - **Position (and other persisted fields):** Restored as-is (position, telemetry, etc. stay). Presence is “unknown/stale”; position is “last known.”

- **Self entry**  
  - **last_seen_ms:** **0** (same rule; no fake continuity).  
  - **last_seen_age_s / is_stale:** Same derivation → self appears stale until first presence-bearing TX (Alive, Core_Pos, or Core_Tail) after boot.  
  - No special case: self also re-establishes presence on first TX.

- **Summary**  
  - Preserve position and other persisted fields.  
  - Do **not** preserve presence across reboot: no persisted presence anchor; all entries (including self) start with “no anchor” (e.g. `last_seen_ms = 0`).  
  - Derived: `last_seen_age_s` large, `is_stale` true until first RX/TX.  
  - First RX (remote) or first presence-bearing TX (self) sets `last_seen_ms = now_ms` and restores normal presence semantics.

---

## Minimal implementation impact on #429

- **Snapshot schema**  
  - **Option (preferred):** Remove `last_seen_ms` from the persisted record: drop the 4 bytes at current offset 21–24; shift `last_core_seq16` and following fields down; record size becomes 36 bytes.  
  - **Option (minimal change):** Keep record layout and 40 bytes, but **do not** persist/restore a meaningful value: pack 0 for that slot, unpack to 0. No schema shrink; behavior correct.

- **Restore logic**  
  - In `unpack_record`: **do not** read `last_seen_ms` from the blob. Set `e->last_seen_ms = 0` (no presence anchor).  
  - If schema is shrunk: adjust offsets in both `pack_record` and `unpack_record` and set `kNodeTableSnapshotRecordBytes = 36`.

- **Tests**  
  - **Update:** `test_nodetable_snapshot_restore_is_self_derived` (and any other test that asserts `e_self.last_seen_ms` / `e_remote.last_seen_ms`): assert **0** after restore instead of the pre-snapshot values (1000, 2000).  
  - **Add (optional):** Narrow test that restored entries have `last_seen_ms == 0` and (if we expose or derive it) are treated as stale until first RX.

- **PR #429**  
  - **Remains the right PR.** Change is a small, scope-contained correction: stop persisting/restoring `last_seen_ms` and fix tests. No need for a new PR or a separate “presence redesign” ticket.

---

## Risks to avoid

- **Persisting `last_seen_ms`** and reinterpreting it after reboot (e.g. as “seconds ago”) — invents semantics not in canon and mixes epochs.  
- **Persisting `last_seen_age_s`** instead — age at snapshot time is not a valid “presence anchor” after reboot; it would still be boot-relative and would not define “when we last saw them” in the new boot.  
- **Faking continuity** (e.g. setting `last_seen_ms = now_ms` on restore so everyone looks “just seen”) — violates “re-establish presence from fresh RX/TX.”  
- **Expanding scope** into #421 (RX semantics) or a full presence/activity redesign.  
- **Rewriting canon** — use existing definitions; only align implementation and snapshot policy with them.

---

## Exit criteria checklist

- [x] Canonical meaning of `last_seen_ms` confirmed (monotonic uptime of last presence event; S03 presence_and_age_semantics_v0 §1).
- [x] Reboot-safety judgment explicit (not reboot-safe; do not persist).
- [x] #418 persistence recommendation explicit (A: exclude entirely; restore with no persisted presence anchor).
- [x] Restore behavior after reboot spelled out (last_seen_ms = 0; position preserved; last_seen_age_s / is_stale derived; first RX/TX re-establishes presence).
- [x] Minimal PR #429 adjustment identified (remove or zero last_seen_ms in schema/restore; update tests; PR #429 remains the vehicle).
