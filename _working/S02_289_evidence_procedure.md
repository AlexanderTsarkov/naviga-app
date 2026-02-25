# #289 Evidence: Cadence from registry (profile record in flash)

**Issue:** #289 Wire cadence parameters from current profile record in flash  
**DoD:** Change the current profile record in flash (via provisioning) and observe changed TX cadence **without reflashing firmware**.

## Storage location

- **Namespace:** NVS `naviga`
- **Keys:** `role_interval_s` (uint32), `role_silence_10s` (uint32), `role_dist_m` (float)
- **API:** `load_current_role_profile_record()`, `save_current_role_profile_record()`, `get_ootb_role_profile(role_id)` (platform/naviga_storage.h, role_profile_ootb.cpp)

## Procedure to prove DoD

1. **Flash** one device with firmware that includes #289 (registry-driven cadence).
2. **Provision** role and ensure profile record exists: e.g. `set role 0` then `reboot`. After boot, log line should show `Applied: interval_s=18 ... cadence=registry`.
3. **Observe baseline:** With `gnss fix <lat> <lon>` and periodic `gnss move` (e.g. every 10s), capture serial log; note CORE TX interval (e.g. ~18 s for Person).
4. **Change record without reflashing:** Over serial send:
   - `profile interval 30`  (or `profile silence 9`, `profile distance 25` as needed)
   - `get profile` → confirm `interval_s=30` (or other field).
   - `reboot`
5. **Observe new cadence:** After reboot, log should show `Applied: interval_s=30 ... cadence=registry`. With same GNSS move pattern, CORE TX interval should be ~30 s (no code change).
6. **Optional:** `set role 1` then `reboot` restores OOTB Dog profile (9s/3/15m) into the record; next boot uses those values.

## Shell commands (provisioning)

| Command | Effect |
|---------|--------|
| `get profile` | Print current profile record (interval_s, silence_10s, dist_m, valid). |
| `profile interval <sec>` | Set minIntervalSec (1–3600); reboot to apply. |
| `profile silence <10s>` | Set maxSilence10s (1–255); reboot to apply. |
| `profile distance <m>` | Set minDisplacementM (0–1000); reboot to apply. |
| `set role 0\|1\|2` | Set role pointer and write OOTB profile (Person/Dog/Infra) into the record; reboot to apply. |

Constraint: `maxSilence >= 3 * minInterval` (enforced on set).

## Fallback

If no profile record in NVS or stored values invalid (range or invariant), boot uses **default** (Person 18/9/25) and writes it back to NVS so next boot has a valid record.
