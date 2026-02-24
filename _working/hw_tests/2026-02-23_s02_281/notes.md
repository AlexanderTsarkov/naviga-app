# Bench evidence: #281 role-derived TX cadence

**Issue:** #281 Wire role minInterval/maxSilence into TX cadence  
**Date:** 2026-02-23  
**Branch/PR:** `fw/281-wire-role-cadence`

## TX gating (implemented in this PR, per field_cadence_v0)

- **minInterval:** Send at most every min_interval_s (role: 18s / 9s / 360s). **Only when position was just committed** (allow_core); otherwise **no send** (NO_SEND). Alive does **not** fill min_interval when Core is gated by minDistance.
- **maxSilence:** Must send within max_silence. **CORE if GNSS fix, ALIVE only when no fix** (liveness).
- **minDistance (minDisplacementM):** From **role profile**: Person 25 m, Dog 15 m, Infra 100 m. Position committed on FIRST_FIX, or (dt ≥ min_time and displacement ≥ min_distance_m), or MAX_SILENCE. After first Core, no send until moved or maxSilence; then maxSilence forces Core (if fix) or Alive (if no fix).

## Expected behaviour (per role_profiles_policy_v0)

| Role   | minInterval | minDisplacementM | maxSilence10s | Expected CORE tx interval (approx) |
|--------|-------------|------------------|---------------|------------------------------------|
| 0 Person | 18 s      | 25               | 9 (90 s)      | ~18 s (when moved ≥25 m)            |
| 1 Dog    | 9 s       | 15               | 3 (30 s)      | ~9 s (when moved ≥15 m)             |
| 2 Infra  | 360 s     | 100              | 255           | ~360 s (when moved ≥100 m)         |

## Capture procedure (stable-start rule)

**Capture only after PR #287 is updated with NO_SEND + role minDistance and build is from branch `fw/281-wire-role-cadence`.**

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
