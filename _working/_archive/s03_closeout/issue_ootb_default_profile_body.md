## Goal

Lock the **intended default User Profile values for out-of-the-box (OOTB) behaviour** in docs/policy and, in a later step, align firmware defaults and runtime behaviour to those values. This is a non-blocking follow-up so current/old defaults are not forgotten and do not remain in use silently.

**Status:** Non-blocking; suitable for later cleanup / tail work. Do not treat as immediate execution.

---

## Observed symptom / motivation

On the node OLED, the role/profile parameter line (e.g. MI/MD/MS) shows values that appear **inconsistent with the intended newer defaults**. In particular, **max_silence** often appears as small values (e.g. `3` on one node, `9` on another), which look like legacy/older firmware-era defaults rather than the intended ones. This is not immediately blocking but should be tracked now so it is not forgotten.

---

## Intended default values to lock

| Role   | Min_Interval | Min_Distance | Max_Silence |
|--------|--------------|--------------|-------------|
| **Dog**  | 11           | 15           | 50          |
| **Human**| 22           | 30           | 110         |

These are the values that OOTB/default User Profiles should resolve to after this follow-up (docs + code alignment).

---

## Scope

- **In scope:** Canonical default OOTB User Profile values; where they are defined in docs and code; aligning firmware default provisioning and runtime readback (including OLED) to the intended values.
- **Out of scope:** Changing ad-hoc live node settings by hand; BLE (#361); full provisioning redesign; broad Product Truth debate. This issue is specifically about **intended default OOTB User Profile values**.

---

## Non-goals

- Do **not** implement the change in this issue; first lock intent, then execute in a follow-up.
- Do **not** reopen #447 or touch BLE / #361.
- Do **not** broaden into full provisioning redesign.
- Do **not** debate Product Truth broadly; scope is OOTB default profile values only.

---

## Future execution outline

1. Identify where current default role/profile values are defined in **docs** and **code**.
2. Compare current firmware/runtime/OOTB defaults against the **intended values** (table above).
3. Update **canonical docs/policy** if needed so the intended defaults are the single source of truth.
4. Update **code / default provisioning behaviour** if needed to match those values.
5. Verify on **real hardware** that OOTB/default profiles resolve to the intended values.
6. Confirm **OLED / runtime readback** (e.g. status, get profile) shows the corrected defaults.

---

## Exit criteria

- [ ] Intended default values (Dog / Human) are recorded in canonical docs/policy.
- [ ] Firmware defaults and OOTB provisioning behaviour align to those values (or a follow-up task is created with clear ownership).
- [ ] Verification on hardware: OOTB/default profile readback (serial, OLED) shows the intended Min_Interval, Min_Distance, Max_Silence per role.
- [ ] No regression to #447 or current behaviour except the intentional default-value change.
