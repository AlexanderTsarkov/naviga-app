# Bench evidence: #281 role-derived TX cadence

**Issue:** #281 Wire role minInterval/maxSilence into TX cadence  
**Date:** 2026-02-23  
**Branch/PR:** `fw/281-wire-role-cadence`

## Expected behaviour (per role_profiles_policy_v0)

| Role   | minInterval | maxSilence10s | Expected CORE tx interval (approx) |
|--------|-------------|---------------|------------------------------------|
| 0 Person | 18 s      | 9 (90 s)      | ~18 s                              |
| 1 Dog    | 9 s       | 3 (30 s)      | ~9 s                               |
| 2 Infra  | 360 s     | 255           | ~360 s                             |

## Capture procedure (stable-start rule)

1. Flash two devices with PR build (`fw/281-wire-role-cadence`).
2. Provision:
   - **Device A:** set role 0, set radio 0, reboot.
   - **Device B:** set role 1, set radio 0, reboot.
3. Wait ~28 s after boot (allow first CORE).
4. On both: `debug on`, wait 2 s, flush logs.
5. Capture 240 s of serial logs.
6. Save: `device_A_serial.log`, `device_B_serial.log`.

## Analysis (to fill after capture)

- Count CORE tx lines per device; compute inter-TX intervals (dt).
- **Pass:** Device A avg dt ≈ 18 s (±20%); Device B avg dt ≈ 9 s (±20%).
- Attach short summary (counts + avg/p95 dt) here or in a separate `analysis.md`.

## Artifacts (placeholders until HW run)

- `device_A_serial.log` — (to be captured)
- `device_B_serial.log` — (to be captured)
- `analysis.md` or inline summary — (to be filled)
