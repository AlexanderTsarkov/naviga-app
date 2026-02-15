# Radio — AutoPower policy v0 (Policy WIP)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue Ref:** [#180](https://github.com/AlexanderTsarkov/naviga-app/issues/180) · **Consumes:** [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) (registries), [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158) (link/telemetry minset)

This policy defines **node-side (firmware) AutoPower** behavior: how tx power is adjusted within user-defined bounds using link-quality signals and confidence constraints. **Registries = facts** (what the HW supports); **AutoPower = policy** (when to step up/down, hysteresis, fallbacks). No firmware or app code changes; no promises of “true” CAD/LBT/RSSI on UART. Thresholds are parameterized/qualitative where possible.

---

## 1) Purpose / scope

- **Purpose:** Specify how the **node** (firmware) adjusts transmit power within user-controlled bounds (Min/Med/Max) when **AutoPower** is on, using link-quality inputs and **confidence** (UART vs SPI) to avoid reacting to unreliable data.
- **Scope (v0):** Input signals and confidence behavior; fallback when RSSI is missing or invalid; transition table / state machine with hysteresis; user controls (Auto on/off + ceiling/floor); telemetry and events for the app to explain effects. No hard numeric tuning; no secure provisioning or anti-spoofing.

---

## 2) Definitions

| Term | Meaning |
|------|--------|
| **AutoPower** | Node-side policy that automatically steps tx power up or down within user-set bounds (Min/Med/Max) based on link-quality signals. User can turn Auto on/off. |
| **Power level** | Discrete level (e.g. Min / Med / Max) or implementation-defined steps within that range. Bounds come from user settings; policy chooses level within bounds. |
| **Link-quality signal** | Input used to decide power steps: e.g. RSSI (optional), loss/timeout, expected packet absence (e.g. “expected RX from peer did not arrive”). Confidence (low/med/high) qualifies how reliable the signal is. |
| **Confidence** | Per-signal or per-adapter: **low** \| **med** \| **high**. From HW registry ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)): UART typically **low** for RSSI/utilization; SPI may have **med** or **high** for some metrics. Policy uses confidence to decide whether to react and how aggressively. |
| **Hysteresis** | Rules to avoid rapid toggling: e.g. require signal to persist for a duration or cross a dead band before changing power level. |

---

## 3) Input signals and confidence behavior

AutoPower consumes the following. **Confidence** is determined by adapter_type and HW capabilities (see [HW registry](../hardware/registry_hw_capabilities_v0.md)); policy behavior depends on it.

| Signal | Description | When missing / invalid | Confidence (v0) |
|--------|-------------|-------------------------|-----------------|
| **RSSI** (optional) | Last or smoothed RSSI from link (e.g. from [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158) minset). | **Fallback:** Do not use RSSI for power steps; use only loss/timeout and expected absence. Treat “no RSSI” as **low confidence** and apply fallback behavior (see §5). | UART: **low** (if present). SPI: **med** or **high** per registry. |
| **Loss / timeout** | Indication that expected traffic was not received within a timeout (e.g. no RX from peer within expected window). | If no timeout/loss signal is available, policy may not step up on “missing peer”; only step down on other cues or keep current level. | Same as above; loss is often available even when RSSI is not. |
| **Expected packet absence** | Policy knows that a packet was expected (e.g. from schedule or peer’s pattern) and it did not arrive. | Used as a weak “link may be marginal” cue when RSSI is unavailable. | Low when used alone. |

**Confidence rules (v0):**

- **Low confidence (e.g. UART):** Prefer **conservative** reactions: smaller steps, longer hysteresis, or no power-up on RSSI alone; rely more on loss/timeout and expected absence. Do not promise “true” RSSI/SNR accuracy.
- **Med / high confidence (e.g. SPI when supported):** Policy may use RSSI and loss more aggressively (still within hysteresis and bounds). Exact thresholds remain parameterized.

---

## 4) Fallback when RSSI is missing or invalid

- If **RSSI is missing** (not reported by stack or HW): AutoPower **must not** use RSSI for transitions. Use only **loss/timeout** and **expected packet absence** (and any other non-RSSI cues) to decide power steps. If no usable signal exists, **hold current power level** (no change).
- If **RSSI is invalid** (out-of-range, stale, or marked invalid): Treat as missing; same fallback as above.
- **Stability:** When operating in fallback mode (no RSSI), policy may still step **down** on repeated loss/absence; step **up** only when there is a clear cue (e.g. repeated successful RX after a step-down) to avoid runaway.

---

## 5) Transition table / state machine (power levels with hysteresis)

AutoPower maintains a **current power level** within the user-set bounds (Min/Med/Max). Transitions are driven by link-quality signals and constrained by **hysteresis** and **confidence**.

**States (conceptual):** Power level = **Min** | **Med** | **Max** (or implementation-defined steps within [Min, Max]). User sets **floor** (min) and **ceiling** (max); policy chooses level inside that range.

**Transition rules (qualitative; exact thresholds parameterized):**

| Condition | Action | Hysteresis / stability |
|-----------|--------|-------------------------|
| Strong / repeated indication that link is **poor** (e.g. loss, timeout, expected absence; or RSSI below threshold when confidence allows) | Consider **step down** (Min ← Med ← Max). | Require condition to persist for a **minimum duration** (or N events) before stepping down; avoid single-packet reaction. |
| Strong / repeated indication that link is **good** (e.g. RX resumed, or RSSI above threshold when confidence allows) | Consider **step up** (Max ← Med ← Min). | Require condition to persist before stepping up; **dead band** or delay to avoid oscillation. |
| RSSI missing or invalid | Use **fallback** (§4); no RSSI-driven step. | Same hysteresis for loss/absence-driven steps. |
| AutoPower off | Hold user-selected **fixed** level; no automatic transitions. | N/A. |
| User changes Min/Max bounds | Recompute allowed range; if current level is outside new bounds, clamp to nearest allowed level. | Immediate clamp; no hysteresis for bound change. |

**Stability rules:**

- Do not change level more often than a **minimum interval** (parameterized; e.g. one step per N seconds or per M events).
- When confidence is **low**, use **longer** hysteresis and **smaller** or fewer steps than when confidence is med/high.

---

## 6) User controls and safety bounds

| Control | Semantics |
|---------|-----------|
| **AutoPower on/off** | When **on**, policy adjusts power within Min–Max per transition rules. When **off**, node uses a **fixed** power level (user-selected or last level). |
| **Min (floor)** | Minimum tx power the policy is allowed to use. Policy never goes below this when Auto is on. |
| **Med** | Optional middle reference (e.g. default or “normal”); policy may use it as default when link is uncertain. |
| **Max (ceiling)** | Maximum tx power the policy is allowed to use. Policy never goes above this when Auto is on. |

**Safety bounds:**

- Regulatory and HW limits (from registries or device) **override** user Min/Max: effective floor/ceiling = min(user_max, hw_max) and max(user_min, hw_min). Policy never exceeds regulatory/HW limits.
- If user sets Min > Max, treat as invalid; use a safe default (e.g. Med or last valid range) until user corrects.

---

## 7) Telemetry and events for the app (UX)

The following are **telemetry/events** the node can expose so the **app** can explain AutoPower state and effects to the user. **No UI implementation** is specified; only the data needed for UX.

| Item | Description |
|------|-------------|
| **AutoPower enabled** | Boolean: Auto on or off. |
| **Current power level** | Current tx power level (e.g. Min / Med / Max or ordinal). For display only; no raw dBm required in v0. |
| **User bounds** | Min and Max (and optionally Med) as set by user. |
| **Last transition reason** | Short reason for last power change: e.g. “link poor”, “link good”, “bounds changed”, “fallback (no RSSI)”. For UI message or debug. |
| **Confidence (current)** | Current confidence for link-quality inputs (low/med/high). Enables UI to show “AutoPower (best effort)” when low. |
| **Fallback active** | Boolean: true when policy is operating without RSSI (fallback mode). UI can show “Power adjustment without signal strength data.” |

**Events (optional, for UI):**

- **Power level changed** (level, reason).
- **AutoPower turned on/off**.
- **Bounds updated** (min, max).

Exact payload and transport (e.g. BT GATT, telemetry in beacon) are implementation-defined; this section only lists **what** the app needs to explain effects.

---

## 8) Non-goals (v0)

- No firmware or mobile code changes; this is a **spec** only.
- No promise of “true” CAD/LBT or accurate RSSI/SNR on UART; confidence reflects uncertainty.
- No secure provisioning or anti-spoofing design.
- No hard numeric tuning; thresholds and intervals remain **parameterized** or **qualitative**.
- No UI layout or implementation; only telemetry/events list for the app.

---

## 9) Open questions / follow-ups

- **Exact hysteresis parameters:** Minimum interval between steps, dead band, and N events — to be tuned in implementation; document chosen values when stable.
- **Mapping adapter_type → confidence:** HW registry ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)) provides confidence per capability; policy consumes it. Default mapping (e.g. UART → low, SPI → med) to be aligned with registry content.
- **Localization:** All user-facing strings (e.g. “link poor”, “fallback active”) are placeholders; product/UX owns final copy and localization.

---

## 10) Related

- **HW Capabilities registry:** [../hardware/registry_hw_capabilities_v0.md](../hardware/registry_hw_capabilities_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) (confidence, adapter_type).
- **RadioProfiles & ChannelPlan registry:** [registry_radio_profiles_v0.md](registry_radio_profiles_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159).
- **SelectionPolicy v0:** [selection_policy_v0.md](selection_policy_v0.md) — throttling and profile/channel selection; AutoPower is separate node-side power policy.
- **Link/Telemetry minset:** [../nodetable/contract/link-telemetry-minset-v0.md](../nodetable/contract/link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158).
- **NodeTable contract:** [../nodetable/index.md](../nodetable/index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
