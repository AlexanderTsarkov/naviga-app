# PR #437 — v0.2 implementation risk audit

**Purpose:** Review-and-tighten pass before merge. Check implementation against live canon (#419, #436, packet_context_tx_rules_v0). Fix only real canon violations or dangerous ambiguities.

**Canon sources:** NodeTable master field table (#419), packet_truth_table_v02.md, packet_context_tx_rules_v0.md, packet_sets_v0.md.

---

## 1) Risk table

| # | Item | Current implementation | Canon expectation | Status | Fix needed? |
|---|------|------------------------|-------------------|--------|--------------|
| 1 | **Pos_Quality → NodeTable** | Pos_Quality decoded to fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small; packed into existing `pos_flags` (pos_flags_small 4b, fix_type 3b, pos_accuracy_bucket 1b) and `sats` (pos_sats 6b, pos_accuracy_bucket 2b). No new NodeEntry fields. | Master table: pos_fix_type/pos_accuracy_bucket "Yes (stub)", pos_sats "Yes (sats)", pos_flags_small "Yes (pos_flags)". Normalized product fields; no opaque radio blob. | **Aligned** | No code change. **Tighten:** Document packing in comment; add test assertions for packed byte values. |
| 2 | **uptime_10m canon vs storage** | Wire `uptime10m` in Status; apply_status stores `uptime_sec = uptime10m * 600`. NodeEntry comment: "canon name uptime_10m". | "uptime_10m at product/canon layer; wire may use uptime10m"; stored as uptime_sec; product in 10m units. | **Aligned** | No. |
| 3 | **hw_profile_id / fw_version_id width** | StatusFields/NodeEntry uint16; encode/decode use write_u16_le/read_u16_le; tests assert UINT16. | Remain uint16. | **Aligned** | No. |
| 4 | **v0.2 TX-only** | update_tx_queue and build_tx only form PosFull (0x06), Alive (0x02), Status (0x07). No Core/Tail/Op/Info TX. | TX sends v0.2 only. | **Aligned** | No. |
| 5 | **RX v0.1+v0.2 compat** | on_rx dispatches 0x01–0x05 to legacy apply; 0x06→apply_pos_full; 0x07→apply_status; 0x02→Alive. Compat RX-only. | RX accepts v0.1 + v0.2; compat RX-only. | **Aligned** | No. |
| 6 | **Old Tail/Core not canonical** | last_core_seq16/has_core_seq16 in NodeEntry; §7 master table: "Runtime-local only; NOT BLE; NOT persisted". apply_pos_full sets last_core_seq16 (v0.2 single-packet path). v0.1 Core apply still sets it. | last_core_seq16 only on Node_Pos_Full (or compat Core_Pos). Not product truth. | **Aligned** | No. |
| 7 | **Node_Status lifecycle** | status_eligible = !pos_or_alive_enqueued && … (no hitchhiking). status_bootstrap_count_ < 2; min_status_interval_ms; T_status_max_ms. on_status_sent increments bootstrap. M1Runtime: 30s, 300s. | Bootstrap max 2; min_status_interval_ms=30s; T_status_max=300s; no hitchhiking. | **Aligned** | No. |
| 8 | **product-vs-wire boundary** | Codec: wire names (uptime10m). Apply: same; NodeEntry: uptime_sec. Clear. | Wire vs canon naming documented. | **Aligned** | No. |
| 9 | **Tests coverage** | PosFull RX: has_pos_flags, has_sats asserted; **not** packed pos_flags/sats values. Status RX: battery, uptime_sec, hw/fw UINT16 asserted. No explicit "no hitchhiking" test (implicit via status_eligible). | Tests prove v0.2 apply and compat; width preservation; lifecycle. | **Tighten** | Add Pos_Quality packed-value assertions in test_rx_pos_full_applies_position_and_quality. |

---

## 2) Findings

### Pos_Quality → NodeTable mapping

- **Decode:** pos_full_codec unpacks Pos_Quality 2 B LE into fix_type (3b), pos_sats (6b), pos_accuracy_bucket (3b), pos_flags_small (4b). Correct per packet_truth_table_v02.
- **Apply:** apply_pos_full writes canonical semantics into existing storage:
  - `pos_flags`: bits [0:3] = pos_flags_small, [4:6] = fix_type, [7] = pos_accuracy_bucket bit 0.
  - `sats`: bits [0:5] = pos_sats, [6:7] = pos_accuracy_bucket bits 1–2.
- Master table allows pos_fix_type/pos_accuracy_bucket as stubs; pos_sats → sats, pos_flags_small → pos_flags. Packing into the same bytes is an implementation choice to avoid new NodeEntry fields and matches "normalized product fields" (four logical values stored in two bytes). **No violation.** Document packing in apply_pos_full comment; add test assertions so regressions are caught.

### Node_Status field mapping

- battery_percent, uptime_sec (from uptime10m), max_silence_10s, hw_profile_id, fw_version_id applied to NodeEntry. tx_power_ch_throttle, role_id, battery_est_rem_time intentionally not stored (NodeEntry has no fields; master table stubs). **Aligned.**

### TX/RX and lifecycle

- TX: only v0.2 packets. RX: v0.1 handlers retained; v0.2 handlers call apply_pos_full/apply_status. No hitchhiking: status_eligible gates on !pos_or_alive_enqueued. **Aligned.**

### Wire IDs and widths

- 0x06, 0x07 in packet_header.h; decode_header accepts up to BeaconStatus. hw/fw uint16 in codec and apply. **Aligned.**

---

## 3) Decisions

| Decision | Action |
|----------|--------|
| Pos_Quality mapping | Keep as-is. Add comment in node_table.cpp documenting packing per master table stubs. Add test assertions for entry.pos_flags and entry.sats in test_rx_pos_full_applies_position_and_quality. |
| Node_Status mapping | No change. |
| TX/RX / lifecycle / widths | No change. |
| Remaining debt | None required for merge. Optional later: explicit test that Status is not enqueued in same formation pass as PosFull (currently implied by status_eligible). |

---

## 4) What was fixed vs left for later

| Item | Fixed in this pass | Left for later |
|------|--------------------|----------------|
| Pos_Quality documentation | Comment in apply_pos_full (packing layout + master table ref). | — |
| Pos_Quality test strength | Assertions for packed pos_flags and sats values in test_rx_pos_full_applies_position_and_quality. | — |
| No-hitchhiking test | — | Optional: dedicated test that only one of PosFull/Alive vs Status is enqueued per pass. |

---

## 5) Merge-ready recommendation

**Verdict: Merge-ready** (corrective changes applied).

- No canon violations found.
- Tightening applied: Pos_Quality packing documented in `node_table.cpp`; test_rx_pos_full_applies_position_and_quality asserts packed `pos_flags` and `sats` values.
- Devkit build (devkit_e220_oled) and test_beacon_logic (75 tests) rechecked and passing.

---

## 6) Exit criteria checklist

| Criterion | Done |
|-----------|------|
| Risk audit written | Yes — this document |
| Pos_Quality mapping classified as aligned or fixed | Aligned; comment + test assertions added |
| Node_Status field mapping classified as aligned or fixed | Aligned; no change |
| TX/RX migration split verified | Verified; no change |
| Legacy compat state boundary verified | Verified; no change |
| Width/naming checks verified | Verified; no change |
| Targeted tests added/updated if needed | Yes — Pos_Quality packed-value assertions |
| Devkit build rechecked if code changed | Yes — green |
| Final merge-ready recommendation | **Merge-ready** |

---

## 7) Remaining known debt

- **None** required for merge. Optional follow-up: add an explicit test that Node_Status is never enqueued in the same formation pass as Node_Pos_Full (no hitchhiking); currently guaranteed by `status_eligible = !pos_or_alive_enqueued && …`.
