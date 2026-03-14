## Goal

Ensure RX semantics and apply rules match canon: presence/age semantics (last_seen_ms, last_seen_age_s, is_stale, pos_age_s); core/tail ref rules (ref_core_seq16, apply Tail only when ref matches lastCoreSeq); accepted/duplicate/ooo handling per [rx_semantics_v0](docs/product/areas/nodetable/policy/rx_semantics_v0.md) and [packet_to_nodetable_mapping_v0](docs/product/areas/nodetable/policy/packet_to_nodetable_mapping_v0.md).

## Definition of done

- On RX: apply logic for each packet type matches canon (which fields updated, when Tail is applied, when packet is accepted/duplicate/ooo).
- Presence and age: last_seen_ms updated on accepted packets; is_stale/pos_age_s derived per canon.
- Coalesce/replace rules per canon (no overwriting newer with older; ref_core_seq16 gate for Tail).

## Touched paths

- `firmware/src/domain/beacon_logic.cpp` (or RX apply path); NodeTable apply/update functions; domain node_table.
- Canon: rx_semantics_v0, packet_to_nodetable_mapping_v0, presence_and_age_semantics_v0.

## Tests

- Unit: apply rules — e.g. Tail applied only when ref_core_seq16 == lastCoreSeq; duplicate/ooo do not overwrite position.
- Unit: presence/age — last_seen_ms and derived age/stale match canon rules.

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: B — NodeTable completeness & wiring (P0).
