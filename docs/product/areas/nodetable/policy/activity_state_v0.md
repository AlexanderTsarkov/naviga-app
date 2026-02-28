# NodeTable — Activity / Presence state v0 (Policy)

**Status:** Canon (policy).

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#197](https://github.com/AlexanderTsarkov/naviga-app/issues/197)

This policy defines the **v0 derived** activity/presence state for nodes: inputs, states, parameterized thresholds, update rules, and UI expectations. All semantics are **receiver-derived**; no new radio or beacon fields. Domain truth is this doc and the referenced contracts; no dependency on OOTB or UI as normative source.

---

## 1) Goal / Non-goals

**Goal:** Provide a canonical, radio-agnostic definition of node presence (Active / Stale / Lost) so UI and higher-level policies can show “last seen” and activity state consistently, based on received beacons and freshness markers.

**Non-goals:**
- No new radio or beacon fields.
- No implementation or code in this doc.
- No dependency on OOTB or UI as product truth.
- No mesh policy changes.

---

## 2) Inputs

| Input | Source | Meaning |
|-------|--------|---------|
| **seq16** | Core (BeaconCore) | Freshness marker per node; canonical in [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) §4.1. Used to detect duplicates, repeats, and out-of-order delivery. |
| **lastRxAt** | Receiver-derived | Time (receiver clock) when the last packet from this node was accepted. Updated on any received packet that passes version/node checks; time base is implementation-defined (monotonic or wall-clock) but must be consistent for ordering. |
| **ageSec** (lastSeenAge) | Receiver-derived | Elapsed time since lastRxAt; i.e. “time since last received packet from this node”. Used for state derivation and UI. |
| **Cadence / forced Core** | Policy | Core cadence and “forced Core” rules (when Core must be sent) come from [field_cadence_v0](field_cadence_v0.md) and [traffic_model_v0](../../../wip/areas/radio/policy/traffic_model_v0.md). They inform expected silence bounds; exact thresholds are **parameterized** below, not fixed in this doc. |

---

## 3) Definitions

### 3.1 States

| State | Meaning (v0) |
|-------|----------------|
| **Unknown** | No packet received from this node yet (e.g. new nodeId), or no valid lastRxAt. |
| **Active** | ageSec ≤ T_active. Node is considered present and recently heard. |
| **Stale** | T_active < ageSec ≤ T_stale. Node was recently heard but beyond “active” window; may still be in range. |
| **Lost** | ageSec > T_stale (or ageSec > T_lost if a separate bound is used). Beyond acceptable silence; treat as absent for presence purposes. |

**Note:** Index §5 uses Online / Uncertain / Stale / Archived. This doc uses Unknown / Active / Stale / Lost for clarity. Mapping: Active ≈ Online; Stale can align with Uncertain or Stale depending on thresholds; Lost ≈ long-term absence. Archived (retention/eviction) is out of scope here.

### 3.2 Parameterized thresholds

Thresholds are **parameters** supplied by policy or configuration. This doc does **not** assign numeric defaults.

| Parameter | Meaning | Constraint |
|-----------|---------|------------|
| **T_active** | Upper bound (seconds) for “Active”. ageSec ≤ T_active ⇒ Active. | Policy/implementation; may be derived from cadence (e.g. related to expected Core interval). |
| **T_stale** | Upper bound (seconds) for “Stale”. T_active < ageSec ≤ T_stale ⇒ Stale. | Policy/implementation; typically T_stale > T_active. |
| **T_lost** | Upper bound (seconds) for “Stale”; beyond this ⇒ Lost. ageSec > T_lost ⇒ Lost. | Optional; if not used, Lost can be defined as ageSec > T_stale. |

No magic numbers: implementations and product policy choose T_active, T_stale, T_lost (or equivalent) and may tie them to [field_cadence_v0](field_cadence_v0.md) (e.g. maxSilence, forced Core) or to role-based profiles later.

---

## 4) Rules

### 4.1 Updating lastRxAt

- On **any** received packet from a node (`Node_OOTB_Core_Pos`, `Node_OOTB_Core_Tail`, `Node_OOTB_Operational`, `Node_OOTB_Informative`, or `Node_OOTB_I_Am_Alive`) that is accepted (valid version, valid nodeId, and any payload checks the receiver applies), the receiver **MUST** update **lastRxAt** for that node to the current reception time.
- **ageSec** (lastSeenAge) is then derived as: current_time − lastRxAt (in seconds, or the receiver’s chosen unit). Activity state is derived from ageSec and the chosen thresholds.
- **Alive packet** is alive-bearing; when the node has no fix it sends Alive instead of BeaconCore. Receiving an Alive packet satisfies the “alive within maxSilence window” for Activity derivation. See [rx_semantics_v0](rx_semantics_v0.md) and [alive_packet_encoding_v0](../contract/alive_packet_encoding_v0.md).

### 4.2 Using seq16 to detect duplicates / repeats

- **seq16** is carried in BeaconCore ([beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) §4.1). For each node, the receiver may store the **last accepted seq16** (e.g. lastCoreSeq).
- If an incoming Core packet has **seq16 equal to** the last accepted seq16 for that node, the packet is a **duplicate or repeat** (same logical sample). The receiver may:
  - **Update lastRxAt** (so “last seen” time refreshes), and
  - **Not** treat the payload as a new sample for position/telemetry (avoid “time travel” or overwriting with the same data).
- If seq16 is **new** (e.g. strictly greater in modulo space, or per implementation’s ordering), the packet is a new sample; update lastRxAt and apply payload as usual.

### 4.3 Out-of-order handling (v0)

- v0 rule: **conservative**. If a packet arrives with seq16 that appears **older** than the last accepted seq16 (e.g. delayed or reordered), the receiver **MAY** either:
  - Ignore it for payload application (do not overwrite newer state), or
  - Accept it only for **lastRxAt** update if it is within a small tolerance, to avoid marking the node as “not heard” when the only packet received was reordered.
- In all cases, **do not** use an older seq16 to overwrite newer position or telemetry (“no time travel”). Exact reorder window is implementation-defined.

---

## 5) UI expectations

- **Always show “last seen” age:** UI should show how long ago the node was last heard (e.g. “last seen 30 s ago”), derived from ageSec. This is truthful and does not imply presence beyond what the data supports.
- **Do not animate/refresh position on duplicate seq:** When the receiver identifies a packet as a duplicate (same seq16), do not animate or refresh position/telemetry from that packet; only lastRxAt may be updated so that “last seen” stays accurate.
- **Presenting Stale / Lost:** UI may present Stale (e.g. dimmed, or “stale” label) and Lost (e.g. struck through, grey, or “lost”) in a way that reflects derived state. This doc does not define exact visual treatment; it is descriptive (e.g. “dim/strike/grey”) not UI-canon. Domain exposes **activityState** and **lastSeenAge**; UI chooses how to render them.

---

## 6) Open questions

- **maxSilence10s (`Node_OOTB_Informative`):** When available from `Node_OOTB_Informative` (`0x05`), a receiver could use **maxSilence10s** to adapt T_stale / T_lost per node (e.g. if the node declares a longer silence bound). Not in v0 scope; placeholder for later.
- **Role-based thresholds:** T_active / T_stale / T_lost may eventually differ by role (e.g. DOG_COLLAR vs HUMAN) or by tracking profile. Policy and [field_cadence_v0](field_cadence_v0.md) (role modifiers) can be extended to supply per-role or per-profile thresholds; this doc stays parameterized.

---

## 7) Related

- **Field cadence:** [field_cadence_v0](field_cadence_v0.md) — Core/Tail tiers, Core only with valid fix; maxSilence via Alive when no fix.
- **RX semantics:** [rx_semantics_v0](rx_semantics_v0.md) — accepted/duplicate/ooo; lastRxAt and Activity on Alive.
- **Beacon encoding:** [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) — seq16 in Common prefix; Core_Tail `ref_core_seq16` (Core linkage key). **Alive:** [alive_packet_encoding_v0](../contract/alive_packet_encoding_v0.md). **Informative (maxSilence10s):** [info_packet_encoding_v0](../contract/info_packet_encoding_v0.md).
- **Traffic model:** [traffic_model_v0](../../../wip/areas/radio/policy/traffic_model_v0.md) — frame limits, beacon-only forwarding.
- **NodeTable hub:** [../index.md](../index.md) §5 — Activity (derived states, lastSeenAge, policy-supplied boundary).
