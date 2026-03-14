## Goal

Align the OLED status screen with the **current S03 runtime truth** so that live bench/debug on real nodes is easier. This is a small operational UI alignment task: make the display reflect the most useful S03 operational and runtime fields. No Product Truth or protocol semantics change.

## Why this matters now

S03 runtime and packet semantics are established. The current OLED layout (BT_MAC, FW, BLE, N, Uptime, S, T, R) is not aligned with the fields that matter most for day-to-day hardware validation and bench work (e.g. Short_ID/Node_Name, TX power, GNSS fix, role/profile, and Pos/Status TX/RX counters). Better OLED visibility materially helps real-node validation and follow-up work after #448/#449.

## Scope

**In scope:**

- Plumbing any missing runtime fields into the OLED status data path (e.g. into `OledStatusData` and the code that fills it in `AppServices::tick()`).
- Updating the OLED render/layout in `oled_status.cpp` to the target layout below.
- Using only compact labels (TX, N, Fix, MI, MD, MS, PosTx, StTx, PosRx, StRx) as specified.

**Out of scope (explicitly excluded):**

- Changing packet semantics, NodeTable semantics, or provisioning semantics.
- BLE or mobile display work (#361, S04, S05).
- Broader graphics/layout framework or generic UX redesign.
- Reopening Product Truth / canon debates.

## Non-goals

- Do not reopen Product Truth or canon debates.
- Do not change S03 semantics.
- Do not touch BLE / #361 / S04 / S05.
- Do not redesign the whole OLED system.
- Do not add new product features unrelated to the current status screen.
- Do not turn this into a generic UX/UI improvement umbrella.
- Do not mix with #447 bench evidence work; this issue is a separate small follow-up.

## Target OLED layout

Compact labels only; screen is small.

**Line 1 (large font for ID/name, normal for uptime):**

- `Short_ID` or `Node_Name` (if non-empty) + `UpTime`
- Example: `A7C2 | 01:23:45` or `MyNode | 01:23:45`
- Rule: if `Node_Name` is non-empty, show it; otherwise show `Short_ID`.

**Line 2:**

- `TX:<power> | N:<active_nodes> | Fix:<state>`
- Example: `TX:21 | N:2 | Fix:N` (or `Fix:2D`, `Fix:3D`)
- Fix state: compact, e.g. `N`, `2D`, `3D`.

**Line 3:**

- `<Role> | MI:<min_interval> | MD:<min_dist> | MS:<max_silence>`
- Example: `Dog | MI:15 | MD:20 | MS:60`
- Role: short name from effective role (e.g. Person, Dog, Infra).

**Line 4:**

- `PosTx:<count> | StTx:<count>`
- Example: `PosTx:100 | StTx:50`

**Line 5:**

- `PosRx:<count> | StRx:<count>`
- Example: `PosRx:100 | StRx:50`

Labels must remain as above (TX, N, Fix, MI, MD, MS, PosTx, StTx, PosRx, StRx); no long spell-outs.

## Expected field plumbing

| Field | Source / note |
|-------|----------------|
| `short_id` | Already available in app (e.g. `short_id_hex_`); use for Line 1 when no node_name. |
| `node_name` | Self node name; may need plumbing from NodeTable self entry or provisioning/storage if not yet exposed to OLED path. |
| `uptime` | Already in `OledStatusData.uptime_ms`; format as HH:MM:SS or MM:SS for compact. |
| `tx_power` | From active radio profile / preset (e.g. dBm); may need to be passed from radio or storage into OLED data. |
| Active node count | Already `runtime_.node_count()` → `nodes_seen`. |
| GNSS fix state | Available from runtime/app (e.g. from `set_self_position` / GNSS snapshot); plumb into OLED data and show as N / 2D / 3D. |
| Role, MI, MD, MS | Effective role and profile (min_interval_sec, min_displacement_m, max_silence_10s) are resolved in app init; pass current values into OLED data each tick (e.g. store in app or read from runtime if exposed). |
| PosTx, StTx, PosRx, StRx | `M1Runtime::traffic_counters()` already provides `tx_sent_pos_full`, `tx_sent_status`, `rx_ok_pos_full`, `rx_ok_status`; plumb into OLED data. |

Implementation may extend `OledStatusData` and the code that populates it in `app_services.cpp`; update `oled_status.cpp` render only for layout and compact labels.

## Exit criteria

- [ ] OLED shows `Short_ID` or `Node_Name` (when non-empty) plus uptime on line 1.
- [ ] OLED shows TX power, active node count (N), and GNSS fix state (Fix) on line 2.
- [ ] OLED shows effective role and profile with compact labels (Role, MI, MD, MS) on line 3.
- [ ] OLED shows compact Node_Pos_Full / Node_Status TX and RX counters (PosTx, StTx, PosRx, StRx) on lines 4–5.
- [ ] Any missing fields are plumbed into the OLED status data path; no protocol or NodeTable semantics changed.
- [ ] No BLE or Product Truth changes.
- [ ] Ready for one small follow-up PR.

## Repo pointers / related issues

- **OLED:** `firmware/src/services/oled_status.h`, `oled_status.cpp` — current layout and `OledStatusData`.
- **Data source:** `firmware/src/app/app_services.cpp` (where `OledStatusData` is filled and `oled_.update()` is called).
- **Counters:** `firmware/src/domain/traffic_counters.h`; `M1Runtime::traffic_counters()`.
- **Role/profile:** `app_services.cpp` init (effective role, `RoleProfileRecord`); runtime holds cadence state.
- **#447** — operational / HW bench context only; this issue does not block #447 unless there is a concrete dependency.
- **#448, #449** — resolved boot blocker; OLED alignment is a separate small follow-up.
