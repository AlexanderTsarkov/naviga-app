# PR-1 Docs integrity report (#278)

**Branch:** `docs/278-pr1-integrity`  
**Scope:** Fix all Stale canon→WIP links per [spec_map_v0.md §4](../../docs/product/wip/spec_map_v0.md); fix any other broken links in `docs/product/areas/**` and `docs/product/wip/**`. Link fixes only (no moves, no banners).

---

## Stale items from spec_map_v0.md §4 — resolution

| # | Canon source | WIP target (Stale) | Action | Result |
|---|--------------|--------------------|--------|--------|
| 1 | `areas/hardware/contract/registry_hw_capabilities_v0.md` | `wip/areas/registry/distribution_ownership_v0.md` | **Verified** | Already linked to canon: `../../registry/policy/distribution_ownership_v0.md`. No change. |
| 2 | `areas/firmware/policy/provisioning_interface_v0.md` | `wip/areas/domain/policy/role_profiles_policy_v0.md` | **Verified** | Already linked to canon: `../../domain/policy/role_profiles_policy_v0.md`. No change. |
| 2b | `areas/firmware/policy/provisioning_interface_v0.md` | `wip/areas/radio/policy/radio_profiles_policy_v0.md` | **Verified** | Already linked to canon: `../../radio/policy/radio_profiles_policy_v0.md`. No change. |
| 3 | `areas/firmware/policy/module_boot_config_v0.md` | `wip/areas/radio/policy/radio_profiles_policy_v0.md` | **Verified** | Already linked to canon: `../../radio/policy/radio_profiles_policy_v0.md`. No change. |
| 4 | `areas/firmware/policy/boot_pipeline_v0.md` | `wip/areas/radio/policy/radio_profiles_policy_v0.md` | **Verified** | Already linked to canon: `../../radio/policy/radio_profiles_policy_v0.md`. No change. |
| 5 | `areas/nodetable/index.md` | `wip/areas/registry/distribution_ownership_v0.md` | **Fixed** | Link was `../../registry/policy/distribution_ownership_v0.md` (resolved to `docs/product/registry/` — wrong). Updated to `../registry/policy/distribution_ownership_v0.md` (resolves to `docs/product/areas/registry/policy/`). |

---

## Other link / integrity fixes

| File | Change |
|------|--------|
| `docs/product/areas/registry/policy/distribution_ownership_v0.md` | In-repo path description updated: `docs/product/wip/areas/` → `docs/product/areas/` in table row (Source of truth format) and in §3 (Where registry truth lives). Ensures doc does not direct readers to the old WIP path. |

---

## Scan summary

- **Canon (`docs/product/areas/**`):** Grep for `distribution_ownership`, `role_profiles_policy`, `radio_profiles_policy` confirmed all other canon docs already use correct relative paths to canon targets. No other broken links found in targeted scan.
- **WIP (`docs/product/wip/**`):** spec_map and redirect stubs use `../areas/` or `../../areas/` correctly; no changes made in WIP in this PR (Stale fix was in canon only).
- **Central docs spot-check:** NodeTable hub (`areas/nodetable/index.md`) and spec_map §4 table both reference registry/distribution_ownership; the fix in nodetable index ensures the “Registry distribution” link from the hub now resolves to the canon file.

---

## Files changed (diff summary)

1. `docs/product/areas/nodetable/index.md` — one link: `../../registry/` → `../registry/` (Registry distribution).
2. `docs/product/areas/registry/policy/distribution_ownership_v0.md` — two in-repo path strings: `wip/areas/` → `areas/`.
3. `_working/docs_integrity/278_pr1_report.md` — this report (new file).

No file moves or renames. No legacy/OOTB banners. No _working archive or index changes.
