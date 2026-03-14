# Audit: #296 and #355 after #417 / PR #428

## #296

- **Classification:** Fully satisfied
- **Why:**
  - Issue scope: persisted seq16 via NVS, restore on boot (restored > 0 ⇒ first TX uses restored + 1), write seq16 on each TX (or periodic flush), unit test, NVS adapter. Optional: legacy doc sweep (low urgency).
  - #417 / PR #428 delivered: narrow NVS adapter (`load_seq16` / `save_seq16` in `naviga_storage`), restore via `set_initial_seq16(restored)` after `runtime_.init()`, per-TX persistence with validity-aware handling (including seq16 == 0 after wraparound), and unit tests (default first packet 1; set_initial 42→43, 100→101; wraparound 65535→0).
  - Injection is via `BeaconLogic::set_initial_seq16(value)` (same semantics as “init(restored_seq)” in the issue).
  - The only unchecked item is the **optional** legacy doc sweep (mark `ootb_radio_v0.md` / `ootb_node_table_v0.md` as legacy); it was out of scope for #417 and is low urgency — can be a follow-up if desired.
- **Recommended action:** Close now
- **Comment to post:** (see below)

---

## #355

- **Classification:** Fully satisfied
- **Why:**
  - Issue is **planning link only**: no implementation in #355; implementation was to stay in #296. DoD: S03 umbrella and this issue reference #296; planning states seq16 persistence + NVS + test is S03 P0; comment on #296 linking to S03 Planning; no code in #355.
  - #417 was the execution wrapper that implemented the scope of #296 (and referenced #355). Planning linkage (S03 P0, #296 as implementation tracker) was already in place; the work #355 pointed to is now done via #417 and PR #428.
- **Recommended action:** Close now
- **Comment to post:** (see below)

---

## Remaining work timing

- **#296 optional legacy doc sweep:** Not part of #417. **Later** — low urgency; no design or execution started in this audit. If needed, open a small docs-only issue or do in a future sweep.
- **#355:** No remaining work; planning/link-only and that work is complete.

---

## Exact comments to post

### Comment for #296

This historical tracker is now fully satisfied by #417, merged in PR #428.

Completed here:
- Persisted seq16 via narrow NVS adapter (`naviga_storage`: `load_seq16` / `save_seq16`; namespace `"naviga"`, key `"seq16"`).
- Restore behavior: `restored > 0` ⇒ next TX uses `restored + 1`; absent/zero ⇒ fresh start (first seq16 = 1).
- Per-successful-TX persistence (validity separate from value; seq16 == 0 after wraparound is valid and persisted).
- Unit tests: default first packet 1; set_initial 42→43, 100→101; wraparound 65535→0.

The optional legacy doc sweep (mark `ootb_radio_v0.md` / `ootb_node_table_v0.md` as legacy) was out of scope for #417 and remains optional/low urgency for a follow-up if desired.

Closing as completed. Execution path: #296 / #355 → #417 → PR #428.

### Comment for #355

This planning-link issue is fully satisfied: the implementation it pointed to (#296) was completed via #417 and merged in PR #428.

Completed:
- S03 P0 planning linkage and reference to #296 were already in place.
- Persisted seq16 + NVS adapter + tests are implemented: restore on boot, per-TX persistence, correct wraparound handling for seq16 == 0.

Closing as completed. Execution path: #355 → #296 → #417 → PR #428.
