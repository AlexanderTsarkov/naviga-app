# NodeTable — V1-A hub

**Status:** Canon.

Product hub for NodeTable (V1-A). Umbrella: [Issue #147](https://github.com/AlexanderTsarkov/naviga-app/issues/147). Source of truth for scope and links: this hub + [spec_map_v0](../../wip/spec_map_v0.md).

---

## 1) What is NodeTable

NodeTable is the **single source of truth for node-level knowledge** (including self). Other components emit observations or events; NodeTable stores best-known facts and derived state used for product and UX decisions. **Boundary:** NodeTable holds **node-level** state only; per-packet mesh state (e.g. PacketState for dedup/ordering) belongs to Mesh protocol, not NodeTable. Stream-level metadata in NodeTable (e.g. **lastOriginSeqSeen**) is permitted; per-packet cache is not.

---

## 2) V1-A invariants (summary)

Canon semantics are in the linked contract and policy docs; this list is a strict short summary. Do not extend or contradict these in the hub.

- **Core only with valid fix.** BeaconCore is transmitted only when the sender has a valid GNSS fix. Core implies a valid lat/lon sample. When the node has no fix, it does not send Core.
- **No-fix liveness via Alive.** The maxSilence liveness requirement is satisfied by an **Alive** packet when the node has no valid fix. Alive is **alive-bearing, non-position-bearing**; it carries identity + seq16 (same per-node counter as Core/Tails).
- **seq16 scope.** seq16 is a **single per-node counter** across all packet types (BeaconCore, Tail-1, Tail-2, Alive) during uptime. RX semantics: accepted / duplicate / out-of-order / wrap are defined per-node globally, not per packet type.
- **Tail-1 never revokes Core.** Tail-1 qualifies the **same** Core sample (core_seq16). Tail-1 **MUST NOT** revoke or invalidate position already received in BeaconCore. CoreRef-lite: Tail-1 payload is applied **only if** core_seq16 matches lastCoreSeq for that node; otherwise payload is ignored (lastRxAt/link metrics may still be updated).

---

## 3) Contracts

| Doc | Description |
|-----|-------------|
| [beacon_payload_encoding_v0](contract/beacon_payload_encoding_v0.md) | Beacon byte layout: Core (19 B) / Tail-1 / Tail-2; payload budgets; Core only with valid fix; position-bearing vs Alive-bearing. |
| [alive_packet_encoding_v0](contract/alive_packet_encoding_v0.md) | Alive packet: no-fix liveness; same per-node seq16; alive-bearing, non-position-bearing. |
| [link-telemetry-minset-v0](contract/link-telemetry-minset-v0.md) | Link/Metrics & Telemetry/Health minset (field set and coupling). |

---

## 4) Policies

| Doc | Description |
|-----|-------------|
| [rx_semantics_v0](policy/rx_semantics_v0.md) | RX semantics: accepted/duplicate/ooo/wrap; seq16 single per-node; Tail-1 no revoke; Alive satisfies liveness. |
| [field_cadence_v0](policy/field_cadence_v0.md) | Field criticality & cadence: Core / Tail-1 / Tail-2 tiers, degrade order, mesh priority, first-fix bootstrap. |
| [activity_state_v0](policy/activity_state_v0.md) | Activity/presence (derived): Unknown/Active/Stale/Lost; lastRxAt/seq16 rules; parameterized thresholds. |
| [link_metrics_v0](policy/link_metrics_v0.md) | Receiver-derived link metrics: rssiLast/snrLast, rssiRecent/snrRecent; update rules. |
| [nodetable_fields_inventory_v0](policy/nodetable_fields_inventory_v0.md) | Fields inventory, coupling rules, Tier/cadence/placement owner worksheet. |
| [position_quality_v0](policy/position_quality_v0.md) | Position quality: Tail-1 posFlags/sats; PositionQuality derived from Core + Tail-1. |

---

## 5) Open questions / future (post V1-A)

The following are **out of scope for V1-A**; do not block implementation or promotion on them.

- **Channel discovery & selection** — [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175). Channel list source and local discovery flow are **post V1-A**. V1-A uses deterministic OOTB default profile per [#211](https://github.com/AlexanderTsarkov/naviga-app/issues/211).
- Role/policy source & precedence (broadcast vs local).
- Tracking profile assignment and OOTB defaults (where set, precedence).
- Persistence mechanism, snapshot cadence, map relevancy vs remembered nodes.
- networkName propagation (cadence, payload); exact activity threshold tuning.

---

## 6) Related (other areas)

- [../../hardware/contract/registry_hw_capabilities_v0.md](../../hardware/contract/registry_hw_capabilities_v0.md) — HW Capabilities registry.
- [../../radio/policy/registry_radio_profiles_v0.md](../../radio/policy/registry_radio_profiles_v0.md) — RadioProfiles & ChannelPlan (channel discovery [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175) post V1-A).
- [../../wip/areas/radio/selection_policy_v0.md](../../wip/areas/radio/selection_policy_v0.md), [../../wip/areas/radio/autopower_policy_v0.md](../../wip/areas/radio/autopower_policy_v0.md) — Radio selection & AutoPower.
- [../../wip/areas/identity/pairing_flow_v0.md](../../wip/areas/identity/pairing_flow_v0.md) — Identity & pairing.
- [../../wip/areas/registry/distribution_ownership_v0.md](../../wip/areas/registry/distribution_ownership_v0.md) — Registry distribution.
- Policy (supporting, WIP): [../../wip/areas/nodetable/policy/source-precedence-v0.md](../../wip/areas/nodetable/policy/source-precedence-v0.md), [../../wip/areas/nodetable/policy/ootb-profiles-v0.md](../../wip/areas/nodetable/policy/ootb-profiles-v0.md), [../../wip/areas/nodetable/policy/persistence-v0.md](../../wip/areas/nodetable/policy/persistence-v0.md), [../../wip/areas/nodetable/policy/snapshot-semantics-v0.md](../../wip/areas/nodetable/policy/snapshot-semantics-v0.md), [../../wip/areas/nodetable/policy/restore-merge-rules-v0.md](../../wip/areas/nodetable/policy/restore-merge-rules-v0.md), [../../wip/areas/nodetable/policy/persistence-cadence-limits-v0.md](../../wip/areas/nodetable/policy/persistence-cadence-limits-v0.md).
- Research: [../../wip/areas/nodetable/research/](../../wip/areas/nodetable/research/).
