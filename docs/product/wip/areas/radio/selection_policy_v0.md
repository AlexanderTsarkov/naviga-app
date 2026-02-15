# Radio — SelectionPolicy v0 (Policy WIP)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Consumes:** [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) (registries), [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158) (link/telemetry minset)

This policy defines **choice rules** for RadioProfile and ChannelPlan selection, throttling under load, and user-facing effects/advice. It **consumes** registries (facts) from [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) and NodeTable/link-telemetry signals from [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158). No firmware or app code changes; no semantics defined via OOTB — OOTB may be referenced only as an example/indicator.

---

## 1) Purpose / scope

- **Purpose:** Specify how the system chooses **default** RadioProfile and ChannelPlan, applies **throttling** when the channel is under load (without CAD/LBT), and surfaces **user-facing messaging** as effects and advice (not raw utilization numbers).
- **Boundary:** **Registries = facts** (what profiles/channels exist, what is compatible); **SelectionPolicy = rules** (which to choose, when to throttle, what to show the user). This doc defines the rules only; it does not redefine registry content.
- **Scope (v0):** Default selection; throttling behavior (jitter-only baseline); user warnings/events and suggested UI message text. Out of scope: autopower algorithm detail, Mesh semantics, raw LoRa parameter UI.

---

## 2) Definitions

| Term | Meaning |
|------|--------|
| **RadioProfile** | User-facing profile (Default / LongDist / Fast) from [registry_radio_profiles_v0](registry_radio_profiles_v0.md). Choice is constrained by HW capabilities and ChannelPlan compatibility. |
| **ChannelPlan** | Set of channels (or channel ranges) with compatibility tags per profile; from [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) registries. |
| **Throttling** | Reducing effective beacon cadence or deferring sends when the policy infers “channel busy.” v0: no CAD/LBT; mitigation is **jitter-only** (see guardrails). |
| **Utilization (estimate)** | Policy-internal signal derived from NodeTable/link metrics (e.g. rxRate, observed activity). **Low confidence** on UART track; used only to drive triggers, not shown as raw numbers to the user. |
| **Effects & advice** | User-visible outcomes: chosen profile/channel, throttling state, and **short messages** (e.g. “Channel busy — sending less often”) instead of numeric utilization. |

---

## 3) Inputs

SelectionPolicy consumes:

- **Registries ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)):** HW Capabilities ([registry_hw_capabilities_v0](../hardware/registry_hw_capabilities_v0.md)), RadioProfiles & ChannelPlan ([registry_radio_profiles_v0](registry_radio_profiles_v0.md)). Used to know valid profile/channel pairs and HW limits (e.g. which profiles this device supports).
- **NodeTable / link-telemetry minset ([#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158)):** Signals such as lastRxAt, rxRate (if present), rssiLast/snrLast, and derived linkQuality or activity. Used to infer “channel busy” or “many nodes active” for throttling triggers. No raw utilization value is exposed to the user; policy maps signals to **discrete states** and **advice**.
- **Device state:** Current profile, current channel, relationship (e.g. Owned vs Seen), and whether Mesh is enabled (for “Fast requires Mesh” constraint).

---

## 4) Outputs

- **Chosen profile and channel:** Default for first boot / OOTB and after factory reset; and valid transitions when the user or policy changes selection (respecting profile–channel compatibility from registries).
- **Throttling state:** Whether the policy is currently applying extra backoff/jitter (e.g. “throttling active” vs “normal”).
- **User-facing events / advice:** Short, non-technical messages for the UI (see table below). No raw utilization percentages or SF/BW/CR.

---

## 5) Guardrails (v0 baseline)

- **UART track:** Channel sense is **OFF**; collision mitigation is **jitter-only**. Any “utilization” or “channel busy” is an **estimate with low confidence**; policy and UI must not present it as a precise metric.
- **SPI/adapter_type:** If the registry marks higher confidence for utilization or sense for a given **adapter_type** (e.g. SPI_CHIP), future policy may differentiate; v0 treats all as **low confidence** for utilization and jitter-only for mitigation.
- **Fast profile:** Documented constraint only: “Fast requires Mesh.” Policy may offer Fast only when Mesh is available/enabled; no Mesh semantics are defined in this doc.

---

## 6) Trigger → Action → User impact → UI message

Policy maps **triggers** (from inputs) to **actions** and **user-visible effects**. UI should show **effects & advice**, not raw numbers.

| Trigger | Action | User impact | UI message (example) |
|--------|--------|--------------|------------------------|
| First boot / factory reset | Select default profile and a compatible default channel from registries. | User gets a working profile/channel without configuration. | (None required; or “Using Default profile.”) |
| User selects a new RadioProfile | Resolve compatible channels from ChannelPlan; if current channel incompatible, switch to a compatible channel or prompt. | Profile and channel stay consistent; no invalid combinations. | “Switched to LongDist. Channel updated for compatibility.” |
| Inferred “channel busy” (e.g. rxRate or activity above policy threshold) | Increase jitter / backoff; reduce effective beacon cadence within allowed range. | Fewer collisions; possibly slightly less frequent updates. | “Channel busy — sending less often to avoid interference.” |
| Inferred “channel busy” clears | Restore normal cadence/jitter. | Normal update rate. | (None, or “Sending at normal rate.”) |
| User selects Fast but Mesh not enabled | Do not apply Fast; keep current profile or prompt to enable Mesh. | Fast not used until Mesh available. | “Fast profile requires Mesh. Enable Mesh or choose another profile.” |
| Unknown hw_profile_id or unsupported channel in payload | Do not assume capabilities; prompt for app/registry update (per HW registry). | User is directed to update. | “Update the app to support this device.” |

*Exact thresholds (e.g. “channel busy”) and message copy are product/UX decisions; this table defines the **categories** of trigger, action, and advice.*

---

## 7) Non-goals (v0)

- No Meshtastic-like raw LoRa parameter UI (SF/BW/CR).
- No CAD/LBT implementation; sense OFF + jitter-only remains the baseline.
- No Mesh protocol semantics; only the product constraint “Fast requires Mesh” is documented.
- No autopower algorithm specification (future follow-up).
- OOTB is not normative; it may be cited as an example of default profile/channel or messaging only.

---

## 8) Open questions / follow-ups

- **Autopower algorithm:** How to adapt tx power or profile from environment and capabilities; separate doc/issue.
- **Exact “channel busy” thresholds:** How rxRate/activity maps to discrete states; implementation-defined with policy guidance.
- **Default profile/channel table:** Concrete OOTB defaults (e.g. Default profile + region-specific channel) as example; not normative in this doc.
- **Localization:** UI message strings are placeholders; final copy and localization are product/UX.

---

## 9) Related

- **HW Capabilities registry:** [../hardware/registry_hw_capabilities_v0.md](../hardware/registry_hw_capabilities_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159).
- **RadioProfiles & ChannelPlan registry:** [registry_radio_profiles_v0.md](registry_radio_profiles_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159).
- **Link/Telemetry minset:** [../nodetable/contract/link-telemetry-minset-v0.md](../nodetable/contract/link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158).
- **NodeTable contract:** [../nodetable/index.md](../nodetable/index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
- **Channel discovery (stub):** [policy/channel-discovery-selection-v0.md](policy/channel-discovery-selection-v0.md) — [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175).
