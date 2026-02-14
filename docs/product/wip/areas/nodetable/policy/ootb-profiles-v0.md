# NodeTable — OOTB tracking profiles (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#155](https://github.com/AlexanderTsarkov/naviga-app/issues/155)

Step 1: Draft the OOTB profiles table and derivation rules. Distance granularity comes first; time between beacons is derived via **speedHint** (typical speed for the subject/role).

---

## 1) Derivation formulas (v0)

- **baseMinTimeSeconds** = `ceil_to_step(minDistMeters / speedHintMps, stepSeconds)`  
  Spatial granularity (minDistMeters) and assumed speed (speedHintMps in m/s) yield a raw interval; round up to the next step (stepSeconds TBD, e.g. 5 s or 10 s).

- **baseMinTimeSeconds** ≥ **baseMinTimeFloorSeconds**  
  Minimum interval floor (TBD, e.g. 2 s) to avoid overly aggressive send cadence.

- **maxSilenceSeconds** = `round(baseMinTimeSeconds × roleMultiplier)`  
  Integer seconds. Upper bound of acceptable beacon silence for this profile; used by scheduler and activity interpretation.

*(Activity interpretation thresholds are defined in §4 below.)*

**v0 rule (movement gating):** Movement gating is evaluated **once per baseMinTime cycle**. If minDist has not been reached by the end of the cycle, do not send; wait for the next cycle. This keeps GNSS sampling periodic and avoids “immediate send” behavior (energy-aware).

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

*derivedExampleBaseMinTime / derivedExampleMaxSilence are **illustrative** (raw = minDist/speedHint; base = ceil_to_step; maxSilence = round(base × roleMultiplier) integer seconds). Not final.*

- **Hiking default:** single `human` profile (e.g. 50–100 m granularity, ~5 km/h).
- **Dog tracking:** `dog` (tighter granularity, higher speed) + `human` handler (coarser, same speed hint as hiking).
- **Hunting:** `dog`, `human:beater`, `human:shooter` with distinct minDist/speedHint per role.

*(Infra profiles such as `infra:repeater` / `infra:gateway` deferred to a later step unless needed for clarity.)*

---

## 4) Activity interpretation (v0)

Step 2: Define activity status thresholds from **maxSilenceSeconds** plus two slacks (delivery vs activity), and an archive boundary.

### Definitions

- **deliverySlackSeconds** = `kDelivery × maxSilenceSeconds`, **kDelivery ≥ 1**  
  Accounts for airtime, mesh delays, and a small buffer beyond one nominal cycle.

- **activitySlackSeconds** = `kSlack × maxSilenceSeconds`, **kSlack > kDelivery**  
  Window where the last known position is still considered “roughly valid” (subject may have moved but is likely within a bounded uncertainty).

- **archiveAfterSeconds**  
  Policy threshold; may depend on affinity or product rules. **Initial guidance:** on the order of 12–24 h (exact value TBD).

### Status boundaries (lastSeenAgeSeconds)

All boundaries use **lastSeenAgeSeconds** (time since last beacon was received for this node):

| Status    | Condition (age = lastSeenAgeSeconds) |
|-----------|--------------------------------------|
| **Online**   | age ≤ deliverySlackSeconds |
| **Uncertain**| deliverySlackSeconds < age ≤ activitySlackSeconds |
| **Stale**    | activitySlackSeconds < age ≤ archiveAfterSeconds |
| **Archived** | age > archiveAfterSeconds |

### Recommended initial coefficient ranges (not hard)

- **kDelivery** ~ 1.1–1.3 — allows for ~1 missed cycle plus delivery variance.
- **kSlack** ~ 2–3 — allows for a few missed cycles while still treating the last position as useful; e.g. 3 cycles at 100 m granularity ≈ 300 m distance uncertainty.

These are policy-level defaults; implementations may tune per deployment.

### Optional derived metric (future)

- **uncertaintyRadiusMeters** ≈ `speedHintMps × age` (or equivalently `minDist × (age / baseMinTime)`).  
  Nice-to-have for UI/debug later; not required for v0 status logic.

*(Channel utilization adaptation — e.g. when to step expectedIntervalNow between baseMinTime and maxSilence under load — is Step 3; not part of this policy.)*

---

## 5) Open questions for Step 2 / 3

- **Refinement of kDelivery / kSlack** — tuning per profile or environment.
- **Channel utilization adaptation policy** — how expectedIntervalNow steps between baseMinTime and maxSilence when utilization is high.
- **stepSeconds choice** — 5 s vs 10 s vs other; impact on scheduler and activity derivation.
- **baseMinTimeFloorSeconds (floorSeconds) choice** — minimum base interval (e.g. 2 s).
- **maxSilence rounding** — whether maxSilence should also be stepped (e.g. to same step as base) or only integer-rounded from (base × roleMultiplier).
