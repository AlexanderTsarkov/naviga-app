# Radio — Radio profiles policy v0 (defaults, persistence, factory reset)

**Status:** Canon (policy).  
**Work Area:** Product Specs WIP · **Parent:** [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines the **v0/V1-A baseline** for radio profile model, persistence semantics, first-boot behavior, and factory reset. It makes **OOTB defaults deterministic** without depending on UI or product copy as normative source. Related: [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) (channel discovery, post V1-A), [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184) (registry bundle format).

---

## 1) Purpose & scope

- **Purpose:** Formally define the **RadioProfile** model (parameters), **profile persistence** (default vs user, current/previous pointers), **first boot** rule, **factory reset** semantics, and a minimal **conceptual API** (list / select / reset). This is the policy that guarantees a defined boot config and deterministic OOTB behavior.
- **Scope (v0):** Default profile (immutable); user profiles (add/remove); persisted pointers (CurrentProfileId, optional PreviousProfileId); first boot and factory reset rules; no storage or wire format — only semantics.
- **Explicit note:** This doc is the **V0/V1-A baseline** that makes OOTB deterministic. Implementation and UI consume this policy; OOTB/UI are not the source of truth for these rules.

---

## 2) Non-goals (v0)

- No channel discovery or channel-list source ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)) in this policy; that is future/post V1-A.
- No CAD/LBT; no mesh/JOIN semantics.
- No backend distribution of profiles (ties to [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184)); OOTB defaults exist regardless of bundle delivery.
- No implementation detail (storage layout, wire format); only normative semantics.

---

## 3) Definitions

### 3.1 RadioProfile (model)

A **RadioProfile** is a named set of parameters used for radio operation:

| Parameter | v0 meaning | Future (out of scope here) |
|-----------|------------|----------------------------|
| **channel** | Logical channel identifier (source and enumeration may come from registry or fixed set; see [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) for discovery). | Same. |
| **modulation preset** | Today: **UART preset** (e.g. E220-style preset index). | Future: explicit SF/BW/CR when SPI or equivalent exposes them. |
| **txPower** | Transmit power (step or dBm as defined by HW/registry). | Same. |

The policy does not define the exact encoding of channel or preset; it requires that a profile identifies them and that a **default** profile exists with well-defined values.

### 3.2 Profile kinds and pointers

| Term | Meaning |
|------|--------|
| **Default profile** | Single, **immutable** profile that **MUST** always exist. **Embedded in firmware** (or product/registry); not user-editable. On first boot (or when no valid current exists), the device **stores the default as an immutable record** in the persisted store so it can be referenced by CurrentProfileId; the default itself is never removed. Used when no user choice exists and after factory reset. |
| **User profiles** | Zero or more profiles created or modified by the user (add/remove). Mutable. |
| **CurrentProfileId** | Persisted **pointer** (id or handle) to the profile used for radio operation — **not** the profile content itself. **Used on boot** to restore last chosen profile. |
| **PreviousProfileId** | Optional persisted **pointer** to the previously active profile; used for **rollback UX** (e.g. "revert to previous"). May be empty. |

---

## 4) Persistence semantics (pointers and expectations)

- **Default profile** is always present: embedded in firmware, and (on first boot or when needed) stored as an **immutable record** in the persisted store so that CurrentProfileId can point to it. It is not "user data" and is not removed by factory reset.
- **User profiles** are stored in implementation-defined storage; policy only requires that they can be **added** and **removed** (or marked inactive).
- **CurrentProfileId** and **PreviousProfileId** are **pointers** (ids/handles) only; they reference a profile record, they do not hold profile content. CurrentProfileId MUST be persisted and MUST be used on boot to select the active profile. If it references a user profile that no longer exists (e.g. after partial reset), behavior is implementation-defined fallback (e.g. treat as "no current" and apply first-boot rule).
- **PreviousProfileId** is optional; if present it MAY be used for rollback. No requirement to persist it across reboot; policy allows "clear previous" on factory reset.

Storage format and location are out of scope; this section defines only **semantic** expectations.

---

## 5) First boot behavior

**Rule:** If there is **no valid persisted CurrentProfileId** (first boot, or storage cleared), the device MUST:

1. **Apply** the **default** profile (channel + modulation preset + txPower from the default profile).
2. **Persist** CurrentProfileId so that it points to the default profile.
3. Optionally clear or leave PreviousProfileId unspecified.

After this, boot config is defined and OOTB behavior is deterministic for "default profile only" without user interaction.

---

## 6) Factory reset behavior

**Rule:** Factory reset MUST:

1. **Remove** all user profiles, or **mark** them inactive (implementation choice); the default profile MUST remain.
2. Set **CurrentProfileId** to the default profile and persist it.
3. **Clear** PreviousProfileId (or leave it empty).

After factory reset, the device behaves as if "current = default" and no user profiles exist; first-boot rule is not re-triggered on next boot because CurrentProfileId is already set to default.

---

## 7) Minimal API surface (conceptual)

The policy requires a minimal **conceptual** API; no UI or wire format is specified.

| Operation | Meaning |
|-----------|--------|
| **list** | Return the set of available profiles (at least the default; plus any user profiles). |
| **select** | Set CurrentProfileId to the given profile; optionally set PreviousProfileId to the previous current. Persist. |
| **reset** | Perform factory reset as in §6 (remove or deactivate user profiles; current = default; clear previous). |

Add/remove user profiles may be part of a larger "profile management" surface; for v0 the policy only requires that user profiles can be added and removed and that list/select/reset are defined as above.

---

## 8) Implementation notes (non-normative)

- **E220 / UART:** Today modulation is typically expressed as a **UART preset** (e.g. index or name). The policy remains stable if future firmware exposes explicit SF/BW/CR (e.g. via SPI or another adapter); the profile model still holds channel + modulation preset (or explicit params) + txPower.
- **Registry bundle ([#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184)):** Can deliver default profile definition or extended profile sets later. This policy does not depend on bundle delivery for OOTB defaults; the default profile may be firmware-built-in or loaded from a bundled registry. Layering: bundle format can deliver registries; OOTB defaults exist regardless.

---

**DoD (per [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211)):** Policy doc written; no contradictions with "OOTB defaults" principle (see [ai_model_policy](../../../../dev/ai_model_policy.md)).
