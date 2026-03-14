## Why this is needed now

- **Radio profile baseline** was aligned in #424 (FACTORY/current canon, Phase A/B, pointers, persistence).
- **User profile baseline** remains an uncovered gap: there is no equivalent execution issue yet for aligning default/current/previous user profile semantics and ensuring the active-values plane drives runtime/TX behavior.

S03 / Product Truth requires User Profile semantics with:
- `role_id`
- `min_interval_s`, `min_distance_m`, `max_silence_10s`
- `default_user_profile`, `current_user_profile`, `previous_user_profile`

The **active-values plane** is mandatory: runtime / TX behavior must use values from the **active user profile**. This issue should mirror the level of rigor used for #424, but for User Profile.

---

## In scope

- Define/align **default_user_profile**, **current_user_profile**, **previous_user_profile** (pointer semantics and NVS/persistence).
- Fallback to **default profile id 0** when missing or invalid; persist normalized pointers/state.
- Ensure **active user profile** values drive:
  - `role_id`
  - `min_interval_s`
  - `min_distance_m`
  - `max_silence_10s`
- Phase B–style **logical resolution only** (load, normalize, persist); no BLE/UI profile management in this issue.

---

## Out of scope

- BLE/UI profile editing flows.
- Broad observability work from #425.
- Docs-only current_state work from #426.
- Reopening #424 radio profile work.

---

## Definition of done

- [ ] User profile pointers and default behavior aligned with canon.
- [ ] Active user profile resolved correctly at boot (missing/invalid → default, persist).
- [ ] Runtime/TX uses active user-profile values (active-values plane).
- [ ] Tests added/updated for pointer resolution and fallback.
- [ ] Devkit build passes.
- [ ] Docs/inventory updated if implementation truth requires it.

---

## Links / context

- **Umbrella:** #416
- **Radio profile baseline (done):** #424
- Product truth / active-values plane docs (as applicable)
- S03 provisioning baseline, boot pipeline (Phase B logical state)
