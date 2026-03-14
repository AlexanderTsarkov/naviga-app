**Done (docs-only).** PR [#410](https://github.com/AlexanderTsarkov/naviga-app/pull/410).

**Doc paths:**
- **WIP semantics:** `docs/product/wip/areas/firmware/policy/hw_profile_id_semantics_s03.md`
- **Registry:** `docs/product/areas/firmware/registry/hw_profile_id_registry_v0.md`
- **Related:** `docs/product/areas/firmware/policy/provisioning_interface_v0.md` §8 — link to semantics + registry

**DoD mapping:**
- **Clear semantics of hwProfileId + examples:** §1 Purpose and scope; §3 Meaning (hardware config profile identifier); §9 Examples and sentinel (0x0001, 0xFFFF, optional 0x0000).
- **Mapping/registry strategy documented:** §7 Minimal schema (implied attributes); §10 Related (registry); [hw_profile_id_registry_v0.md](https://github.com/AlexanderTsarkov/naviga-app/blob/main/docs/product/areas/firmware/registry/hw_profile_id_registry_v0.md) — allocation rules, reserved/sentinel, table hwProfileId → label → implied attributes.
- **Links to provisioning baseline and packet contract:** §10 Related — [info_packet_encoding_v0](https://github.com/AlexanderTsarkov/naviga-app/blob/main/docs/product/areas/nodetable/contract/info_packet_encoding_v0.md) (sentinel + loss tolerance), [provisioning_baseline_s03](https://github.com/AlexanderTsarkov/naviga-app/blob/main/docs/product/wip/areas/firmware/policy/provisioning_baseline_s03.md), e220_radio_profile_mapping_s03, tx_power_contract_s03, boot_pipeline_v0.

**Normative:** §5 — MUST NOT change decode rules based on hwProfileId; MUST NOT be required for correctness. Loss-tolerance invariant preserved.
