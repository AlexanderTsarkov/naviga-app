# Hardware — HW Capabilities registry (Registry v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)

This doc defines the **HW Capabilities registry** v0: facts and possibilities (adapter types, capability flags, confidence), local vs remote disclosure, and versioning. No firmware or app implementation; no semantics defined via OOTB screens — OOTB may be cited only as an example/indicator.

---

## 1) Purpose / phase

- **Purpose:** Single catalog of known hardware types and their capabilities so that NodeTable, UI, and policy can interpret **hwProfile** / **hwType** (e.g. from [link-telemetry-minset](../nodetable/contract/link-telemetry-minset-v0.md)) and show correct options (radio profiles, features, limits).
- **Phase:** Post–OOTB spec work; registry content and schema are WIP. Implementation ownership (firmware vs mobile vs backend) is out of scope for this v0; doc defines the **data model and rules** only.

---

## 2) Definitions

| Term | Meaning |
|------|--------|
| **hw_profile_id / hw_type** | Opaque identifier for a hardware model/variant (e.g. board + radio adapter combo). Used in telemetry/claim and looked up in this registry. |
| **adapter_type** | How the radio is connected: e.g. `UART_MODEM`, `SPI_CHIP`. Drives which drivers and which capabilities (e.g. sense/CAD) apply. |
| **capability** | A boolean or enumerated fact about the hardware (e.g. “supports RSSI”, “supports baro”, “powerbank reserve”). |
| **confidence** | How reliable the capability claim is: `low` \| `med` \| `high`. Enables UI to show “best effort” or “supported” appropriately. |

---

## 3) Local vs remote disclosure

- **Local (BT-connected):** Full capabilities can be discovered (query device or read from firmware). App can show full feature set and compatibility (e.g. which RadioProfiles are valid).
- **Remote (radio only):** Over the air we disclose at most a **short hw id** (or hw_profile_id) in beacons/claims. Remote peers resolve via registry to a **subset** of capabilities (e.g. “likely has RSSI”) and optional confidence. No requirement to broadcast full capability bitmap on the air; privacy and payload size favor short id + registry lookup.

---

## 4) Field groups (v0)

Registry entries are grouped conceptually as follows. Schema format (JSON, protobuf, or table) is implementation-defined; this section defines **what** is described.

| Group | Contents | Notes |
|-------|----------|--------|
| **Radio adapter** | **adapter_type** (e.g. UART_MODEM, SPI_CHIP). | Drives driver and default behavior (e.g. UART track: sense OFF, jitter-only as default). |
| **Metrics support + confidence** | RSSI, SNR, utilization (if applicable); each with optional **confidence** (low/med/high). | Enables “link metrics available” and “channel utilization” policies. |
| **Sensors** | e.g. baro (barometric altitude). | Optional; for nodes that report altitude or pressure. |
| **Power features** | e.g. powerbank reserve, charging state. | Optional; for UI and power policy. |
| **Other** | Extensible; new capability keys can be added with schema rev. | Unknown keys ignored by older clients per versioning rules below. |

---

## 5) Versioning rules

- **Schema revision:** Registry has a **schema rev** (e.g. v1). New capabilities or new adapter types may require a rev bump.
- **Forward / backward compat:** New fields in an entry are optional; old clients ignore unknown fields. Old entries remain valid unless explicitly deprecated.
- **Unknown hw id behavior:** If the app or device encounters an **hw_profile_id** not present in its local registry (e.g. newer device, or registry not yet updated), behavior is **“prompt for update”** (or equivalent): do not assume capabilities; prompt user to update app/registry so the new id can be resolved. No silent failure or wrong feature visibility.

---

## 6) Non-goals (v0)

- No implementation of CAD/LBT in this doc; **sense OFF + jitter-only** remains the default assumption for the UART track.
- No definition of semantics via OOTB screens; OOTB can be cited only as an example of which capabilities might be exposed.
- Ownership and distribution of the registry (firmware vs mobile vs backend) are deferred to a follow-up.

---

## 7) Open questions / follow-ups

- **Selection policy:** Separate doc/issue for how profile/channel choices are made (autopower, throttling, defaults).
- **Autopower algorithm:** How to adapt tx power or profile from capabilities and environment; out of scope here.
- **Identity / pairing flow:** How local vs remote disclosure is used in pairing or “owned vs seen” flows; out of scope here.
- **Registry ownership and format:** Where the table lives (bundle in app, backend, firmware) and exact schema (e.g. JSON table) for implementation.

---

## 8) Related

- **NodeTable contract:** [nodetable/index.md](../nodetable/index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- **Link/Telemetry minset:** [nodetable/contract/link-telemetry-minset-v0.md](../nodetable/contract/link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158) (hwProfile field).
- **Radio profiles registry:** [../radio/registry_radio_profiles_v0.md](../radio/registry_radio_profiles_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159).
