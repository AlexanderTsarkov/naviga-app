# Product Spec Map v0

**Snapshot:** 2026-02-16  
**Status:** WIP index only (not implementation requirements).  
**Canonical specs:** `docs/product/wip/areas/`

---

## Dashboard

*Read this first: what’s decided, in progress, open, next, and what blocks the first vertical slice.*

| Bucket | What |
|--------|------|
| **Decided** | WIP doc captures v0 decisions; rules clear. No open design choices blocking. |
| **In progress** | Doc or issue exists; follow-up work or dependency in flight. |
| **Open** | Open questions that block progress (need doc or issue). |
| **Next** | Next 3–5 docs-only tasks (proposed); see § Next issues. |
| **Blockers for first vertical slice** | Table below; each has issue link and status. |

### Decided (canonicalized in WIP)

- **Registry** — [distribution_ownership_v0](areas/registry/distribution_ownership_v0.md): source of truth, bundling, schema rev, BT vs registry merge ([#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184)).
- **Radio policy** — [selection_policy_v0](areas/radio/selection_policy_v0.md), [autopower_policy_v0](areas/radio/autopower_policy_v0.md), [traffic_model_v0](areas/radio/policy/traffic_model_v0.md): choice rules, throttling, AutoPower, traffic model (OOTB vs Session/Group, frame limits, **mesh-lite beacon-only forwarding v0**) ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159), [#180](https://github.com/AlexanderTsarkov/naviga-app/issues/180)).
- **NodeTable / beacon split** — [field_cadence_v0](areas/nodetable/policy/field_cadence_v0.md) §2: canonical **Core / Tail-1 / Tail-2** definitions; Tail-1 **core_seq16** (CoreRef-lite); bootstrap/backoff for Tail-2.
- **Identity pairing** — [pairing_flow_v0](areas/identity/pairing_flow_v0.md): node_id, first-time/NFC, many-nodes, switch-confirm ([#182](https://github.com/AlexanderTsarkov/naviga-app/issues/182)).
- **Hardware registry** — [registry_hw_capabilities_v0](areas/hardware/registry_hw_capabilities_v0.md): hw_profile_id, adapter_type, confidence, local vs remote ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)).

### In progress (doc/issue + status)

- **NodeTable hub** — [areas/nodetable/index.md](areas/nodetable/index.md) · Doc · [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).
- **NodeTable contract** — [link-telemetry-minset-v0](areas/nodetable/contract/link-telemetry-minset-v0.md) · Doc · [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159).
- **Beacon payload encoding** — [beacon_payload_encoding_v0](areas/nodetable/contract/beacon_payload_encoding_v0.md) · Doc · [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) (contract done).
- **Field cadence v0** — [field_cadence_v0](areas/nodetable/policy/field_cadence_v0.md) · Doc · [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) (Core/Tail tiers, cadences, role modifiers).
- **NodeTable fields inventory v0** — [nodetable_fields_inventory_v0](areas/nodetable/policy/nodetable_fields_inventory_v0.md) · Doc · [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) (owner worksheet; Tier/cadence/placement).
- **Radio registry** — [registry_radio_profiles_v0](areas/radio/registry_radio_profiles_v0.md) · Doc · [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) (channel discovery).
- **Identity secure claim** — [secure_claim_concept_v0](areas/identity/secure_claim_concept_v0.md) · Stub · [#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187).
- **Firmware lifecycle** — issue-only · [#186](https://github.com/AlexanderTsarkov/naviga-app/issues/186) (spec TBD).

### Open (what blocks progress)

- Channel list source & local discovery flow → [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175).
- Registry bundle format (path, JSON vs per-registry files) → implementation choice under [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184); doc update TBD.
- Secure claim protocol & threat model (if required for v1) → [#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187); stub only today.
- NodeTable implementation order / wiring for first slice → [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).

### Next (see § Next issues)

Next 3–5 docs-only tasks are listed in **Next issues (proposed)** below; they reflect current spec_map state, not aspirational backlog.

---

## 1) Inventory

| Area | Doc | Kind | Status | Defines | Depends on | Follow-up (issue) | Owner | Blocker for v1 slice? | Promotion note |
|------|-----|------|--------|---------|------------|-------------------|-------|------------------------|----------------|
| WIP context | [README.md](README.md) | — | — | WIP statuses, promotion rule | — | — | — | N | — |
| NodeTable | [areas/nodetable/index.md](areas/nodetable/index.md) | Doc | v0 | Spec hub for NodeTable (links, scope, follow-ups) | Policies, registries, contracts below | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | **Y** — central contract; many consumers | [Promotion criteria](areas/nodetable/index.md#promotion-criteria-wip--canon) in area hub |
| NodeTable contract | [areas/nodetable/contract/link-telemetry-minset-v0.md](areas/nodetable/contract/link-telemetry-minset-v0.md) | Doc | v0 | Link/Metrics & Telemetry/Health minset (#158) | Source precedence, snapshot/restore | [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) | — | N — encoding doc exists | — |
| Beacon payload encoding | [areas/nodetable/contract/beacon_payload_encoding_v0.md](areas/nodetable/contract/beacon_payload_encoding_v0.md) | Doc | v0 | Byte layout, payload budgets, profile coupling (#173) | Minset, RadioProfiles registry | — | — | N — [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) doc done | — |
| NodeTable policy | [areas/nodetable/policy/field_cadence_v0.md](areas/nodetable/policy/field_cadence_v0.md) | Doc | v0 | Field criticality & cadence: Core/Tail-1/Tail-2 tiers, cadences, degrade order, mesh priority, DOG_COLLAR vs HUMAN | Traffic model, minset, encoding | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | N | Align encoding with Core/Tail when freshness decided |
| NodeTable policy | [areas/nodetable/policy/nodetable_fields_inventory_v0.md](areas/nodetable/policy/nodetable_fields_inventory_v0.md) | Doc | v0 | Fields inventory (extract from WIP docs only); Coupling rules; owner worksheet for Tier/cadence/placement | field_cadence, minset, encoding, hub, traffic_model | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | N | Owner review: decide Tier/cadence/placement |
| Hardware | [areas/hardware/registry_hw_capabilities_v0.md](areas/hardware/registry_hw_capabilities_v0.md) | Doc | v0 | HW capabilities (hw_profile_id, adapter_type, confidence, local vs remote) | — | [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184) (distribution) | — | N | — |
| Radio | [areas/radio/registry_radio_profiles_v0.md](areas/radio/registry_radio_profiles_v0.md) | Doc | v0 | RadioProfiles & ChannelPlan (Default/LongDist/Fast, profile–channel compatibility) | HW registry | [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) (channel discovery) | — | **Y** — channel list/source and discovery flow not fully specified | — |
| Registry | [areas/registry/distribution_ownership_v0.md](areas/registry/distribution_ownership_v0.md) | Doc | v0 | Distribution & ownership (source of truth, bundling, schema rev, BT vs registry merge) | HW + Radio registries | — | — | N | — |
| Radio policy | [areas/radio/selection_policy_v0.md](areas/radio/selection_policy_v0.md) | Doc | v0 | SelectionPolicy (default profile/channel, throttling, user advice) | #159, #158 | [#180](https://github.com/AlexanderTsarkov/naviga-app/issues/180) (AutoPower) | — | N | — |
| Radio policy | [areas/radio/autopower_policy_v0.md](areas/radio/autopower_policy_v0.md) | Doc | v0 | AutoPower (node-side tx power, bounds, hysteresis, fallback) | #159, #158 | — | — | N | — |
| Radio policy | [areas/radio/policy/traffic_model_v0.md](areas/radio/policy/traffic_model_v0.md) | Doc | v0 | Traffic model: OOTB vs Session/Group; ProductMaxFrameBytes=96; no fragmentation; **mesh-lite beacon-only forwarding** (extras direct-link only) | Beacon encoding, NodeTable | — | — | N | Reference for cadence/mesh decisions |
| Identity | [areas/identity/pairing_flow_v0.md](areas/identity/pairing_flow_v0.md) | Doc | v0 | Pairing flow (node_id, first-time/NFC connect, many-nodes, switch-confirm) | NodeTable identity, HW registry | [#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187) (secure claim) | — | N | — |
| Identity | [areas/identity/secure_claim_concept_v0.md](areas/identity/secure_claim_concept_v0.md) | Stub | stub | Secure claim/provisioning (threat model + concept placeholder) | Pairing flow | — | — | **Y** — stub only; anti-spoofing/ownership not specified if required for v1 | — |
| Firmware lifecycle | [#186](https://github.com/AlexanderTsarkov/naviga-app/issues/186) | Issue | issue-only | FW update via app flow (sealed device) — spec TBD | BLE connection/pairing | [#186](https://github.com/AlexanderTsarkov/naviga-app/issues/186) | — | N | — |

*Kind: Doc = WIP spec doc; Stub = placeholder/concept only; Issue = issue-only, no doc yet. Owner: set when needed for accountability.*

---

## 2) Blockers for first vertical slice

| Blocker | Issue | Status | Blocks |
|---------|-------|--------|--------|
| Beacon payload & encoding (byte layout, airtime) | [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) | Doc | Encoding contract done; implementation/validation in progress |
| NodeTable as central consumer (implementation order, wiring) | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | In progress | Single end-to-end slice until shape fixed |
| Channel discovery & selection (channel list source, local discovery flow) | [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) | Open | v1 slice if user-selectable channels required |
| Secure claim (stub; threat model / protocol TBD) | [#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187) | Stub | Only if product requires ownership/anti-spoofing for first slice |
| Registry bundle format (path, JSON vs per-registry; implementation-defined today) | [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184) (or follow-up) | Open | App-side registry consumption until chosen |

---

## 3) Next issues (proposed)

List of next docs-only tasks that align with this map. Update when spec_map or priorities change.

1. **[#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173)** — Beacon payload & encoding: contract done ([beacon_payload_encoding_v0.md](areas/nodetable/contract/beacon_payload_encoding_v0.md)); implementation/validation next.
2. **[#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)** — Channel discovery & selection (channel list source, local discovery flow).
3. **[#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184)** — Registry distribution (bundle format/path doc update if needed).
4. **[#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187)** — Secure claim: expand stub to threat model + protocol outline if required for v1.
5. **#147 / NodeTable** — Implementation order and wiring for first vertical slice (docs/decisions, not code).
6. **Field cadence → encoding:** Align [beacon_payload_encoding_v0](areas/nodetable/contract/beacon_payload_encoding_v0.md) with Core/Tail tiers once freshness marker encoding (seq8 vs seq16) is decided ([field_cadence_v0](areas/nodetable/policy/field_cadence_v0.md)).
7. **Owner review:** Decide Tier / default cadence / packet placement using [nodetable_fields_inventory_v0](areas/nodetable/policy/nodetable_fields_inventory_v0.md); then update field_cadence and encoding as needed.

*Traffic model v0 ([traffic_model_v0.md](areas/radio/policy/traffic_model_v0.md)) is the reference for later cadence/mesh policy decisions.*

---

## 4) Related

- Umbrella: [#147 NodeTable — Define & Research](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- Canon areas: [areas/](areas/)

---

**Last updated:** 2026-02-16

**Changelog:**
- 2026-02-16: PR #191 — Rebased on main. Spec_map superset: traffic_model_v0, field_cadence_v0, beacon_payload_encoding_v0, nodetable_fields_inventory_v0 in Inventory and Dashboard; mesh-lite beacon-only forwarding v0 + NodeTable/beacon split in Decided; Next items 6–7; single changelog entry.
- 2026-02-16: Field cadence v0 added ([field_cadence_v0.md](areas/nodetable/policy/field_cadence_v0.md)). Core/Tail-1/Tail-2 tiers, cadences, degrade order, mesh priority, DOG_COLLAR vs HUMAN. Traffic model v0 + Beacon payload encoding v0. Dashboard Decided + Inventory + Next.
- 2026-02-16: Tail-1 posFlags(uint8)+sats(uint8), hex example; Tail-2 Operational rule (on change + at forced Core); encoding Packet types + Core 19 B, Tail-1/Tail-2 layouts. Field cadence §2.1 Tail-2 scheduling, maxSilence10s Tier C. (Refs [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147))
