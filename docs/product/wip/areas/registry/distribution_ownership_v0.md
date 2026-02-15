# Registry — Distribution & ownership v0 (Policy WIP)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue Ref:** [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184)

This doc defines **registry distribution & ownership v0**: where registry “truth” lives in the repo, who owns it, how registries are shipped to the mobile app (bundled in v0), schema rev and compatibility rules, behavior for unknown **hw_profile_id** / higher **registry_schema_rev**, and a future path to backend-delivered registry updates (concept only). No implementation of generator, pipeline, or backend; no changes to existing registry semantics ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)); no code changes. OOTB is example only, not truth.

---

## 1) Purpose / scope

- **Purpose:** Remove ambiguity about **where** registry content lives, **who** maintains it, **how** it reaches the app (v0: bundled), and **what** happens when the app or device sees an unknown profile or a newer schema. A future “backend mode” is documented as a concept only.
- **Scope (v0):** Source-of-truth format and ownership; schema rev rules; mobile bundling; firmware vs app precedence; unknown-profile UX; future backend (stub). Out of scope: implementation of build pipeline, backend API, or code.

---

## 2) Decision summary (v0)

| # | Decision point | v0 choice | Open / follow-up |
|---|----------------|-----------|-------------------|
| 1 | Source of truth format | **Human-editable first:** canonical content lives in **Markdown** under `docs/product/wip/areas/` (e.g. [HW](../hardware/registry_hw_capabilities_v0.md), [Radio](../radio/registry_radio_profiles_v0.md)). Machine-consumable artifacts (JSON/YAML) may be generated later from the same source or maintained in parallel; v0 does not mandate a single canonical machine format. | Exact pipeline (MD→JSON, or hand-maintained JSON) is implementation; document when stable. |
| 2 | Ownership map | **Canonical spec in repo:** owned by Product/Architecture (single merge owner). **Input owners:** Firmware/HW confirm hardware facts and hw_profile_id→capabilities mapping; Mobile is a consumer and provides UX constraints/format needs. **v1 note:** Later (v1), registry sources should be tool-generated (not hand-edited), with a human-readable render committed alongside. | Backend or firmware may later hold a *copy* or *cache*; ownership of the canonical content stays in repo/docs. |
| 3 | Schema rev rules | **Append-only** capability/field keys; do not rename; deprecations keep old keys until a **schema rev window** (e.g. one major app version). **Who bumps rev:** owner (product/docs) when adding or deprecating in a way that requires client awareness. **“Update app” trigger:** when app or device sees **unknown hw_profile_id** or **registry_schema_rev** &gt; app’s supported rev. | Exact rev numbering (v1, v2 vs semver) and window length are implementation. |
| 4 | Mobile bundling | **Bundled with app:** registry assets (e.g. HW capabilities table, RadioProfiles + ChannelPlan) live in the **app repo** (e.g. under an assets or config path). They are **versioned with the app release**; no separate registry version in v0 beyond schema_rev inside the payload. App ships with a known schema_rev; store/play store update delivers new registries. | Exact path and format (single JSON vs per-registry files) are implementation. |
| 5 | Firmware vs mobile precedence | **v0 merge/fallback:** (a) If BT provides explicit capability values → trust BT as local truth. (b) If BT does not provide capability details → resolve via bundled registry by hw_profile_id. (c) If BT conflicts with bundled registry → prefer BT; optionally surface "inconsistent with registry" for diagnostics (no behavior change). App may **cache** for offline/display; cache is not source of truth. | Caching TTL and refresh rules are implementation. |
| 6 | Unknown profile handling UX | **Messaging:** prompt user to “update the app” (or equivalent) when **hw_profile_id** is missing from app’s registry or when **registry_schema_rev** from device/remote is higher than the app supports. **Fallback:** do not assume capabilities; do not show wrong feature flags or profile options. Optional: show raw hw_profile_id for support. | Copy and minimum app version checks are product/UX. |
| 7 | Future backend mode | **Concept only:** a future phase may deliver registry updates via a **backend** (e.g. signed packs, CDN). App would **cache** them; **minimum app version** may be required to interpret new schema. Not implemented or specified in v0. | Full design (signing, caching, min version) is a follow-up. |

---

## 3) Where registry truth lives (repo)

- **Canonical content** for v0 lives in the **repo** under `docs/product/wip/areas/`:
  - HW Capabilities: [hardware/registry_hw_capabilities_v0.md](../hardware/registry_hw_capabilities_v0.md)
  - RadioProfiles & ChannelPlan: [radio/registry_radio_profiles_v0.md](../radio/registry_radio_profiles_v0.md)
- Content is **human-editable** (Markdown tables and lists). Machine-consumable exports (e.g. JSON) may be generated or maintained alongside; v0 does not require a single canonical machine format.
- **Ownership:** Canonical spec in repo is owned by Product/Architecture (single merge owner). **Input owners:** Firmware/HW confirm hardware facts and hw_profile_id→capabilities mapping; Mobile is a consumer and provides UX constraints/format needs. Later (v1), registry sources should be tool-generated (not hand-edited), with a human-readable render committed alongside. Changes via PR, aligned with [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).

---

## 4) How registries are shipped to the mobile app (v0)

- **Bundled:** The mobile app **bundles** registry data (HW capabilities, RadioProfiles, ChannelPlan) as part of the app package. No separate download or backend fetch in v0.
- **Versioning:** Bundled assets are **versioned with the app release**. The app has a **registry_schema_rev** (or equivalent) it supports; when the app is updated from the store, it ships with the latest bundled registries and rev.
- **Where in app repo:** Exact path (e.g. `assets/registries/`, `config/`) is implementation-defined; this doc only states that v0 = bundled with app, no backend.

---

## 5) Schema rev and compatibility rules

- **Schema rev:** Each registry (or combined bundle) has a **registry_schema_rev**. Bump when:
  - New **hw_profile_id** or profile/channel entries are added that older clients must not misinterpret, or
  - Keys are deprecated (old keys kept for a deprecation window; after window, rev bump so clients know to ignore removed keys).
- **Append-only keys:** Do not rename capability or profile keys; add new keys only. Unknown keys are **ignored** by older clients.
- **“Update app” trigger:** If the app sees an **hw_profile_id** it does not have in its bundled registry, or a **registry_schema_rev** (from device over BT or from remote) **greater than** the app’s supported rev, the app should **prompt for update** and not assume capabilities.

---

## 6) Unknown hw_profile_id / higher registry_schema_rev (behavior)

- **Unknown hw_profile_id:** Device or remote sends an hw_profile_id that is not in the app’s bundled registry. **Behavior:** Prompt user to update the app; do not guess capabilities; optional: show raw id for support.
- **Higher registry_schema_rev:** Device (over BT) or remote payload reports a registry_schema_rev higher than the app supports. **Behavior:** Same as above — prompt for update; do not use device/remote capability payload to override or extend in an ad-hoc way. Updated app brings new bundled registry and supported rev.

---

## 7) Firmware vs mobile precedence (BT full capabilities)

- **v0 merge/fallback rules:**
  - **(a)** If BT provides explicit capability values → trust BT as local truth.
  - **(b)** If BT does not provide capability details → resolve via bundled registry by hw_profile_id.
  - **(c)** If BT conflicts with bundled registry → prefer BT; optionally surface "inconsistent with registry" for diagnostics (no behavior change).
- The app does **not** override device-reported capabilities with app-only data. The app may **cache** BT-reported capabilities and registry lookups for performance or offline display; cache is **not** the source of truth. Refresh or invalidate on connect or on app update.

---

## 8) Future backend-delivered registry (concept only)

- A **future** phase may introduce **backend-delivered** registry updates (e.g. signed packs, CDN). Concepts only:
  - **Signed packs:** Registry updates could be signed so the app can verify integrity and origin.
  - **Caching:** App caches downloaded registry; TTL or version check determines when to re-fetch.
  - **Minimum app version:** New schema may require a minimum app version to interpret; backend could advertise minimum version.
- v0 does **not** implement or specify this; no backend, no pipeline. Documented here so the v0 bundled approach is clearly the baseline and the future path is noted.

---

## 9) Non-goals (v0)

- No implementation of generator, pipeline, or backend.
- No changes to the **semantics** of existing registries ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)); only distribution and ownership rules.
- No code changes. OOTB is example only.

---

## 10) Open questions / follow-ups

- **Exact bundle path and format** in app repo (single JSON vs per-registry files).
- **Schema rev numbering** (v1, v2 vs semver) and **deprecation window** length.
- **Backend mode:** full design (signing, caching, min app version) when needed.
- **Firmware copy:** whether firmware ever bundles a subset of registries (e.g. for headless) and how it stays in sync; out of scope for v0.

---

## 11) Related

- **HW Capabilities registry:** [../hardware/registry_hw_capabilities_v0.md](../hardware/registry_hw_capabilities_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)
- **RadioProfiles & ChannelPlan registry:** [../radio/registry_radio_profiles_v0.md](../radio/registry_radio_profiles_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)
- **NodeTable contract:** [../nodetable/index.md](../nodetable/index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
