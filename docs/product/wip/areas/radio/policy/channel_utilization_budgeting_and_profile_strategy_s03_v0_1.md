# Radio — Channel utilization budgeting & radio profile strategy (S03 v0.1)

**Status:** WIP. Planning artifact for future continuation. **Epic:** [#403](https://github.com/AlexanderTsarkov/naviga-app/issues/403) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351).

This doc records the **budgeting concept**, **profile matrix** (Long/Standard/Fast), **merge/split strategy** by traffic category, **throttling** linkage, **rate_tier** strategy, **mesh forwarding** modes (including adaptive piggyback-or-standalone), and **TBD/calibration** items. Single source of truth for resuming the channel-utilization and profile-strategy track. No code; no BLE/UI; no Session Mesh design (referenced as future track).

**Inputs:** [traffic_model_s03_v0_1](traffic_model_s03_v0_1.md) ([#360](https://github.com/AlexanderTsarkov/naviga-app/issues/360)), [gnss_tail_completeness_and_budgets_s03](../../nodetable/contract/gnss_tail_completeness_and_budgets_s03.md) ([#359](https://github.com/AlexanderTsarkov/naviga-app/issues/359)), [packet_to_nodetable_mapping_s03](../../nodetable/policy/packet_to_nodetable_mapping_s03.md) ([#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358)), [radio_user_settings_review_s03](radio_user_settings_review_s03.md) ([#356](https://github.com/AlexanderTsarkov/naviga-app/issues/356)), [radio_profiles_model_s03](radio_profiles_model_s03.md), [e220_radio_profile_mapping_s03](e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)), [tx_power_contract_s03](tx_power_contract_s03.md) ([#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384)).

---

## 1) Purpose, scope, non-goals

- **Purpose:** Capture agreed concepts and open decisions for **channel utilization budgeting** and **radio profile strategy** so work can resume later without losing context. Support decisions: U targets, profile matrix, merge/split rules, throttling, rate_tier, mesh forwarding, user-generated scheduling.
- **Scope:** Budgeting (U_target, risk bands); profile matrix (Long/Standard/Fast); merge/split by category; throttling policy; adaptive decision (merge vs throttle vs faster tier); mesh piggyback hybrid; role cadence examples; calibration TBD list.
- **Non-goals:** No firmware changes; no BLE/UI protocol; no Session Mesh full design (reference only); no LBT/CAD; no calibration of link thresholds in this doc (TBD).

---

## 2) Definitions

| Term | Meaning |
|------|---------|
| **U** | Channel utilization: Σ (rate_i × airtime_i); dimensionless fraction of time the channel is occupied by the modeled packets. |
| **U_target** | Default target utilization; **0.30** (guidance). Keep U ≤ U_target under realistic scenarios. |
| **Risk bands (guidance)** | Green U ≤ 0.15; Yellow 0.15 &lt; U ≤ 0.30; Red U &gt; 0.30–0.40. Without LBT/scheduling, losses grow sharply above ~0.30–0.40. |
| **LinkQuality** | Composite from RSSI/SNR (UART vs SPI) + RX success rate. Used to choose merge vs throttle vs faster tier. **Thresholds TBD/calibrate later.** |
| **Packetization mode** | **Split:** Core_Pos + separate Core_Tail (current S03). **Merged:** Core_Pos + full Pos_Quality in one packet (PosFull). **Auto:** decision by U + LinkQuality (guidance). |

---

## 3) Profile matrix (Long / Standard / Fast)

| Profile | Intended node density | PHY / rate_tier intent | Packetization default | Mesh mode (public) |
|---------|------------------------|-------------------------|------------------------|--------------------|
| **Long** | ≤5 nodes | LONG (slower rate; max range). Mapping TBD for SPI; E220 clamps LONG → STANDARD. | Split acceptable; low U. | Hops ≤1; session mesh referenced as future. |
| **Standard** | ~10–20 nodes | STANDARD (e.g. 2.4 kbps E220). OOTB default. | Split or merged by policy; throttle when U high. | Hops ≤3; attach_window policy. |
| **Fast** | 20–30 nodes | FAST (e.g. 4.8 kbps E220); shorter airtime, lower margin. | Prefer merged + throttle to keep U ≤ target. | Hops ≤3; tighter attach_window. |

- **Mesh constraints:** Public mesh limits (hop count, payload budget); **Session mesh** is a dependent future track (minimized payload: id1, covered_mask8, pos4 from anchor); not designed here.
- **Packetization defaults** are profile-level hints; adaptive policy (§6) can override based on U and LinkQuality.

---

## 4) Merge vs split by traffic category (normative rules)

| Category | Rule | Notes |
|----------|------|--------|
| **Position** | **Split** (Core_Pos + Core_Tail) vs **PosFull** (Core_Pos + full Pos_Quality ~4B in one packet). No “partial quality” mode. Choice by adaptive policy (§6). | Tail carries pos_fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small; merge reduces per-update overhead. |
| **Ops/Info** | Merge into **OpsInfo** where appropriate; **P3** skip/expiry under load. Never upgrade Ops/Info above P3 for position-bearing traffic. | See [tx_priority_and_arbitration_v0_1](../../nodetable/policy/tx_priority_and_arbitration_v0_1.md); P3 drops first when U high. |
| **Mesh forwarding** | **Standalone** (dedicated mesh packet) vs **piggyback/attach** (attach to next self TX of same class within attach_window). **Never mix classes/priorities** in one payload. | §7 for attach_window policy. |
| **User-generated** | Always **separate stream**; burst cycles with **round-robin slots** vs P3. No piggyback of user traffic onto Position/Ops/Info. | Scheduling policy TBD in epic sub-issue. |
| **Session** | Minimized payload (id1, covered_mask8, pos4 from anchor) as **future track**; referenced only. | Not designed in S03. |

---

## 5) Throttling policy (normative)

- **Scope:** Affects **min_interval_eff** only; **max_silence** unchanged (liveness bound preserved).
- **Step rule:** +10% of (max_silence − min_interval) per throttle step k. Example: if max_silence=20s, min_interval=4s, step = 10% × 16s ≈ 1.6s per k.
- **Example thresholds (guidance):** U_high ~0.25 (start throttling); U_low ~0.18 (relax). Exact values **TBD/calibrate later**.
- **Propagation note:** If throttle or “reduce Tail” signal is carried in **low-priority (P3)** packets, it may **propagate slowly** under overload (those packets are dropped first). Document as **risk/consideration** for implementation.

---

## 6) Adaptive decision policy (guidance)

- **If U high + link good:** Prefer **merge first** (reduce per-update overhead; single packet still reliable).
- **If U high + link edge:** Prefer **throttle first** (keep Core-only; smaller packets; merge when link improves).
- **Faster tier** as 3rd lever when link good (shorten airtime; trade link margin).
- **Thresholds** (U_high, U_low, LinkQuality bands) **TBD/calibrate later**. LinkQuality inputs: RSSI (UART) or SNR (SPI) + RX success rate.

---

## 7) Mesh piggyback hybrid (normative / guidance split)

- **Rule (normative):** After RX of a mesh-forwardable packet, if **time_to_next_self_tx_of_same_class ≤ attach_window** → **attach** payload to that TX; else send **standalone** mesh packet.
- **attach_window** adaptive by U/throttle: e.g. 1s → 2s → 3s; clamp per profile. **TTL** = attach_window; limit to **1** attached payload per self TX; **size budget** guardrail to avoid oversizing.
- **Jitter** retained for standalone mesh TX (do not collapse jitter when piggybacking).
- **Same class:** Only attach mesh Position to next self Position TX; mesh Ops/Info to next self Ops/Info TX. Never mix classes/priorities.

---

## 8) Role cadence bundles (examples only)

*Label: examples for illustration; not normative.*

| Bundle | Typical use | Dog | Human | Shooter | Driver |
|--------|-------------|-----|-------|---------|--------|
| **Conservative** | Low density, max range | min_int 8s, max_sil 30s | min_int 15s, max_sil 60s | min_int 40s, max_sil 200s | — |
| **Average (OOTB)** | Default | min_int 4s, max_sil 20s | min_int 8s, max_sil 40s | min_int 35s, max_sil 200s | — |
| **Aggressive (Session/Hunting)** | High update rate | min_int 2s, max_sil 15s | min_int 4s, max_sil 20s | min_int 20s, max_sil 120s | — |

Exact values and role IDs from [role_profiles_policy_v0](../../../../areas/domain/policy/role_profiles_policy_v0.md) and [node_role_type_review_s03](../../domain/policy/node_role_type_review_s03.md); these rows are illustrative only.

---

## 9) Links to existing artifacts

| Artifact | Description |
|----------|-------------|
| [#403](https://github.com/AlexanderTsarkov/naviga-app/issues/403) | Epic: Channel utilization budgeting & radio profile strategy. |
| [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) | S03 planning master umbrella. |
| [traffic_model_s03_v0_1](traffic_model_s03_v0_1.md) ([#360](https://github.com/AlexanderTsarkov/naviga-app/issues/360)) | U calculations, risk zones, levers, control outline. |
| [gnss_tail_completeness_and_budgets_s03](../../nodetable/contract/gnss_tail_completeness_and_budgets_s03.md) ([#359](https://github.com/AlexanderTsarkov/naviga-app/issues/359)) | Pos quality containers, Tail completeness. |
| [packet_to_nodetable_mapping_s03](../../nodetable/policy/packet_to_nodetable_mapping_s03.md) ([#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358)) | Packet → NodeTable mapping. |
| [radio_user_settings_review_s03](radio_user_settings_review_s03.md) ([#356](https://github.com/AlexanderTsarkov/naviga-app/issues/356)) | Radio settings in product terms. |
| [radio_profiles_model_s03](radio_profiles_model_s03.md) | rate_tier, profile model, NVS schema. |
| [e220_radio_profile_mapping_s03](e220_radio_profile_mapping_s03.md) ([#383](https://github.com/AlexanderTsarkov/naviga-app/issues/383)) | E220 mapping. |
| [tx_power_contract_s03](tx_power_contract_s03.md) ([#384](https://github.com/AlexanderTsarkov/naviga-app/issues/384)) | TX power baseline vs runtime. |
| [tx_priority_and_arbitration_v0_1](../../nodetable/policy/tx_priority_and_arbitration_v0_1.md) | P0–P3, throttling, expiry. |

---

## 10) Open questions / calibration TBD list

- **RSSI/SNR thresholds** for LinkQuality bands (good / edge / bad): TBD; calibrate later with field data.
- **Airtime tables per backend:** E220 UART (nominal 2.4/4.8 kbps) vs future SPI (SF/BW/CR). Exact ms-per-packet tables TBD per backend; [traffic_model_s03_v0_1](traffic_model_s03_v0_1.md) uses baseline assumptions ±10–15%.
- **U_high / U_low** numeric values: example 0.25 / 0.18 as guidance; calibrate with tests.
- **attach_window** clamp bounds per profile: TBD (e.g. 1–3 s range).
- **Field test plan** for U and link-quality calibration: TBD; to be defined in epic sub-issue (calibration list).
