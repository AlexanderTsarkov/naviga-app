## Summary

Define **fwVersionId** semantics (what/why/how) and a minimal registry for S03. Docs-only; no code or protocol layout changes.

Closes #405.

## Scope

- **WIP:** `docs/product/wip/areas/firmware/policy/fw_version_id_semantics_s03.md` — purpose (informational/diagnostic only), data type (uint16 LE), sentinel (0xFFFF), meaning (opaque build/release id), why not SemVer-on-wire, update rules/lifecycle, where used (on-air Informative, NodeTable persisted, S04 UI via registry), **normative: MUST NOT change decode rules based on fwVersionId**, examples. Cross-links to info_packet_encoding_v0 (sentinel + loss tolerance), provisioning_baseline_s03, fields_v0_1.csv, packet_sets_v0_1 (u16 unchanged), registry.
- **Registry:** `docs/product/areas/firmware/registry/fw_version_id_registry_v0.md` — allocation rules, 0xFFFF reserved sentinel, optional 0x0000 unknown, table (hex → human label), note for mobile embed.
- **Related:** Added fwVersionId semantics + registry to `provisioning_interface_v0.md` §8 Related.

## DoD (mapped)

- Clear definition of fwVersionId (what, scope) → §1, §3
- Encoding rules and examples → §2, §8
- Sentinel/unknown rules → §2, registry §1 (0xFFFF, 0x0000)
- Update rules and lifecycle → §5
- Links to provisioning baseline and NodeTable master table → §9
- Note: "Future: S04 BLE exposure" → §9

## Quality

- No mention of fwVersionId changing decoding; §4 and §7 state the opposite (only payloadVersion governs).
- Canon preserved: uint16 LE, sentinel 0xFFFF; loss-tolerance invariant referenced.
- Registry: simple table + rules; 0x0000 optional reserved.
