**Done (docs-only).** PR [#409](https://github.com/AlexanderTsarkov/naviga-app/pull/409).

**Doc paths:**
- **WIP semantics:** `docs/product/wip/areas/firmware/policy/fw_version_id_semantics_s03.md`
- **Registry:** `docs/product/areas/firmware/registry/fw_version_id_registry_v0.md`
- **Related:** `docs/product/areas/firmware/policy/provisioning_interface_v0.md` §8 — link to semantics + registry

**DoD mapping:**
- **Clear definition (what, scope):** §1 Purpose and scope; §3 Meaning (opaque firmware build/release identifier).
- **Encoding rules and examples:** §2 Data type and encoding (uint16 LE, sentinel 0xFFFF); §8 Examples (0x0042, 0xFFFF).
- **Sentinel/unknown rules:** §2 + registry §1 (0xFFFF reserved; 0x0000 optional “unknown/unstamped”).
- **Update rules and lifecycle:** §5 (assigned at build/release; monotonic or curated; NodeTable update on RX).
- **Links to provisioning baseline and NodeTable master table:** §9 Related (provisioning_baseline_s03, fields_v0_1.csv).
- **Future S04 BLE exposure:** §9 “Future: S04 BLE exposure of fwVersionId … out of scope for S03.”

**Normative:** §7 — MUST NOT change decode rules based on fwVersionId; only payloadVersion governs. Loss-tolerance invariant preserved (Informative optional).
