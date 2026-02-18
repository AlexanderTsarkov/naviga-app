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
- **NodeTable / beacon split** — [field_cadence_v0](areas/nodetable/policy/field_cadence_v0.md) §2: canonical **Core / Tail-1 / Tail-2** definitions; Tail-1 **core_seq16** (CoreRef-lite); bootstrap/backoff for Tail-2. **Alive + RX semantics (PR #205):** [beacon_payload_encoding_v0](areas/nodetable/contract/beacon_payload_encoding_v0.md) Core only with valid fix; [alive_packet_encoding_v0](areas/nodetable/contract/alive_packet_encoding_v0.md) for no-fix liveness; [rx_semantics_v0](areas/nodetable/policy/rx_semantics_v0.md) accepted/duplicate/ooo, seq16 single per-node.
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

- Channel list source & local discovery flow → [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175). *Clarification: #175 is post V1-A; V0/V1-A uses deterministic OOTB default profile per [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211).*
- Registry bundle format → [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184) doc added ([registry_bundle_format_v0](areas/registry/contract/registry_bundle_format_v0.md)); path/pipeline implementation when stable.
- Secure claim protocol & threat model (if required for v1) → [#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187); stub only today.
- NodeTable implementation order / wiring for first slice → [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147).

### Next (see § Next issues)

Next 3–5 docs-only tasks are listed in **Next issues (proposed)** below; they reflect current spec_map state, not aspirational backlog.

---

## 1) Inventory

| Area | Doc | Kind | Status | Defines | Depends on | Follow-up (issue) | Owner | Blocker for v1 slice? | Promotion note | V1-A? | Promote |
|------|-----|------|--------|---------|------------|-------------------|-------|------------------------|----------------|-------|--------|
| WIP context | [README.md](README.md) | — | — | WIP statuses, promotion rule | — | — | — | N | — | OUT | — |
| NodeTable | [areas/nodetable/index.md](areas/nodetable/index.md) | Doc | v0 | Spec hub for NodeTable (links, scope, follow-ups) | Policies, registries, contracts below | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | **Y** — central contract; many consumers | Links complete + aligned with PR #205 (Alive/RX/Core-only-with-fix/seq16) | IN | Ready |
| NodeTable contract | [areas/nodetable/contract/link-telemetry-minset-v0.md](areas/nodetable/contract/link-telemetry-minset-v0.md) | Doc | v0 | Link/Metrics & Telemetry/Health minset (#158) | Source precedence, snapshot/restore | [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) | — | N — encoding doc exists | — | IN | Ready |
| Beacon payload encoding | [areas/nodetable/contract/beacon_payload_encoding_v0.md](areas/nodetable/contract/beacon_payload_encoding_v0.md) | Doc | v0 | Byte layout, payload budgets, profile coupling (#173); Core only with valid fix; position-bearing vs Alive-bearing (§3.1) | Minset, RadioProfiles registry, alive_packet_encoding | — | — | N — [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) doc done | PR #205: Core-only-with-fix; Alive-bearing | IN | Ready |
| NodeTable contract | [areas/nodetable/contract/alive_packet_encoding_v0.md](areas/nodetable/contract/alive_packet_encoding_v0.md) | Doc | v0 | Alive packet (no-fix liveness): minimal payload version+nodeId+seq16; same per-node seq16; alive-bearing, non-position-bearing | beacon_payload_encoding, field_cadence | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | N | PR #205 | IN | Ready |
| NodeTable policy | [areas/nodetable/policy/rx_semantics_v0.md](areas/nodetable/policy/rx_semantics_v0.md) | Doc | v0 | RX semantics: accepted/duplicate/ooo/seq16 wrap; lastRxAt/link metrics/Activity; Tail-1 no revoke; Alive satisfies liveness; seq16 single per-node across types | beacon_payload_encoding, alive_packet_encoding, field_cadence, activity_state | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | N | PR #205 | IN | Ready |
| NodeTable policy | [areas/nodetable/policy/field_cadence_v0.md](areas/nodetable/policy/field_cadence_v0.md) | Doc | v0 | Field criticality & cadence: Core/Tail-1/Tail-2 tiers, cadences, degrade order, mesh priority, DOG_COLLAR vs HUMAN | Traffic model, minset, encoding | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | N | seq16 canonical (PR #205) | IN | Ready |
| NodeTable policy | [areas/nodetable/policy/nodetable_fields_inventory_v0.md](areas/nodetable/policy/nodetable_fields_inventory_v0.md) | Doc | v0 | Fields inventory (extract from WIP docs only); Coupling rules; owner worksheet for Tier/cadence/placement | field_cadence, minset, encoding, hub, traffic_model | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | N | Owner decisions complete (PR #204), semantics aligned (PR #205) | IN | Ready |
| NodeTable policy | [areas/nodetable/policy/position_quality_v0.md](areas/nodetable/policy/position_quality_v0.md) | Doc | v0 | Position quality: Tail-1 posFlags/sats (sent); PositionQuality state derived from Core + Tail-1 + optional age | encoding, inventory, field_cadence, index §6 | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | N | Iteration 3: canonized Tail-1 position-quality fields | IN | Ready |
| NodeTable policy | [areas/nodetable/policy/activity_state_v0.md](areas/nodetable/policy/activity_state_v0.md) | Doc | v0 | Activity/presence state (derived): Unknown/Active/Stale/Lost; parameterized thresholds; lastRxAt/seq16 rules; UI expectations | field_cadence, encoding, traffic_model | [#197](https://github.com/AlexanderTsarkov/naviga-app/issues/197) | — | N | Derived-only; no new radio fields | IN | Ready |
| NodeTable policy | [areas/nodetable/policy/link_metrics_v0.md](areas/nodetable/policy/link_metrics_v0.md) | Doc | v0 | Receiver-derived link metrics: rssiLast/snrLast, rssiRecent/snrRecent (bounded window); update rules; optional rxDuplicates/rxOutOfOrder | activity_state_v0, field_cadence, minset, inventory | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | — | N | Derived-only; Recent not Avg for current link | IN | Ready |
| Hardware | [areas/hardware/registry_hw_capabilities_v0.md](areas/hardware/registry_hw_capabilities_v0.md) | Doc | v0 | HW capabilities (hw_profile_id, adapter_type, confidence, local vs remote) | — | [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184) (distribution) | — | N | — | IN | Ready |
| Radio | [areas/radio/registry_radio_profiles_v0.md](areas/radio/registry_radio_profiles_v0.md) | Doc | v0 | RadioProfiles & ChannelPlan (Default/LongDist/Fast, profile–channel compatibility) | HW registry | [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) (channel discovery) | — | **Y** — channel list/source and discovery flow not fully specified | — | MAYBE | Candidate |
| Registry | [areas/registry/distribution_ownership_v0.md](areas/registry/distribution_ownership_v0.md) | Doc | v0 | Distribution & ownership (source of truth, bundling, schema rev, BT vs registry merge) | HW + Radio registries | — | — | N | — | IN | Ready |
| Registry | [areas/registry/contract/registry_bundle_format_v0.md](areas/registry/contract/registry_bundle_format_v0.md) | Doc | v0 | Bundle format v0: schema (metadata, contents, registries), versioning, integrity, transport-independent, replace/merge semantics | distribution_ownership_v0, HW + Radio registries | [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184) | — | N | #184 bundle format contract | IN | Ready |
| Radio policy | [areas/radio/policy/radio_profiles_policy_v0.md](areas/radio/policy/radio_profiles_policy_v0.md) | Doc | v0 | RadioProfile model; default (immutable) + user profiles; CurrentProfileId/PreviousProfileId; first boot; factory reset; minimal API (list/select/reset) | selection_policy, registry_radio_profiles | [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211) | — | N | OOTB defaults + persistence + factory reset | IN | Candidate |
| Radio policy | [areas/radio/selection_policy_v0.md](areas/radio/selection_policy_v0.md) | Doc | v0 | SelectionPolicy (default profile/channel, throttling, user advice) | #159, #158 | [#180](https://github.com/AlexanderTsarkov/naviga-app/issues/180) (AutoPower) | — | N | — | IN | Ready |
| Radio policy | [areas/radio/autopower_policy_v0.md](areas/radio/autopower_policy_v0.md) | Doc | v0 | AutoPower (node-side tx power, bounds, hysteresis, fallback) | #159, #158 | — | — | N | — | IN | Ready |
| Radio policy | [areas/radio/policy/traffic_model_v0.md](areas/radio/policy/traffic_model_v0.md) | Doc | v0 | Traffic model: OOTB vs Session/Group; ProductMaxFrameBytes=96; no fragmentation; **mesh-lite beacon-only forwarding** (extras direct-link only) | Beacon encoding, NodeTable | — | — | N | Reference for cadence/mesh decisions | IN | Ready |
| Identity | [areas/identity/pairing_flow_v0.md](areas/identity/pairing_flow_v0.md) | Doc | v0 | Pairing flow (node_id, first-time/NFC connect, many-nodes, switch-confirm) | NodeTable identity, HW registry | [#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187) (secure claim) | — | N | — | IN | Ready |
| Identity | [areas/identity/secure_claim_concept_v0.md](areas/identity/secure_claim_concept_v0.md) | Stub | stub | Secure claim/provisioning (threat model + concept placeholder) | Pairing flow | — | — | **Y** — stub only; anti-spoofing/ownership not specified if required for v1 | — | OUT | Idea |
| Session | [areas/session/contract/session_identity_exchange_v0_1.md](areas/session/contract/session_identity_exchange_v0_1.md) | Doc | v0.1 | Session-only identity exchange: NameChunk, encoding limits, normalization, epoch/cache, traffic & UX policy | NodeTable index §4, traffic_model_v0 | [#194](https://github.com/AlexanderTsarkov/naviga-app/issues/194) | — | N | Session-only names; no beacon/OOTB; refs Meshtastic research | MAYBE | Candidate |
| Firmware lifecycle | [#186](https://github.com/AlexanderTsarkov/naviga-app/issues/186) | Issue | issue-only | FW update via app flow (sealed device) — spec TBD | BLE connection/pairing | [#186](https://github.com/AlexanderTsarkov/naviga-app/issues/186) | — | N | — | OUT | Idea |

*Kind: Doc = WIP spec doc; Stub = placeholder/concept only; Issue = issue-only, no doc yet. Owner: set when needed for accountability.*

*V1-A? = **IN** (in first vertical slice scope) | **OUT** (deferred) | **MAYBE** (conditional). **Promote** = **Idea** (early/stub) | **Candidate** (doc exists; review or deps in progress) | **Ready** (canon v0; ready to promote when implementation aligns).*

---

## 2) Blockers for first vertical slice

| Blocker | Issue | Status | Blocks |
|---------|-------|--------|--------|
| Beacon payload & encoding (byte layout, airtime) | [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) | Doc | Encoding contract done; implementation/validation in progress |
| NodeTable as central consumer (implementation order, wiring) | [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) | In progress | Single end-to-end slice until shape fixed |
| Channel discovery & selection (channel list source, local discovery flow) | [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) | Open | Post V1-A; V1-A uses deterministic OOTB default per [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211) |
| Secure claim (stub; threat model / protocol TBD) | [#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187) | Stub | Only if product requires ownership/anti-spoofing for first slice |
| Registry bundle format (path, JSON vs per-registry; implementation when stable) | [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184) | Doc | Bundle format contract done ([registry_bundle_format_v0](areas/registry/contract/registry_bundle_format_v0.md)); app/firmware consumption can proceed |

---

## 3) Next issues (proposed)

List of next docs-only tasks that align with this map. Update when spec_map or priorities change.

1. **[#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173)** — Beacon payload & encoding: contract done ([beacon_payload_encoding_v0.md](areas/nodetable/contract/beacon_payload_encoding_v0.md)); implementation/validation next.
2. **[#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)** — Channel discovery & selection (channel list source, local discovery flow).
3. **[#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184)** — Registry bundle format v0 doc added ([registry_bundle_format_v0](areas/registry/contract/registry_bundle_format_v0.md)); implementation (path, pipeline) when stable.
4. **[#187](https://github.com/AlexanderTsarkov/naviga-app/issues/187)** — Secure claim: expand stub to threat model + protocol outline if required for v1.
5. **#147 / NodeTable** — Implementation order and wiring for first vertical slice (docs/decisions, not code).

*Traffic model v0 ([traffic_model_v0.md](areas/radio/policy/traffic_model_v0.md)) is the reference for later cadence/mesh policy decisions.*

---

## 4) Related

- Umbrella: [#147 NodeTable — Define & Research](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- Canon areas: [areas/](areas/)

---

**Last updated:** 2026-02-19

**Changelog:**
- 2026-02-19: [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211) Radio profiles policy v0 — new [radio_profiles_policy_v0](areas/radio/policy/radio_profiles_policy_v0.md) (defaults, persistence, factory reset, first boot, minimal API). Spec_map: Inventory row added (IN, Candidate); #175 clarified as post V1-A, V1-A uses OOTB default per #211 (Open + Blockers).
- 2026-02-16: [#184](https://github.com/AlexanderTsarkov/naviga-app/issues/184) Registry bundle format v0: new contract [registry_bundle_format_v0](areas/registry/contract/registry_bundle_format_v0.md) (schema, versioning, integrity, transport-independent, replace/merge). Spec_map: Inventory + Blocker + Open + Next updated; NodeTable Next 6–7 removed (done); field_cadence note → seq16 canonical (PR #205).
- 2026-02-16: PR #205 merged — Alive packet + RX semantics; Core only with fix; seq16 scope unified. Inventory: [alive_packet_encoding_v0](areas/nodetable/contract/alive_packet_encoding_v0.md), [rx_semantics_v0](areas/nodetable/policy/rx_semantics_v0.md) added as IN/Ready; beacon_payload_encoding note updated. [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- 2026-02-16: Spec snapshot & V1-A lens: Inventory table +2 columns (V1-A? IN/OUT/MAYBE, Promote Idea/Candidate/Ready); NodeTable index Links updated (no orphans: beacon_payload_encoding, field_cadence, nodetable_fields_inventory, position_quality, activity_state, link_metrics). Status snapshot for [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) in PR. Docs-only.
- 2026-02-16: Link metrics v0 ([link_metrics_v0](areas/nodetable/policy/link_metrics_v0.md)) — receiver-derived rssiLast/snrLast, rssiRecent/snrRecent (bounded window, not lifetime avg); update rules; optional rxDuplicatesCount/rxOutOfOrderCount. Inventory + spec_map. [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- 2026-02-16: Activity/Presence state v0 ([activity_state_v0](areas/nodetable/policy/activity_state_v0.md)) — derived states (Unknown/Active/Stale/Lost), parameterized T_active/T_stale/T_lost, lastRxAt/seq16 rules, UI expectations. Spec_map Inventory + changelog. [#197](https://github.com/AlexanderTsarkov/naviga-app/issues/197)
- 2026-02-16: NodeTable iteration 3 (Position Quality v0): canonized Tail-1 position-quality fields (posFlags, sats only; no new encoding). New [position_quality_v0](areas/nodetable/policy/position_quality_v0.md) — sent vs derived, placement rule, UI derivation from Core + Tail-1 + optional age. Updated inventory (PositionQuality → Derived; posFlags/sats notes), field_cadence (posFlags/sats rows), beacon_payload_encoding (Tail-1 position-quality ref). Spec_map Inventory + this changelog. [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- 2026-02-16: PR #191 — Rebased on main. Spec_map superset: traffic_model_v0, field_cadence_v0, beacon_payload_encoding_v0, nodetable_fields_inventory_v0 in Inventory and Dashboard; mesh-lite beacon-only forwarding v0 + NodeTable/beacon split in Decided; Next items 6–7; single changelog entry.
- 2026-02-16: Field cadence v0 added ([field_cadence_v0.md](areas/nodetable/policy/field_cadence_v0.md)). Core/Tail-1/Tail-2 tiers, cadences, degrade order, mesh priority, DOG_COLLAR vs HUMAN. Traffic model v0 + Beacon payload encoding v0. Dashboard Decided + Inventory + Next.
- 2026-02-16: Tail-1 posFlags(uint8)+sats(uint8), hex example; Tail-2 Operational rule (on change + at forced Core); encoding Packet types + Core 19 B, Tail-1/Tail-2 layouts. Field cadence §2.1 Tail-2 scheduling, maxSilence10s Tier C. (Refs [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147))
- 2026-02-16: Owner decisions Iteration 2: no names over radio v0; networkName excluded from beacon/OOTB; localAlias+ShortId local-only; role derived from hwProfileId.
- 2026-02-16: Session identity exchange v0.1 contract ([areas/session/contract/session_identity_exchange_v0_1.md](areas/session/contract/session_identity_exchange_v0_1.md)) — NameChunk, encoding limits, normalization, epoch/cache, traffic & UX policy ([#194](https://github.com/AlexanderTsarkov/naviga-app/issues/194)).