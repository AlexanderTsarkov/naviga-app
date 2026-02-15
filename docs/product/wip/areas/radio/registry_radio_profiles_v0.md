# Radio — RadioProfiles & ChannelPlan registry (Registry v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)

This doc defines the **RadioProfiles** and **ChannelPlan** registry v0: user-facing abstraction (Default / LongDist / Fast), relation to channels, policy boundary, and non-goals. No firmware or app code changes; no semantics defined via OOTB screens. No real CAD/LBT; no Meshtastic-like raw LoRa parameter UI.

---

## 1) Purpose / phase

- **Purpose:** Provide a stable, user-facing abstraction over radio behavior so that UI and policy work with **profile names** and **channel compatibility** instead of raw SF/BW/CR. Registries hold **facts** (what exists, what is compatible); a separate **SelectionPolicy** (future) holds **choice rules** (autopower, throttling, defaults).
- **Phase:** Post–OOTB spec work; registry content is WIP. Implementation ownership is out of scope for this v0.

---

## 2) User abstraction: profile names (no SF/BW/CR UI)

- **User-facing profiles (v0):** **Default** | **LongDist** | **Fast**. Users choose a profile name; they do **not** see or edit raw LoRa parameters (SF, BW, CR) in normal flows.
- **Rationale:** Keeps UX simple and avoids invalid combinations; mapping from profile to actual air parameters is firmware/stack responsibility and may depend on **adapter_type** and **HW capabilities** (see [HW registry](../hardware/registry_hw_capabilities_v0.md)).
- **Constraint (documented, not defining Mesh semantics):** “Fast profile requires Mesh” (or similar) is a **product constraint** only — i.e. Fast may be offered only when Mesh is available or enabled. This doc does **not** define Mesh semantics; it only records that such a constraint exists.

---

## 3) Relation to ChannelPlan: compatibility

- **ChannelPlan** defines channels (e.g. frequency, regulatory region). Each channel (or channel range) can be **tagged** with **compatible RadioProfile(s)**.
- **What “compatible” means:** A **compatible channel** for a profile is one where nodes on that channel are expected to use mutually decodable air-parameter families for that profile. Different RadioProfiles may use different air-parameter families; therefore they must not share the same channel range by default, to avoid mutual interference and non-visibility.
- **Switching profile implies channel compatibility:** When the user switches to a given RadioProfile, the valid channel set is **those channels tagged as compatible with that profile**. If the current channel is not compatible with the new profile, the device/app should switch to a compatible channel (or prompt); exact UX is out of scope here.
- **Registries contain facts:** The RadioProfile registry lists profiles and their semantics (e.g. “LongDist = better range, lower rate”); the ChannelPlan registry (or combined table) lists channels and which profile(s) they support. **SelectionPolicy** (future doc/issue) defines **which** profile/channel to choose under which conditions (autopower, throttling, defaults).

---

## 4) Policy boundary

- **Registries:** Contain **facts** — which profiles exist, which channels exist, which profile–channel pairs are valid. No “when to switch” or “default for first boot” here.
- **SelectionPolicy:** Contains **choice rules** — default profile/channel, throttling under load, user-facing effects/advice. See [selection_policy_v0.md](selection_policy_v0.md). Autopower algorithm remains a separate follow-up.
- **v0 baseline policy:** UART track baseline: channel sense is OFF; collision mitigation via jitter-only. Any “utilization” is an estimate with low confidence.

---

## 5) Non-goals (v0)

- **No Meshtastic-like raw LoRa parameter UI:** Users do not set SF/BW/CR directly; profile abstraction only.
- **No real CAD/LBT in this doc:** **Sense OFF + jitter-only** remains the default assumption for the UART track. CAD/LBT may be a future capability flag in the HW registry but is not implemented or specified here.
- **No Mesh semantics defined:** “Fast profile requires Mesh” is documented as a constraint only; Mesh protocol and behavior are out of scope.

---

## 6) Open questions / follow-ups

- **SelectionPolicy:** Choice rules (defaults, throttling, user advice) are in [selection_policy_v0.md](selection_policy_v0.md). Autopower algorithm remains a separate follow-up.
- **Autopower algorithm:** How tx power or profile is adapted from environment/capabilities; out of scope here.
- **Channel discovery:** See [policy/channel-discovery-selection-v0.md](policy/channel-discovery-selection-v0.md) ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)) for local scan and future backend heatmap; registry defines compatibility, not discovery flow.
- **Beacon minset & encoding:** Payload and airtime (e.g. [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173)) are separate; this registry does not define packet format.

---

## 7) Related

- **HW Capabilities registry:** [../hardware/registry_hw_capabilities_v0.md](../hardware/registry_hw_capabilities_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159).
- **NodeTable contract:** [../nodetable/index.md](../nodetable/index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
- **Channel discovery & selection (stub):** [policy/channel-discovery-selection-v0.md](policy/channel-discovery-selection-v0.md) — [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175).
