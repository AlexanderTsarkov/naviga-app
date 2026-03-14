# Goal

Define and execute the **real-hardware validation** needed to declare S03 **fully closed at product level**. This is a **closure validation issue**, not an implementation issue: no firmware/code changes; only running an acceptance checklist on real devices and capturing evidence.

**Context:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) states that S03 is not yet fully closed because two items remain: (1) real hardware validation, (2) BLE-prep / snapshot operations inventory. This issue covers **(1) only**. BLE-prep [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361) is a separate track after HW validation.

---

## Scope

- **In scope:** Real-device acceptance matrix and evidence for S03.1 closure; single master issue to coordinate bench runs and document pass/fail.
- **Anchors:** Corrected Product Truth and merged S03 execution ([#416](https://github.com/AlexanderTsarkov/naviga-app/issues/416), [#426](https://github.com/AlexanderTsarkov/naviga-app/issues/426), [current_state](docs/product/current_state.md)). Canon: [boot_pipeline_v0](docs/product/areas/firmware/policy/boot_pipeline_v0.md), [ootb_autonomous_start_v0](docs/product/areas/firmware/policy/ootb_autonomous_start_v0.md), [module_boot_config_v0](docs/product/areas/firmware/policy/module_boot_config_v0.md), [provisioning_baseline_v0](docs/product/areas/firmware/policy/provisioning_baseline_v0.md), [packet_truth_table_v02](docs/product/areas/nodetable/policy/packet_truth_table_v02.md), [packet_context_tx_rules_v0](docs/product/areas/radio/policy/packet_context_tx_rules_v0.md).

---

## Non-goals

- **No firmware/code changes** — validation only.
- **No canon / Product Truth changes** — checklist reflects existing canon.
- **No BLE scope** — no BLE snapshot fields, format, cadence, GATT, or transport; [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361) is a separate next track.
- **No reopening** of completed S03 execution issues (#417–#426, #435, #438, #443).
- **No S04/S05 scope** — no channel discovery, mesh/JOIN, or future roadmap items in this issue.
- **No long bench log dumps** — structured evidence and checklist only; logs/photos only where they support pass/fail.

---

## Real-hardware validation matrix

### Mandatory (closure evidence)

| # | Check | Pass condition | Canon / reference |
|---|--------|-----------------|-------------------|
| M1 | **Single-node fresh flash / first boot** | Device flashes, boots, and proceeds through setup without USB; no hard block. | [ootb_autonomous_start_v0](docs/product/areas/firmware/policy/ootb_autonomous_start_v0.md) §2.1; [#423](https://github.com/AlexanderTsarkov/naviga-app/issues/423). |
| M2 | **Power-on without USB** | Node powers on (e.g. battery or external supply), runs Phase A→B→C without waiting for Serial/USB. | [boot_pipeline_v0](docs/product/areas/firmware/policy/boot_pipeline_v0.md); [#423](https://github.com/AlexanderTsarkov/naviga-app/issues/423) (Serial block removed). |
| M3 | **Phase A/B/C observable boot path** | Serial or log shows Phase A result (Ok/Repaired/RepairFailed); Phase B role/profile resolved; Phase C starts cadence. Phase A result logged once; RepairFailed does not brick. | [boot_pipeline_v0](docs/product/areas/firmware/policy/boot_pipeline_v0.md) §7; [module_boot_config_v0](docs/product/areas/firmware/policy/module_boot_config_v0.md) §4; [#423](https://github.com/AlexanderTsarkov/naviga-app/issues/423). |
| M4 | **Alive / Node_Pos_Full / Node_Status within window** | Within expected window (e.g. maxSilence for Alive; first-fix bootstrap for first Node_Pos_Full; Node_Status per min_status_interval / T_status_max): at least Alive or Node_Pos_Full observed on TX; Node_Status observed when applicable. | [packet_truth_table_v02](docs/product/areas/nodetable/policy/packet_truth_table_v02.md), [packet_context_tx_rules_v0](docs/product/areas/radio/policy/packet_context_tx_rules_v0.md); [field_cadence_v0](docs/product/areas/nodetable/policy/field_cadence_v0.md) §2.1. |
| M5 | **Second-node RX confirmation** | With two nodes on same channel/rate: second node receives and accepts at least one v0.2 packet (Alive or Node_Pos_Full or Node_Status); NodeTable or RX counters show accepted RX. | [packet_migration_v01_v02](docs/product/areas/nodetable/policy/packet_migration_v01_v02.md); [#421](https://github.com/AlexanderTsarkov/naviga-app/issues/421), [#438](https://github.com/AlexanderTsarkov/naviga-app/issues/438). |
| M6 | **NodeTable behavior sanity** | Self entry and (when RX) remote entry show expected fields (e.g. node_id, seq16, position when available); no crash or assert on apply. | [NodeTable hub](docs/product/areas/nodetable/index.md), [rx_semantics_v0](docs/product/areas/nodetable/policy/rx_semantics_v0.md). |
| M7 | **Reboot / persistence sanity** | After reboot (power cycle or software reboot), node reuses persisted role/profile and seq16/snapshot where applicable; Phase B loads NVS; no regression to “first boot” every time. | [provisioning_baseline_v0](docs/product/areas/firmware/policy/provisioning_baseline_v0.md) §4; [#417](https://github.com/AlexanderTsarkov/naviga-app/issues/417), [#418](https://github.com/AlexanderTsarkov/naviga-app/issues/418). |
| M8 | **Traffic counters / observability** | Where serial or debug output is available: `M1Runtime::traffic_counters()` (or equivalent) shows TX sent and RX accepted by type (e.g. tx_sent_alive, tx_sent_pos_full, tx_sent_status; rx_ok_*); reset before run and read after tick. | [#425](https://github.com/AlexanderTsarkov/naviga-app/issues/425), [PR #445](https://github.com/AlexanderTsarkov/naviga-app/pull/445); [debug_playbook](docs/dev/debug_playbook.md) §D. |

### Optional (stretch validation)

| # | Check | Notes |
|---|--------|--------|
| O1 | **GNSS fix → first Node_Pos_Full** | Confirm first-fix bootstrap: first Node_Pos_Full sent at next opportunity without min-displacement gate. |
| O2 | **Node_Status lifecycle** | Confirm Node_Status bootstrap and T_status_max refresh (e.g. status sent within 300 s when no position path). |
| O3 | **RepairFailed path** | If radio or GNSS verify-and-repair fails, device sets observable fault state and continues (no brick); Phase C still starts. |

---

## Evidence required

- **Format:** One or more issue comments that fill a **checklist** (e.g. M1–M8, O1–O3) with **Pass / Fail / N/A** and a short note per row.
- **Artifacts (if useful):** Serial log excerpts, photos of setup, or links to `_working/hw_tests/` (or archive) with `notes.md` describing run and outcome. Keep minimal: what supports pass/fail, not full raw dumps.
- **Pass/fail/blocker:**
  - **Pass:** Condition met; evidence described or linked.
  - **Fail:** Condition not met; brief description and (if known) cause.
  - **Blocker:** Failure that blocks S03.1 closure until resolved (e.g. no TX without USB, or no RX on second node).
- **N/A:** Not applicable (e.g. single-node only run → M5 N/A; document when second node used).

---

## Exit criteria

- [ ] All **mandatory** checks (M1–M8) completed with Pass or documented N/A.
- [ ] Any **blocker** resolved or explicitly deferred with reason.
- [ ] Evidence (checklist + optional artifacts) recorded in this issue.
- [ ] Optional stretch (O1–O3) completed or explicitly skipped with one-line reason.
- [ ] [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) updated (e.g. comment) that HW validation track is this issue; BLE-prep [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361) remains separate.

---

## Links / repo pointers

| Item | Link |
|------|------|
| S03 status & remaining closure items | [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) |
| S03 execution umbrella (completed) | [#416](https://github.com/AlexanderTsarkov/naviga-app/issues/416) |
| Boot phases A/B/C + OOTB (legacy input) | [#423](https://github.com/AlexanderTsarkov/naviga-app/issues/423) |
| Traffic counters (runtime validation) | [PR #445](https://github.com/AlexanderTsarkov/naviga-app/pull/445) |
| current_state reconciled with S03 | [PR #446](https://github.com/AlexanderTsarkov/naviga-app/pull/446), [current_state](docs/product/current_state.md) |
| BLE gate (out of scope here) | [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361) |
| Bench checklist reference | [_working/423_boot_phases_contract_and_gaps.md](_working/423_boot_phases_contract_and_gaps.md) §6 |
| Debug / observability | [debug_playbook.md](docs/dev/debug_playbook.md) §D |

---

## Summary

- **One master issue** for S03.1 real-hardware closure validation.
- **No BLE, no S04/S05** in scope.
- **Checklist-driven:** mandatory M1–M8, optional O1–O3; evidence in comments + optional logs/photos.
- **Ready for execution:** a separate execution chat or assignee can run the bench from this matrix and report back.
