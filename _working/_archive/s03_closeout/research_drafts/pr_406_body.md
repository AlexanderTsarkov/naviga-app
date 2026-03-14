## Summary

Define **hwProfileId** semantics (what/why/how) and a minimal registry for S03. Docs-only; no code or protocol layout changes.

Closes #406.

## Scope

- **WIP:** `docs/product/wip/areas/firmware/policy/hw_profile_id_semantics_s03.md` — purpose (informational/diagnostic/config identity), data type (uint16 LE), sentinel (0xFFFF), meaning (hardware config class: board + radio SKU + GNSS), why (diagnostics, OOTB baselines e.g. TX power by SKU, mobile display), MUST NOT affect decoding / MUST NOT be required for correctness, lifecycle (per hardware variant, stable across FW builds), minimal schema (implied attributes in registry), where used, examples. Cross-links to info_packet_encoding_v0, provisioning_baseline_s03, e220_radio_profile_mapping_s03, tx_power_contract_s03, boot_pipeline_v0, fields_v0_1.csv, packet_sets, registry.
- **Registry:** `docs/product/areas/firmware/registry/hw_profile_id_registry_v0.md` — allocation rules, 0xFFFF sentinel, optional 0x0000 unknown, table (hex → human label → implied attributes: radio_sku, gnss class, notes).
- **Related:** Added hwProfileId semantics + registry to `provisioning_interface_v0.md` §8.

## DoD (mapped)

- Clear semantics of hwProfileId + examples → §1, §3, §9
- Mapping/registry strategy documented → §7, §10, registry doc
- Links to provisioning baseline and packet contract → §10 (info_packet_encoding_v0, provisioning_baseline_s03)
