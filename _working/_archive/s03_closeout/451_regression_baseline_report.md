# #451 regression baseline report

**Date:** 2026-03-13  
**Workflow:** Baseline-first regression (main → #451 on one node).

## Phase A — main

| Step | Result |
|------|--------|
| Tree | `main` @ d18f07a (includes #449) |
| Build | `pio run -e devkit_e22_oled_gnss` — SUCCESS |
| Node | One node only: `/dev/cu.wchusbserial5B3D0164361` (MAC 9c:13:9e:ab:ba:a0) |
| Flash | main firmware flashed — SUCCESS |
| Serial | ~22 s capture: ticks, GNSS FIX_3D, nodetable size=2; no overflow, no reboot |

**Verdict: main stable.**

## Phase B — #451

| Step | Result |
|------|--------|
| Tree | `issue/450-oled-s03-alignment` @ 01161b0 |
| Build | SUCCESS |
| Flash | #451 firmware flashed — SUCCESS |
| Serial | Right after boot banner: `***ERROR*** A stack overflow in task loopTask has been detected` → reboot loop |

**Verdict: #451 unstable (loopTask stack overflow).**

## Root cause (not OLED code)

- `git merge-base issue/450-oled-s03-alignment main` = **689506b** (commit before #449).
- **main** has 689506b → **d18f07a (#449)** (NodeTable snapshot buffer moved off loopTask stack).
- **#451** has 689506b → 01161b0 (OLED only); **#449 is not in the branch**.

So the overflow is the **same #448 bug**: the 8 KB NodeTable snapshot buffer is still on loopTask stack because the #451 branch was cut from main **before** #449 was merged. The OLED changes did not introduce the overflow; the **missing #449** did.

## Phase C — outcome

- **main stable, #451 unstable** → #451 confirmed as regression source **in the sense** that the branch as currently based reintroduces the crash.
- **Regression corridor:** base of branch (missing #449), **not** the OLED update/render/data-plumbing code itself.

## Next step (smallest action)

1. **Rebase #451 onto current main** so the branch includes #449.
2. Rebuild, re-flash the same node, capture serial again.
3. If stable after rebase → the fix is “rebase #451 onto main”; then PR can be updated and re-verified.
4. If still overflow after rebase → then narrow within #451 (e.g. disable OLED update, then specific plumbing).

No code change to OLED logic required for this fix; only branch base update.
