# Firmware — fwVersionId registry v0

**Status:** Canon (registry). **Issue:** [#405](https://github.com/AlexanderTsarkov/naviga-app/issues/405) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This document defines the **fwVersionId** registry: allocation discipline, reserved ranges, and a table mapping uint16 values to human-readable labels (e.g. SemVer + tag). It is the single source for “what does this id mean” and can be embedded in the mobile app for offline display.

---

## 1) Allocation rules

- **Discipline:** Values are allocated at **build/release time**. Either **monotonic** (increment per build) or **curated** (assigned from this registry); avoid ad hoc collisions.
- **Reserved:** **0xFFFF** = sentinel “not present” (canon: [info_packet_encoding_v0.md §4.3](../../nodetable/contract/info_packet_encoding_v0.md#43-sentinel-handling)). Do not allocate.
- **Optional reserved:** **0x0000** = “unknown / unstamped” (e.g. legacy or unversioned build). If used, receivers treat as “version unknown”; do not use for a real release id if 0x0000 is reserved.
- **Range:** Allocated ids should stay within 0x0001–0xFFFE unless 0x0000 is used for unknown.

---

## 2) Registry table

| fwVersionId (hex) | Human label (SemVer + tag) | Notes |
|-------------------|----------------------------|--------|
| 0x0000 | (reserved: unknown/unstamped) | Optional; do not allocate for releases. |
| 0x0042 | S03 OOTB v0.1.7 (example) | Example entry; replace with real entry when first S03 build is stamped. |
| … | … | Add rows as releases are stamped. |
| 0xFFFF | (reserved: not present) | Sentinel; never allocated. |

*Mobile app may embed this table (or a generated mapping) to display labels offline.*

---

## 3) Related

- **Semantics (what/why/how):** [fw_version_id_semantics_v0.md](../policy/fw_version_id_semantics_v0.md) ([#405](https://github.com/AlexanderTsarkov/naviga-app/issues/405)) — canon policy; encoding, lifecycle, “must not affect decoding.”
- **Canon contract:** [info_packet_encoding_v0.md](../../nodetable/contract/info_packet_encoding_v0.md) — §3.2 fwVersionId uint16 LE, §4.3 sentinel 0xFFFF.
