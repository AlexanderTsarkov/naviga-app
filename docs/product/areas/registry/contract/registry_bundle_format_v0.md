# Registry — Bundle format v0 (Contract)

**Status:** Canon (contract).  
**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184)

This contract defines the **v0 registry bundle format**: a single, transport-independent structure for delivering HW profiles, radio presets/params, and optional channel/region plans into the system (embedded, app-packaged, or downloaded). It does **not** define domain semantics, channel selection rules, or OOTB behaviour; those are policy. Canonical ownership and distribution rules are in [distribution_ownership_v0](../policy/distribution_ownership_v0.md).

---

## 1) Purpose and scope

- **Purpose:** One consistent format ("bundle") so that registries can be supplied from **firmware embedded default**, **app packaged**, or **downloaded later** without format drift. Clients (app, firmware) consume the same structure regardless of transport.
- **Layering:** The bundle is an **input** to policy/selection (e.g. [selection_policy_v0](../../../wip/areas/radio/selection_policy_v0.md)); it does **not** define which channel or profile is chosen. OOTB and UI are not normative sources for this contract.
- **Scope (v0):** Bundle schema (metadata, contents list, registries), versioning (bundle format + per-registry content), integrity (checksum/hash; optional signature placeholder), update semantics (replace vs merge; how clients decide "newer"). Out of scope: backend API, mesh/JOIN, power-control; channel discovery flow ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)) is a separate concern — this doc only defines bundle structure and leaves integration points for channel list source.

---

## 2) Non-goals (v0)

- No backend or download implementation; no crypto implementation for signatures (placeholder only).
- No definition of channel selection or discovery flow (#175); only the bundle shape and how registry content is versioned and integrity-checked.
- No dependency on OOTB or UI as product truth.
- No mesh/JOIN or power-control.

---

## 3) Versioning

### 3.1 Bundle format version (schema version)

- The bundle has a **schema version** (e.g. `schemaVersion: "v0"` or integer). Clients that do not support that version **MUST** ignore or reject the bundle (and may prompt for update per [distribution_ownership_v0](../policy/distribution_ownership_v0.md)).
- Backward/forward compatibility: **v0 rule** — add new optional top-level keys or registry sections only; do not remove or rename keys used by v0. New schema versions (v1, v2) may introduce breaking changes; clients that support only v0 ignore unknown schema versions.

### 3.2 Registry content versioning

- Each registry (or each logical registry within the bundle) has its own **content version** and/or **content hash** so clients can decide "newer" and cache correctly.
- Content version may be an integer, semver, or timestamp; exact scheme is implementation-defined but **MUST** be comparable (ordering) and stable. Content hash (e.g. SHA-256 of canonical serialization) is recommended for integrity and cache invalidation; see §5.

---

## 4) Integrity

- **Checksum/hash:** The bundle **SHOULD** include a **content hash** (e.g. over the canonical JSON representation, excluding the hash field itself) so consumers can verify integrity. Algorithm is implementation-defined (e.g. SHA-256 hex).
- **Optional signature placeholder:** A future extension may add a **signature** field (e.g. over the hash). v0 does **not** define crypto or verification; the field may be reserved (e.g. `signature: null` or omitted).
- **Deterministic ordering / stable IDs:** For reproducible hashing and consistent behaviour, registry entries **MUST** use **stable IDs** (e.g. `hw_profile_id`, `radio_profile_id`). Order of entries in arrays may be specified as canonical (e.g. sorted by id) for hash stability; otherwise implementation-defined.

---

## 5) Transport independence

- The same bundle format may be supplied by:
  - **Firmware embedded default** (e.g. compiled into device).
  - **App packaged** (e.g. in app assets/config).
  - **Downloaded later** (e.g. from backend or CDN; v0 does not implement, but format supports it).
- Contract does **not** specify paths, URLs, or packaging; only the in-memory/document structure.

---

## 6) Minimal structure (v0)

The bundle is a single document. Example shape (JSON; YAML or other equivalent is allowed; no binding to a specific runtime):

```json
{
  "schemaVersion": "v0",
  "bundleId": "optional-unique-id-for-this-bundle-instance",
  "createdAt": "optional-ISO8601-or-timestamp",
  "contentHash": "hex-optional-sha256-of-canonical-payload",
  "contents": [
    { "id": "hwProfiles", "version": 1, "hash": "optional-per-registry-hash" },
    { "id": "radioProfiles", "version": 1, "hash": "optional" },
    { "id": "channelPlans", "version": 1, "hash": "optional" }
  ],
  "registries": {
    "hwProfiles": [ ],
    "radioProfiles": [ ],
    "channelPlans": [ ]
  }
}
```

- **metadata:** `schemaVersion` (required), `bundleId` (optional, unique per bundle instance), `createdAt` (optional), `contentHash` (optional, over whole bundle or over `registries` only — define consistently).
- **contents:** List of registry sections with `id`, `version`, and optional `hash` for per-registry versioning and integrity.
- **registries:** Keyed by same `id` as in `contents`. Values are arrays of entries (structure per [registry_hw_capabilities_v0](../../hardware/contract/registry_hw_capabilities_v0.md), [registry_radio_profiles_v0](../../radio/policy/registry_radio_profiles_v0.md), etc.). References (IDs used elsewhere, e.g. in NodeTable or selection policy) are the stable ids inside these arrays.

---

## 7) Update semantics

- **Replace vs merge:** v0 default is **replace**: applying a new bundle (or a new version of a registry section) **replaces** the previous content for that section. **Merge** (e.g. merge by id, with conflict rules) may be defined per consumer or in a future revision; not required for v0.
- **How clients decide "newer":** Compare **content version** or **content hash** of the incoming bundle (or section) with the one currently held. Higher version or different hash ⇒ newer. Exact policy (e.g. "always accept higher version", "accept only if hash differs") is implementation-defined; this contract only requires that version and/or hash are present so that such a decision can be made.
- **Caching expectations:** Clients may cache the bundle (or per-registry sections). Cache invalidation is by version/hash; when a new bundle is supplied (e.g. via app update or download), replace cache with the new content. TTL or refresh policy is implementation-defined; v0 does not mandate.

---

## 8) Pointers to future work

- **Signatures:** Placeholder for signed bundles; no crypto or verification in v0.
- **Backend distribution:** [distribution_ownership_v0](../policy/distribution_ownership_v0.md) §8; bundle format is transport-agnostic so backend can deliver the same structure.
- **Channel list source / discovery (#175):** Channel plans may be part of the bundle; **which** channel list is used and how discovery works is out of scope here. This contract only defines that `channelPlans` (or equivalent) can appear in the bundle; [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) defines inputs and flow.

---

## 9) Related

- **Distribution & ownership:** [distribution_ownership_v0](../policy/distribution_ownership_v0.md) — who owns content, how it is shipped (bundled in v0), schema rev rules, unknown profile handling.
- **HW registry:** [registry_hw_capabilities_v0](../../hardware/contract/registry_hw_capabilities_v0.md) — semantics of hwProfiles entries.
- **Radio registry:** [registry_radio_profiles_v0](../../radio/policy/registry_radio_profiles_v0.md) — semantics of radioProfiles / channelPlans.
- **Selection policy:** [selection_policy_v0](../../../wip/areas/radio/selection_policy_v0.md) — consumes registries; does not define bundle format.
- **Issue:** [#184 Registry bundle format v0](https://github.com/AlexanderTsarkov/naviga-app/issues/184)
