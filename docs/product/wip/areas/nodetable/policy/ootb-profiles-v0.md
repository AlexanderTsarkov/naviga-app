# NodeTable — OOTB tracking profiles (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#155](https://github.com/AlexanderTsarkov/naviga-app/issues/155)

Step 1: Draft the OOTB profiles table and derivation rules. Distance granularity comes first; time between beacons is derived via **speedHint** (typical speed for the subject/role).

---

## 1) Derivation formulas (v0)

- **baseMinTimeSeconds** = `round_to_step(minDistMeters / speedHintMps)`  
  Spatial granularity (minDistMeters) and assumed speed (speedHintMps in m/s) yield a base interval. Rounding policy: TBD; e.g. round to nearest 5 s or 10 s step.

- **maxSilenceSeconds** = `baseMinTimeSeconds × roleMultiplier`  
  Upper bound of acceptable beacon silence for this profile; used by scheduler and activity interpretation.

*(activitySlack and exact activity thresholds are out of scope for this step; see Open questions.)*

---

## 2) Subject types and role convention

- **Base subject types:** `human` | `dog` | `infra`
- Profiles may refine by **role variant** in the same row using a colon, e.g.:
  - `human:beater`, `human:shooter`
  - `infra:repeater`, `infra:gateway`
- This is a naming convention in the table; no extra schema.

---

## 3) OOTB profiles table (v0)

| profileId       | role           | minDistMeters | speedHintMps | roleMultiplier | priority | derivedExampleBaseMinTime | derivedExampleMaxSilence |
|-----------------|----------------|---------------|--------------|----------------|----------|---------------------------|--------------------------|
| hiking-default  | human          | 75            | 1.4          | 1.5            | 1        | 54 → 60 s                 | 90 s                     |
| dog-tracking-dog| dog            | 25            | 2.2          | 1.5            | 2        | 11 → 10 s                 | 15 s                     |
| dog-tracking-handler | human  | 250           | 1.4          | 1.5            | 1        | 179 → 180 s               | 270 s                    |
| hunting-dog     | dog            | 8             | 2.5          | 1.5            | 2        | 3 → 5 s                   | 8 s                      |
| hunting-beater  | human:beater   | 18            | 1.2          | 1.5            | 1        | 15 → 15 s                 | 23 s                     |
| hunting-shooter | human:shooter  | 40            | 0.8          | 1.5            | 1        | 50 → 50 s                 | 75 s                     |

*derivedExampleBaseMinTime / derivedExampleMaxSilence are **illustrative** (minDist/speedHint → raw value, then rounded; maxSilence = base × roleMultiplier). Not final.*

- **Hiking default:** single `human` profile (e.g. 50–100 m granularity, ~5 km/h).
- **Dog tracking:** `dog` (tighter granularity, higher speed) + `human` handler (coarser, same speed hint as hiking).
- **Hunting:** `dog`, `human:beater`, `human:shooter` with distinct minDist/speedHint per role.

*(Infra profiles such as `infra:repeater` / `infra:gateway` deferred to a later step unless needed for clarity.)*

---

## 4) Open questions for Step 2 / 3

- **kSlack / activity thresholds** — activitySlackSeconds = kSlack × maxSilenceSeconds; bounds for Online / Uncertain / Stale / Archived.
- **Channel utilization adaptation policy** — how expectedIntervalNow steps between baseMinTime and maxSilence when utilization is high.
- **Final rounding step policy** — 5 s vs 10 s vs other; impact on scheduler and activity derivation.
