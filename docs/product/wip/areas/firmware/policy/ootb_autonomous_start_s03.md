# Firmware — OOTB autonomous start (S03)

**Status:** WIP (spec).  
**Issue:** [#354](https://github.com/AlexanderTsarkov/naviga-app/issues/354) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This doc defines **OOTB autonomous start** for S03: the trigger conditions and exact boot-time sequence by which a node starts beaconing and updating NodeTable **without any phone, app, or BLE interaction**. It ties to the [provisioning baseline](provisioning_baseline_s03.md) (Phase A/B/C and fallbacks) and to the [NodeTable master table](../../nodetable/master_table/README.md) (#352) as the consumer truth for what gets produced, updated, and sent on-air.

---

## 1) Definition: “autonomous” in S03

**Autonomous** means:

- **No phone, no BLE, no user action** required for the node to begin transmitting (Alive/Beacon) and receiving, and for NodeTable (self and remote entries) to be populated from provisioned config and runtime.
- **Factory default** (FACTORY default RadioProfile, default role) or **previously persisted** pointers (current role, current radio profile) are allowed. Provisioning may be one-time (e.g. at factory) or default-on-first-boot; the node does not wait for an app or BLE connection to start comms.
- S03 does **not** require user-defined profile creation or role change via UI/BT; OOTB/default only.

---

## 2) Preconditions and triggers

### 2.1 Trigger: power-on / reboot

- **Power on** or **reboot** is the trigger. There is no “wait for BLE” or “wait for user confirmation” step before Phase C starts comms.
- **First boot** = boot with no valid persisted NVS state for role/radio pointers (or empty/corrupted NVS). Behavior: Phase A applies FACTORY default to the radio; Phase B applies default role and default radio profile (id 0), then persists those pointers. OOTB comms work without any prior NVS. See [provisioning_baseline_s03.md](provisioning_baseline_s03.md) §6.

### 2.2 What must be true before Phase C starts comms

Per [boot_pipeline_v0](../../../../areas/firmware/policy/boot_pipeline_v0.md) and [provisioning_baseline_s03.md](provisioning_baseline_s03.md):

- **Phase A invariants:** GNSS and radio module are in required Naviga operating mode (verify-and-heal done; or RepairFailed with observable fault state and best-effort continue). Radio is configured with FACTORY default RadioProfile (channel_slot=1, rate_tier=STANDARD→2.4k, tx_power step0=MIN 21 dBm) so that OOTB comms can run. See [module_boot_config_v0](../../../../areas/firmware/policy/module_boot_config_v0.md).
- **Phase B invariants:** Role and current radio profile are **resolved** (default if missing/invalid) and **persisted**. No UI or backend is required; defaults exist so that “first comms” does not depend on them.

---

## 3) Boot-time sequence (canonical)

| Phase | Name | What happens (summary) |
|-------|------|------------------------|
| **Phase A** | **Hardware provisioning** | GNSS: open UART, send UBX config, verify link alive; repair on mismatch. E220: read module config, apply FACTORY default (channel, air_rate, tx power MIN), verify critical params; repair on mismatch. Outcomes: Ok / Repaired / RepairFailed. RepairFailed → set observable fault state; do not brick; continue best-effort. See [provisioning_baseline_s03.md](provisioning_baseline_s03.md) §3 and [module_boot_config_v0](../../../../areas/firmware/policy/module_boot_config_v0.md). |
| **Phase B** | **Product provisioning + persistence** | Read role and radio profile pointers from NVS; if missing or invalid → apply default role and default radio profile (id 0), persist. Radio hardware already configured in Phase A; Phase B only ensures logical state (pointers/records) is consistent for cadence, rollback, and future UI. See [provisioning_baseline_s03.md](provisioning_baseline_s03.md) §4 and [boot_pipeline_v0](../../../../areas/firmware/policy/boot_pipeline_v0.md) §4. |
| **Phase C** | **Start comms** | Start **Alive / Beacon cadence** using provisioned role and provisioned current profile. Begin RX path; NodeTable updates (self + remote) per encoding and RX semantics. No ad hoc defaults at “start comms” time. See [boot_pipeline_v0](../../../../areas/firmware/policy/boot_pipeline_v0.md) §5. |

**First-fix bootstrap** ([field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md) §2.1): While **no** GNSS fix, only **Alive (no-fix)** per maxSilence. When **first** valid fix and no baseline (first BeaconCore not yet sent), send first BeaconCore at next opportunity **without** min-displacement gate; then normal cadence applies.

---

## 4) Fallback behavior

| Scenario | Behavior |
|----------|----------|
| **Missing or invalid NVS pointers** | Phase B treats as “no valid current”; apply **default role** and **default radio profile** (id 0); persist pointers. Radio was already configured in Phase A with FACTORY default. OOTB comms proceed. See [provisioning_baseline_s03.md](provisioning_baseline_s03.md) §6. |
| **RepairFailed (radio or GNSS)** | Do **not** brick. Set **observable fault state** (e.g. boot_config_result = RepairFailed). Continue best-effort: Phase B and Phase C may run; device may participate with degraded config or diag-only. **Progressive signaling plan:** logs/diag now; fault LED later (product-defined); user-visible notification when phone connects later. See [module_boot_config_v0](../../../../areas/firmware/policy/module_boot_config_v0.md) §4 and [provisioning_baseline_s03.md](provisioning_baseline_s03.md) §3.4, §6. |

---

## 5) When do we start sending what (on-air packets)

The **packet set** and **fields per packet** are defined by the NodeTable master table and encoding contracts. This section **references** that truth only; it does not define new packet semantics.

**Source of truth:** [NodeTable master table README](../../nodetable/master_table/README.md), [packets_v0_1.csv](../../nodetable/master_table/packets_v0_1.csv), [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv). Packet names, purpose, cadence, gating, and fields included are as in those artifacts.

### 5.1 Packets that begin in OOTB (expected)

| Packet (canon name) | When OOTB starts sending | Gating / condition | Fields (see packets_v0_1.csv, fields_v0_1.csv) |
|--------------------|---------------------------|--------------------|-----------------------------------------------|
| **Alive** | As soon as Phase C starts, when **no GNSS fix**. | No-fix liveness; within maxSilence. | payloadVersion, node_id, seq16, aliveStatus (optional). Identity + seq16 only; non-position-bearing. |
| **Node_Core_Pos** | When **valid GNSS fix**; first Core at next opportunity without min-displacement (first-fix bootstrap); then every beacon tick when position valid. | Only with valid fix. | payloadVersion, node_id, seq16, positionLat, positionLon. |
| **Node_Core_Tail** | When position valid; every Tail-1 (ref_core_seq16 matches lastCoreSeq). | When position valid; Core linkage. | payloadVersion, node_id, seq16, ref_core_seq16, posFlags (opt), sats (opt). |
| **Node_Operational** | On change + at forced Core (cadence per field_cadence_v0). | No CoreRef. | payloadVersion, node_id, seq16, batteryPercent (opt), uptimeSec (opt). |
| **Node_Informative** | On change + every 10 min (cadence per field_cadence_v0). | MUST NOT send on every Operational. | payloadVersion, node_id, seq16, maxSilence10s (opt), hwProfileId (opt), fwVersionId (opt). |

**Conditional:** Node_Core_Pos and Node_Core_Tail are **conditional on GNSS fix**. Alive is sent when there is **no** fix to satisfy maxSilence liveness. Encoding and gating: [beacon_payload_encoding_v0](../../../../areas/nodetable/contract/beacon_payload_encoding_v0.md), [alive_packet_encoding_v0](../../../../areas/nodetable/contract/alive_packet_encoding_v0.md); cadence and first-fix: [field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md) §2.1.

---

## 6) NodeTable effects (consumer view)

Which fields become valid when, and which remain unknown until GNSS fix / RX / timeouts, is defined by the **NodeTable master table** (fields_v0_1.csv, packets_v0_1.csv). Columns **producer_status**, **source**, **trigger_type**, **packet_name_canon**, and **default_cadence** are the authority. This section only summarizes the OOTB-autonomous view; no new semantics.

### 6.1 Early-valid (self / config / provisioned)

- **Self identity:** node_id, payloadVersion, seq16 — from provisioned/config and TX path; valid as soon as Phase C starts.
- **Role-derived cadence:** minIntervalSec, minDisplacementM, maxSilence10s (from role record / default) drive when Alive vs Core vs Tail are sent; see field_cadence_v0 and the master table.
- **Config/informative:** fwVersionId, hwProfileId, maxSilence10s — from NodeEntry/config; may be sent in Node_Informative per cadence.
- **Operational (self):** batteryPercent, uptimeSec — from HW/uptime; sent in Node_Operational per cadence.

### 6.2 After GNSS fix

- **Position fields:** positionLat, positionLon, pos_valid, ref_core_seq16, sats (or pos_quality16 in v0.1) — valid when fix is available; Core_Pos and Core_Tail then sent per cadence and first-fix bootstrap (§3, §5).

### 6.3 After RX (injected / derived)

- **last_seen_ms / lastRxAt:** updated on RX; receiver-injected.
- **last_rx_rssi:** updated on RX (link metrics); receiver-injected. See master table and link_metrics policy.
- **is_stale, last_seen_age_s:** derived from last_seen_ms and cadence/grace; see presence_and_age_semantics.

### 6.4 Unknown until condition

- **Position:** unknown until GNSS fix; then Core_Pos/Tail populate.
- **Remote entries:** unknown until first RX from that node; then NodeTable updated per RX semantics and encoding.

**Reference:** [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv) (field_key, packet_name_canon, source, producer_status, trigger_type), [packets_v0_1.csv](../../nodetable/master_table/packets_v0_1.csv) (packet_name_canon, gating_rule, fields_included).

---

## 7) S03 constraints and what comes next

- **S03:** OOTB autonomous start is **embedded-only**: no BLE, no app, no user action. Factory default or previously persisted pointers; no user-defined profile UI/protocol in S03. NodeTable and on-air behavior are as defined by the master table and encoding contracts; this doc only ties boot sequence and triggers to that consumer truth.
- **S04 and later:** S04 adds the BLE bridge and UI; provisioning (role/profile selection, optional user profiles) may then be changed via app. S03 only guarantees that the node **autonomously** starts beaconing and updating NodeTable without phone/BLE.

---

## 8) Related

| Doc / issue | Description |
|-------------|-------------|
| [#354](https://github.com/AlexanderTsarkov/naviga-app/issues/354) | This task (OOTB autonomous start doc). |
| [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) | S03 umbrella planning. |
| [#352](https://github.com/AlexanderTsarkov/naviga-app/issues/352) | NodeTable Field Map (consumer truth). |
| [provisioning_baseline_s03.md](provisioning_baseline_s03.md) | Phase A/B/C, storage, fallbacks, effects on OOTB and NodeTable. |
| [boot_pipeline_v0](../../../../areas/firmware/policy/boot_pipeline_v0.md) | Canon boot phases and invariants. |
| [module_boot_config_v0](../../../../areas/firmware/policy/module_boot_config_v0.md) | GNSS and E220 verify/heal; failure behavior §4. |
| [provisioning_interface_v0](../../../../areas/firmware/policy/provisioning_interface_v0.md) | Serial provisioning; Phase B reads pointers. |
| [field_cadence_v0](../../../../areas/nodetable/policy/field_cadence_v0.md) | Tier/cadence; §2.1 first-fix bootstrap. |
| [NodeTable master table](../../nodetable/master_table/README.md) | [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv), [packets_v0_1.csv](../../nodetable/master_table/packets_v0_1.csv). |
