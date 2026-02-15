# Product Spec Map v0

**Snapshot:** 2026-02-15  
**Status:** WIP index only (not implementation requirements).  
**Canonical specs:** `docs/product/wip/areas/`

---

## 1) Inventory

| Area | Doc | Status | Defines | Depends on | Follow-ups (issues) | Blocker for v1 slice? |
|------|-----|--------|---------|------------|---------------------|------------------------|
| WIP context | [README.md](README.md) | — | WIP statuses, promotion rule | — | — | N |
| NodeTable | [areas/nodetable/index.md](areas/nodetable/index.md) | v0 | NodeTable contract (identity, activity, position, capabilities, radio context, etc.) | Policies, registries, contracts below | #147 | **Likely Y** — central contract; many consumers. |
| NodeTable contract | [areas/nodetable/contract/link-telemetry-minset-v0.md](areas/nodetable/contract/link-telemetry-minset-v0.md) | v0 | Link/Metrics & Telemetry/Health minset (#158) | Source precedence, snapshot/restore | #173 (beacon encoding), #159 (registries) | **Likely Y** — payload/encoding deferred; needed for first slice. |
| Hardware | [areas/hardware/registry_hw_capabilities_v0.md](areas/hardware/registry_hw_capabilities_v0.md) | v0 | HW capabilities (hw_profile_id, adapter_type, confidence, local vs remote) | — | #184 (distribution) | N — registry exists; distribution doc covers bundling. |
| Radio | [areas/radio/registry_radio_profiles_v0.md](areas/radio/registry_radio_profiles_v0.md) | v0 | RadioProfiles & ChannelPlan (Default/LongDist/Fast, profile–channel compatibility) | HW registry | #175 (channel discovery) | **Likely Y** — channel list/source and discovery flow not fully specified. |
| Registry | [areas/registry/distribution_ownership_v0.md](areas/registry/distribution_ownership_v0.md) | v0 | Distribution & ownership (source of truth, bundling, schema rev, BT vs registry merge) | HW + Radio registries | — | N — rules clear; implementation path open. |
| Radio policy | [areas/radio/selection_policy_v0.md](areas/radio/selection_policy_v0.md) | v0 | SelectionPolicy (default profile/channel, throttling, user advice) | #159, #158 | #180 (AutoPower) | N — policy defined; AutoPower separate. |
| Radio policy | [areas/radio/autopower_policy_v0.md](areas/radio/autopower_policy_v0.md) | v0 | AutoPower (node-side tx power, bounds, hysteresis, fallback) | #159, #158 | — | N — concept doc; implementation follows. |
| Identity | [areas/identity/pairing_flow_v0.md](areas/identity/pairing_flow_v0.md) | v0 | Pairing flow (node_id, first-time/NFC connect, many-nodes, switch-confirm) | NodeTable identity, HW registry | #187 (secure claim) | N — flow defined; secure claim is future. |
| Identity | [areas/identity/secure_claim_concept_v0.md](areas/identity/secure_claim_concept_v0.md) | stub | Secure claim/provisioning (threat model + concept placeholder) | Pairing flow | — | **Likely Y** — stub only; anti-spoofing/ownership not specified if required for v1. |

---

## 2) Likely blockers for first vertical slice

1. **Beacon payload & encoding** — Link/telemetry minset defines fields but not byte layout or airtime; [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) is the follow-up. Likely blocker for any slice that sends/receives beacons.
2. **Channel discovery & selection** — Radio registry defines profile–channel compatibility; channel list source and local discovery flow are in [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175). Likely blocker if v1 slice requires user-selectable channels.
3. **NodeTable as central consumer** — Many policies and contracts depend on NodeTable; implementation order and wiring may block until NodeTable shape is fixed. Likely blocker for a single “end-to-end” slice.
4. **Secure claim (stub)** — If v1 slice must enforce ownership or resist spoofing, the secure claim doc is stub-only; no threat model or protocol yet. Likely blocker only if product requires it for first slice.
5. **Registry bundle format** — Distribution doc states “bundled with app” but exact path/format (JSON, per-registry files) is implementation-defined. Likely blocker for app-side registry consumption until chosen.

---

## 3) Related

- Umbrella: [#147 NodeTable — Define & Research](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
- Canon areas: [areas/](areas/)
