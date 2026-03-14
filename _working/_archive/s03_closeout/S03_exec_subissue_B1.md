## Goal

Ensure NodeTable fields match the canon field map and master table: add missing fields, remove or replace stubs, align types and semantics with [nodetable_field_map_v0](docs/product/areas/nodetable/policy/nodetable_field_map_v0.md) and master table (fields_v0_1.csv / packets_v0_1.csv).

## Definition of done

- Every canon field in the S03 slice has a corresponding NodeTable field (or explicit "not implemented" with reason).
- Stub or placeholder fields removed or replaced with real types.
- In-memory layout and naming aligned to canon (Core_Pos, Core_Tail, Operational, Informative, Self_exposed, Derived as applicable).

## Touched paths

- `firmware/src/app/node_table.cpp`, `firmware/src/app/node_table.h`; `firmware/src/domain/node_table.h`; domain structs for NodeEntry.
- Canon: nodetable_field_map_v0, s03_nodetable_slice_v0, nodetable_fields_inventory_v0.

## Tests

- Unit: struct/field existence and types match canon list (e.g. golden or generated checks).
- No regression in existing NodeTable tests.

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: B — NodeTable completeness & wiring (P0).
