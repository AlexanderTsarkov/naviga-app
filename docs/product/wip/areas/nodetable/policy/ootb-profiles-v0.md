# NodeTable — OOTB tracking profiles (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#155](https://github.com/AlexanderTsarkov/naviga-app/issues/155)

Step 1: Draft the OOTB profiles table and derivation rules. Distance granularity comes first; time between beacons is derived via **speedHint** (typical speed for the subject/role).

**Scope / Non-normative note.** This document describes **one OOTB consumer policy** for activity interpretation and staleness-boundary examples. It is **not** normative product truth for domain semantics. Domain, policy, and spec consume a **policy-supplied boundary** and do not depend on this doc as the only source.

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

*(Channel utilization adaptation is defined in §5 below.)*

---

## 5) Channel utilization adaptation (v0)

Step 3: Adapt **expectedIntervalNow** between baseMinTime and maxSilence based on channel utilization. Yielding is implicit from profile parameters; stabilization and severity bands avoid cascading and flapping.

### Definitions

- **channelUtilization** — Self-estimated airtime load (0..1 or %). Algorithm/source TBD; treat as provided by radio/runtime.

- **expectedIntervalNowSeconds** — Adaptive expected beacon interval. It **MUST** satisfy:
  **baseMinTimeSeconds** ≤ **expectedIntervalNowSeconds** ≤ **maxSilenceSeconds**.

### Adaptation direction (policy)

- When utilization is **high**, increase expectedIntervalNow in discrete steps toward maxSilence.
- When utilization is **low**, decrease expectedIntervalNow in discrete steps toward baseMinTime.
- **Hysteresis (policy):** Avoid flapping around thresholds; “high” and “low” bands may be separated or require persistence over time.

### No explicit priority in v0 (implicit yielding)

There is **no separate priority parameter** in v0. Yielding is implicit via baseMinTime and maxSilence:

- Roles with **smaller** baseMinTime and **smaller** maxSilence (more important tracking) remain faster.
- Roles with **larger** baseMinTime/maxSilence slow down more under congestion.

*Future hook:* An explicit priority override MAY be introduced later only if needed; not defined in v0.

### Step model (discrete adaptation)

expectedIntervalNow changes in **discrete step increments** (step size policy TBD). It is always **clamped** to [baseMinTimeSeconds, maxSilenceSeconds].

*Illustrative example (non-binding):* baseMinTime = 15 s, maxSilence = 45 s, step = 10 s. Under rising load: 15 → 25 → 35 → 45 s. Under falling load: 45 → 35 → 25 → 15 s.

### Severity-based step selection (“bands”) (v0)

Utilization bands **U1 < U2 < U3** (thresholds TBD). StepDelta at the moment of adaptation (then hold-off starts):

| Band (u = channelUtilization) | stepDelta (illustrative, tunable) |
|-------------------------------|-----------------------------------|
| u ≤ U1 (low)                  | −1 step (or symmetric decrease)   |
| U1 < u ≤ U2                   | +1 step                           |
| U2 < u ≤ U3                   | +2 steps                          |
| u > U3                        | +3 steps                          |

Band evaluation determines stepDelta once; then stabilization (hold-off) applies. Decreases may be made more conservative (e.g. −1 only) to reduce oscillation; policy TBD.

### Stabilization / hold-off (v0)

After changing expectedIntervalNow, **do not change it again** before **adaptationHoldSeconds**.

**Intent:** Allow the network time to respond before further adaptation. **Initial policy (TBD):** adaptationHoldSeconds ≈ one “network round” time, e.g. maxSilenceMaxAcrossActiveNodes or a bounded variant. Exact bounds TBD.

### Safety guarantees / invariants

- Even under congestion, a node **MUST** send “I am alive” at least every ~maxSilence. (deliverySlack is handled in activity interpretation §4.)
- Adaptation modifies **expectedIntervalNow** only; it does **not** change maxSilence (upper bound) or activity interpretation rules.
- This policy is compatible with movement gating: checks happen on cycles; no “immediate send” between cycles.

---

## 6) Open questions for Step 2 / 3

- **Refinement of kDelivery / kSlack** — tuning per profile or environment.
- **Ownership/source of channelUtilization** — radio firmware vs domain; algorithm TBD.
- **Initial default thresholds (U1/U2/U3) and step size** — definition and tuning.
- **Bounds for adaptationHoldSeconds** — avoid too long (slow reaction) or too short (flapping).
- **Decreases vs increases** — whether decreases should be symmetric or slower than increases.
- **stepSeconds choice** — 5 s vs 10 s vs other; impact on scheduler and activity derivation.
- **baseMinTimeFloorSeconds (floorSeconds) choice** — minimum base interval (e.g. 2 s).
- **maxSilence rounding** — whether maxSilence should also be stepped (e.g. to same step as base) or only integer-rounded from (base × roleMultiplier).
