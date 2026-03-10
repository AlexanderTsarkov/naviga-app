# Radio — Traffic model v0

**Status:** Canon (policy).  
**Issue:** [#360](https://github.com/AlexanderTsarkov/naviga-app/issues/360) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351).

This doc records **preliminary traffic-model calculations** and product conclusions: estimated channel utilization (U) for representative scenarios, why the Core_Pos + Tail split can overload the channel, comparison of mitigation levers, and a control-policy outline. It also states **channel and frame policy (v0)**: which packet classes are allowed on OOTB public vs Session/Group, delivery expectations, and frame limits. Semantic truth is this policy; channel modes and frame limits are defined here, not by UI or product copy.

---

## 1) Scope and what this model is

- **Scope:** Single shared channel; no LBT/scheduling; OOTB packet set (Core_Pos, Alive, Core_Tail, Operational, Informative). Human and dog role cadences; representative "hunt" and scaled 5/10/20 node scenarios.
- **What this is:** Preliminary, **assumption-driven** utilization estimate. Airtimes and motion probabilities are chosen for illustration; real airtime depends on modem/bitrate (e.g. E220 2.4 kbps). **Purpose:** show relative impact of split vs merged vs throttling vs rate tier and support lever choice (U + link quality). **Not:** precise calibration; RSSI/SNR thresholds (TBD/calibrate later); JOIN/Mesh/CAD/LBT.
- **Containers unchanged:** Core vs Tail placement is fixed per S03; "merge" is analyzed as a **future optimization option** only.

---

## 2) Inputs (explicit assumptions)

### 2.1 Packet airtimes (baseline)

All values **assumed** for a nominal LoRa-like profile (e.g. ~2.4 kbps effective). **±10–15%** expected in practice; document as assumption.

| Packet | Payload (bytes) | Airtime (ms) | Note |
|--------|------------------|--------------|------|
| Node_Core_Pos | 17 | 198 | Core only; no Tail. |
| Node_I_Am_Alive | 12 | 177 | When no fix. |
| Node_Core_Tail | 15 | 198 | After Core_Pos; ref_core_seq16 + pos_quality. |
| Node_Operational | 15 | 198 | Battery, uptime, etc. |
| Node_Informative | 15 | 198 | maxSilence, hw/fw, etc. |

**Merged (future option):** Core_Pos + Tail content in one packet: **18–21 B ≈ 218 ms** (single update; combined overhead). Used only for comparison; not a format change in S03.

### 2.2 Motion model (approximation)

- **p_move** = probability that the **min_displacement** condition triggers within **min_interval** (node "moves enough" to send another Core_Pos).
- **Core_Pos rate per node** (packets per second):
  - **r_pos = p_move / min_interval + (1 − p_move) / max_silence**

Interpretation: when moving, node sends at cadence ~1/min_interval; when not, at ~1/max_silence. **Assumption:** p_move is scenario-dependent (e.g. dogs 0.9, beaters 0.7, shooters 0.3).

### 2.3 Tail send policy (scenarios)

- **Split mode (current S03):** One Core_Tail per Core_Pos when position valid. **p_tail = 1.0** (baseline). Comparison: **p_tail = 0.5** or **0.25** (e.g. send Tail every 2nd or 4th Core) as reduced-Tail option.
- **Merged mode (future):** One combined packet per position update (Core + Tail content); no separate Tail packet. Used only in §4 for U comparison.

### 2.4 Operational / Informative cadence (assumption)

- **Operational:** one packet per node every **60 s** (assumption; on-change + min_interval in practice). Rate = N/60 per second.
- **Informative:** one packet per node every **300 s** (assumption; on-change + periodic). Rate = N/300 per second.

Label explicitly as **assumption**; real cadence from [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) and role/profile may differ.

### 2.5 Alive (no-fix)

- When a node has no GNSS fix, it sends **Alive** instead of Core_Pos. For **"hunt" and "dogs-only"** scenarios below we assume **all nodes have fix** so Alive contribution is **negligible**. If needed, add Alive rate = (fraction without fix) × (1/max_silence) × N; not included in tables for simplicity.

---

## 3) Scenario set

### 3.1 Scenario A — "Hunt heavy load" (user-provided)

| Role | Count N | min_interval (s) | max_silence (s) | p_move | r_pos per node (1/s) | N × r_pos (1/s) |
|------|---------|-------------------|------------------|--------|----------------------|------------------|
| Dogs | 5 | 4 | 20 | 0.9 | 0.9/4 + 0.1/20 = 0.23 | 1.15 |
| Beaters | 4 | 8 | 40 | 0.7 | 0.7/8 + 0.3/40 = 0.095 | 0.38 |
| Shooters | 16 | 35 | 200 | 0.3 | 0.3/35 + 0.7/200 ≈ 0.0121 | 0.193 |
| **Total** | **25** | — | — | — | — | **1.723** |

**Assumption:** min_displacement and other role params (e.g. min_dist) are reflected in the chosen p_move; exact mapping is scenario-specific.

### 3.2 Scenario B — 5 / 10 / 20 nodes (dogs-only, same params)

Scale **dogs only** with same cadence as in Scenario A (min_interval=4 s, max_silence=20 s, p_move=0.9).

| N (dogs) | r_pos per node | Total pos rate (1/s) |
|----------|-----------------|----------------------|
| 5 | 0.23 | 1.15 |
| 10 | 0.23 | 2.30 |
| 20 | 0.23 | 4.60 |

---

## 4) Calculations — utilization U

**Definition:** U = Σ (rate_i × airtime_i), dimensionless (fraction of time the channel is occupied by these packets). Same formula for each packet type; then sum.

### 4.1 Hunt (25 nodes: 5 dogs + 4 beaters + 16 shooters)

**Position packets (Core_Pos + Tail):**

- Total position rate **R_pos = 1.723**/s.
- **Split (p_tail = 1.0):** U_core = 1.723 × 0.198 = **0.341**; U_tail = 1.723 × 0.198 = **0.341** → **U_pos = 0.682**.
- **Split (p_tail = 0.5):** U_core = 0.341; U_tail = 0.5 × 0.341 = 0.1705 → **U_pos = 0.51**.
- **Merged:** one packet per update, 218 ms → U_pos = 1.723 × 0.218 = **0.376**.

**Operational + Informative (assumption: 60 s Op, 300 s Info):**

- Op: 25/60 × 0.198 = **0.0825**; Info: 25/300 × 0.198 = **0.0165** → **U_op+info = 0.099**.

| Hunt scenario | U_pos (split p_tail=1) | U_pos (split p_tail=0.5) | U_pos (merged) | U_op+info | **U_total (split 1.0)** | **U_total (split 0.5)** | **U_total (merged)** |
|---------------|------------------------|---------------------------|----------------|-----------|--------------------------|-------------------------|----------------------|
| 25 nodes | 0.682 | 0.51 | 0.376 | 0.099 | **0.78** | **0.61** | **0.475** |

**Conclusion (hunt):** Split with p_tail=1 yields **U ≈ 0.78** (far above safe range). Split 0.5 still **0.61**. Merged brings position load to **0.376**; total **0.475** (still high but lower). Per-packet overhead of two packets (Core + Tail) is the main driver.

### 4.2 Dogs-only (5 / 10 / 20 nodes)

Same motion model (r_pos = 0.23/s per node). Op+Info: N/60 × 0.198 + N/300 × 0.198.

| Scenario | N | R_pos (1/s) | U_pos split (1.0) | U_pos split (0.5) | U_pos merged | U_op+info | **U_total split 1.0** | **U_total merged** |
|----------|---|-------------|-------------------|-------------------|--------------|-----------|------------------------|---------------------|
| 5 dogs | 5 | 1.15 | 0.455 | 0.34 | 0.251 | 0.020 | **0.48** | **0.27** |
| 10 dogs | 10 | 2.30 | 0.91 | 0.68 | 0.50 | 0.04 | **0.95** | **0.54** |
| 20 dogs | 20 | 4.60 | 1.82 | 1.37 | 1.00 | 0.08 | **1.90** | **1.08** |

**Conclusion (dogs-only):** Even **5 dogs** with split (p_tail=1) gives **U ≈ 0.48** (position alone 0.46). **10/20 dogs** exceed 0.9 and 1.9 — channel saturated. Merged halves position load; total U still exceeds 1.0 for 20 dogs without other levers.

---

## 5) Interpretation — risk zones (engineering guidance)

- **Green (U ≤ 0.15):** Low load; losses and jitter expected to be modest. **Guidance only**; no guarantee without LBT/scheduling.
- **Yellow (0.15 < U ≤ 0.30):** Moderate load; losses and delay may increase; acceptable for best-effort presence/map.
- **Red (U > 0.30–0.40):** High load; **without LBT/scheduling, losses grow sharply**; P3 (Operational/Informative) will be starved; Tail loss rate high. **> 0.40** is critical.

**Note:** Thresholds are **engineering guidance** for lever choice and bench planning; not normative product guarantees. Exact values depend on modem, environment, and collision model; **TBD/calibrate later**.

**Why split overloads:** Each position update sends **two** packets (Core_Pos + Core_Tail). Airtime doubles for the same logical update; U_pos scales with 2 × rate × airtime_core. Merge removes the second packet and reduces per-update overhead.

---

## 6) Mitigation levers and trade-offs

| Lever | Effect | Trade-off / note |
|-------|--------|-------------------|
| **1) P3 expiry/priority** | Under load, P3 (Operational, Informative) are dropped first; Core_Pos and Alive preserved. | Local fairness; presence/map stay working; battery/uptime/config updates may lag or be lost. See [tx_priority_and_arbitration_v0](../../nodetable/policy/tx_priority_and_arbitration_v0.md). |
| **2) Throttling (min_interval_eff only)** | Increase effective min_interval (e.g. step = 10% of (max_silence − min_interval)); max_silence unchanged. | Lowers Core_Pos rate and thus U; position updates less frequent; liveness still bounded by max_silence. |
| **3) Packet merge (Core_Pos + Tail)** | One packet per position update; Tail content in same packet. | Reduces per-update overhead (see §4); **hurts edge-of-link robustness** (larger packet more likely to fail; no "Core-only" fallback for that update). |
| **4) Faster radio profile (rate_tier / SF8 concept)** | Shorter airtime per byte (e.g. higher rate or lower SF). | Lowers U for same packet count; **reduces link margin**; range/sensitivity trade-off. See WIP [radio_profiles_model_s03](../../../wip/areas/radio/policy/radio_profiles_model_s03.md), [e220_radio_profile_mapping_s03](../../../wip/areas/radio/policy/e220_radio_profile_mapping_s03.md). |

**Choice:** Combine levers based on **U** and **link quality** (see §7). No single lever is sufficient for all scenarios; merge + throttle + P3 drop together keep U in check while preserving presence.

---

## 7) Control policy outline (future; not implemented)

- **If U > U_high** (e.g. U_high in yellow/red band; **threshold TBD/calibrate**): choose **merge** vs **throttle** based on link quality:
  - **Link good** (e.g. high SNR/RSSI, high RX success): prefer **merge first** (reduce overhead; single packet still reliable).
  - **Link edge** (low margin, high loss): prefer **throttle first** (keep Core-only; smaller packets; merge later when link improves).
- **Link quality inputs:** SNR (SPI) or RSSI (UART) + RX success rate. **Thresholds TBD/calibrate later**; not defined in this doc.
- **Risk:** If throttle or "reduce Tail" signal is carried in **low-priority** packets (e.g. Informative), it may **propagate slowly** under overload (those packets are dropped first). Document as **consideration** for future protocol/implementation.

---

## 8) Outputs and next steps

- **Baseline for tuning:** This model provides the **baseline** for later calibration (airtime, p_move, cadence) and for deciding **merge / throttle / rate-tier** policies. [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) and [#360](https://github.com/AlexanderTsarkov/naviga-app/issues/360) are the planning anchors.
- **Implementation tickets:** Future work may add: (a) throttle step rule and U estimate in FW, (b) link-quality inputs and thresholds, (c) optional merge mode as a profile or runtime choice. **This doc does not create code issues**; it only states implications.

---

## 9) Channel and frame policy (v0)

**OOTB public channel:** "Out of the box" public channel — default channel mode before the user selects a non-public or session/group context. Policy rule: tracker + "who is around" only; no user comms on public by default.

- **Allowed:** **BeaconCore** (periodic; **MUST** be delivered); **BeaconTail** (periodic but rare; **MAY** be lost, best-effort).
- **Disallowed:** UserMessage, GeoObject, GeoObjectActivation. If the user wants messaging or geo/comms, they **MUST** leave the public channel by configuration.

**Session/Group channel:** User has selected a **non-public** channel **AND** there is at least a **presumed recipient**. BeaconCore/BeaconTail same as above; UserMessage/GeoObject/GeoObjectActivation as **non-periodic**, **best-effort**, **loss-tolerant**.

**Frame limits (v0, no fragmentation):**

- **ProductMaxFrameBytes v0 = 96**. Any payload that would exceed **96 bytes** (after any product-level framing) **MUST** be rejected or compressed/trimmed to fit. **Fragmentation is NOT supported in v0.** A single logical payload **MUST** fit in one frame.
- **RadioMaxFrameBytes** is a separate **HW/profile** parameter. **Invariant:** **ProductMaxFrameBytes MUST NOT exceed RadioMaxFrameBytes** for the current profile (validated at build-time or runtime).

---

## 10) Cross-links

| Doc / issue | Description |
|-------------|-------------|
| [#360](https://github.com/AlexanderTsarkov/naviga-app/issues/360) | This task (traffic model 5/10/20, human & dog). |
| [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) | S03 umbrella planning. |
| [packet_to_nodetable_mapping_v0](../../nodetable/policy/packet_to_nodetable_mapping_v0.md) | Packet types and field mapping. |
| [gnss_tail_completeness_and_budgets_s03](../../../wip/areas/nodetable/contract/gnss_tail_completeness_and_budgets_s03.md) (WIP) | Tail semantics; Core/Tail split fixed. |
| [tx_priority_and_arbitration_v0](../../nodetable/policy/tx_priority_and_arbitration_v0.md) | P0–P3; P3 drops under load. |
| [packet_sets_v0](../../nodetable/policy/packet_sets_v0.md) | Packet sets and eligibility. |
| [field_cadence_v0](../../nodetable/policy/field_cadence_v0.md) | Cadence by field/packet. |
