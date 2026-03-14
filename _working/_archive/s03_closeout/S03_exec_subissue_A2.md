## Goal

Define NodeTable persistence snapshot format and restore policy: what is persisted vs derived vs prohibited; how snapshot is written (batched/flush) and restored on boot; invariants after restore.

## Definition of done

- Snapshot format documented (in canon or ADR): which fields are persisted, which derived on boot, which prohibited from persistence.
- Restore policy: when and how NodeTable (or minimal subset) is restored; consistency with seq16 and presence/age semantics.
- Implementation: save/restore path implemented and wired; no unbounded write frequency (flash wear).
- Unit tests for restore and invariants (e.g. last_seen_age, is_stale consistent with restored data).

## Touched paths

- `firmware/src/app/node_table.cpp`, `firmware/src/domain/node_table.h` (or equivalent); NVS adapter; possibly `docs/product/areas/nodetable/` for policy.
- Canon: identity_naming_persistence_eviction_v0, presence_and_age_semantics_v0.

## Tests

- Unit: restore from snapshot yields valid NodeTable state; invariants (e.g. ref_core_seq16, last_seen) hold.
- Unit: prohibited/derived fields are not persisted or are cleared on restore as specified.

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: A — Storage & persistence foundation (P0).
