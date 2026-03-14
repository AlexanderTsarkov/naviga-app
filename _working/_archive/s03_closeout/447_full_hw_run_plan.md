# #447 Full real-hardware validation run — execution plan

**Master issue:** #447 (S03.1 HW validation)  
**Authority:** #351 (HW validation required before S03 closure); #448/#449 resolved.  
**Run type:** Authoritative full run — no skipped mandatory checks, serial primary evidence.

---

## 1) Bench setup (from #447 / artifacts/hw447)

| Item | Value |
|------|--------|
| **Node A** | `/dev/cu.wchusbserial5B3D0112491` (serial 5B3D011249) — **Person / Human** (role 0) |
| **Node B** | `/dev/cu.wchusbserial5B3D0164361` (serial 5B3D016436) — **Dog** (role 1) |
| **Board** | devkit_e22_oled_gnss (E22 433 MHz, GNSS, OLED) |
| **Baud** | 115200 |
| **Profile (canon)** | Person: minIntervalSec=18, minDisplacementM=25, maxSilence10s=9; Dog: 9/15/3 |

**Evidence:** Serial logs primary; OLED supplemental only.

---

## 2) M1→M8 execution checklist (order and intent)

| Step | Check | Concrete action | Evidence | User physical action? |
|------|--------|------------------|----------|------------------------|
| **M1** | Single-node fresh flash / first boot | Flash **one** node (e.g. Node A) with current main firmware; capture **first boot** serial log from power-on. Confirm device boots and proceeds through setup (no hard block). | Serial log: boot banner, Phase A/B/C or equivalent, no crash/loop. | Yes: connect only Node A for flash; after flash, observe first boot (USB connected for log). |
| **M2** | Power-on without USB | On **one** node: disconnect USB, power cycle (or external power); wait ≥30 s; connect USB and run `status`. Confirm node already ran Phase A→B→C (no Serial block). | Serial after reconnect: `status` shows role_cur, radio_boot=OK, no “waiting for Serial”. | Yes: disconnect USB, power cycle, wait, reconnect USB. |
| **M3** | Phase A/B/C observable boot path | From any full boot log: Phase A result (Ok/Repaired/RepairFailed); Phase B role/profile resolved; Phase C cadence started. | Serial: boot lines with Phase A result, Phase B role/profile, radio_boot in status. | No (capture during M1 or post-M2 reconnect). |
| **M4** | Alive / Node_Pos_Full / Node_Status within window | Both nodes: `gnss fix` + `gnss move` (override). Enable `debug on`. Collect serial for ~45 s. Require ≥1 TX of Alive or Node_Pos_Full or Node_Status per node within window. | Serial: `pkt tx` lines with type=CORE|POS_FULL|ALIVE|STATUS (or equivalent). | No. |
| **M5** | Second-node RX confirmation | Two nodes on same channel/rate. Collect `pkt rx` on both for ~50 s. Each node must receive ≥1 v0.2 packet from the other. | Serial: `pkt rx` lines; NodeTable size=2 or peer info. | No (both nodes USB for serial). |
| **M6** | NodeTable behavior sanity | After M5: self + remote entries; expected fields (node_id, seq16, position when available); no crash on apply. | Serial: `nodetable: size=2`, peer lines; no crash/assert. | No. |
| **M7** | Reboot / persistence sanity | Reboot one node (e.g. `reboot` or power cycle). After boot: `status` + `get profile`. Role and profile must be persisted (e.g. role_cur=0, interval_s=18 for Person). | Serial: status + get profile after reboot match provisioned role/profile. | Optional: power cycle for stronger test. |
| **M8** | Traffic counters / observability | No serial command for `traffic_counters()` in current firmware. Evidence: `debug on` + count **pkt tx** and **pkt rx** lines in logs; optionally note tx_sent_* / rx_ok_* in code (PR #445) but not exposed over shell. | Serial: presence of pkt tx/rx lines; explicit count of tx vs rx lines per node. | No. |

**GNSS override:** Used for M4 (and O1) to deterministically get fix and movement so Node_Pos_Full/CORE can be observed.  
**Profile verification:** After provisioning, confirm via `status` and `get profile` (interval_s, silence_10s, dist_m) for Node A (Person) and Node B (Dog).

---

## 3) Optional O1–O3

| Step | Check | Action | If skipped |
|------|--------|--------|------------|
| **O1** | GNSS fix → first Node_Pos_Full | From M4 log: first-fix bootstrap — first Node_Pos_Full/CORE at next opportunity after gnss fix+move. | Mark “Skipped” with reason (e.g. time-bound run). |
| **O2** | Node_Status lifecycle | Confirm Node_Status sent within 300 s when no position path; bootstrap max 2. | Mark “Skipped” with reason. |
| **O3** | RepairFailed path | If radio/GNSS verify-and-repair fails, device sets observable fault and continues (no brick). | N/A if both boot Ok/Repaired. |

---

## 4) Pre-run verification

- [ ] Both nodes on current `main` firmware (build from repo; confirm commit / env devkit_e22_oled_gnss).
- [ ] Both nodes connected via USB; serial ports confirmed (Node A, Node B).
- [ ] Provisioning: Node A = Person (role 0), Node B = Dog (role 1); verify with `status` and `get profile`.

---

## 5) Execution flow (high level)

1. Confirm repo at `main`, build firmware for `devkit_e22_oled_gnss`.
2. **M1:** Flash Node A only → first boot → capture full boot log.
3. **M2:** Node A (or B): disconnect USB, power-on without USB, wait, reconnect → run `status` → evidence.
4. **M3:** Extract Phase A/B/C from boot log (M1 or post-M2).
5. Connect both nodes; provision roles (set role 0 / set role 1); reboot both; verify profiles.
6. **M4:** gnss fix + move on both; debug on; collect pkt tx for ~45 s.
7. **M5:** Collect pkt rx on both for ~50 s.
8. **M6:** Confirm NodeTable size=2, peer info, no crash.
9. **M7:** Reboot one node; status + get profile; confirm persistence.
10. **M8:** From logs, count pkt tx / pkt rx; record as evidence (no serial counters command).
11. **O1–O3:** Run if practical; else mark Skipped/N/A with reason.
12. Post structured checklist comment to #447; state verdict and #351 readiness.

---

## 6) Artifacts

- Raw serial logs: e.g. `artifacts/hw447/run_YYYYMMDD/nodeA_447.log`, `nodeB_447.log`.
- Evidence summary: checklist with Pass/Fail/N/A per item; short notes; overall verdict.
- #447 comment: bench setup, Node A/B identity and profiles, M1–M8 and O1–O3 table, supplemental OLED (if any), closure-ready yes/no, blockers, next action.
