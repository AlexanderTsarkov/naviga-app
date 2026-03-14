# #426 — current_state reconciliation report (S03 execution)

**Issue:** #426 · **Umbrella:** #416 · **Iteration:** S03__2026-03__Execution.v1

**Summary:** Contract restated; five stale statements in `current_state.md` identified and fixed (v0.1 packet/TX/NodeTable wording → v0.2; Next focus no longer points to completed seq16/snapshot or S03 planning as primary). Stale-vs-needed audit and implementation-vs–Product Truth reconciliation performed (no remaining mismatch). NodeTable field-origin audit: all groups have a clear source; no ambiguity. `docs/product/current_state.md` updated to merged S03 execution state; #416 ready for final closure/recap.

---

## 1) #426 contract (restated)

Update `docs/product/current_state.md` so it reflects the **actual merged S03 execution outcome** (#416); reconcile with **corrected Product Truth**; explicitly remove stale statements that survived mid-project canon correction; fix "Next focus" so it does not point to already completed work; add a concise S03 execution summary; keep the canon-vs-implementation disclaimer; perform implementation-vs-truth reconciliation and NodeTable field-origin audit. **Non-goals:** no code changes; no reopening #417–#425, #435, #438; no rewriting canon; no turning current_state into a planning doc.

---

## 2) Stale or misleading statements (3–5) — with evidence

| # | Location | Statement / snippet | Why stale |
|---|----------|---------------------|------------|
| 1 | `docs/product/current_state.md` § Radio/Protocol, line 28 | "Packet types (on-air): Core_Pos (0x01), Alive (0x02), Core_Tail (0x03), Operational (0x04), Informative (0x05)" | After #435 / #438, **canonical** on-air family is **v0.2**: Node_Pos_Full (0x06), Node_Status (0x07), Alive (0x02). v0.1 types are no longer accepted on RX ([packet_migration_v01_v02](docs/product/areas/nodetable/policy/packet_migration_v01_v02.md), [packet_truth_table_v02](docs/product/areas/nodetable/policy/packet_truth_table_v02.md)). |
| 2 | Same, line 29 | "Tail-1 (Core_Tail) and Operational/Informative sub-ordered" | v0.2 has no Core_Tail; TX is Node_Pos_Full, Node_Status, Alive. Tail-1 / Operational / Informative ordering is v0.1-only. |
| 3 | `docs/product/current_state.md` § Domain/NodeTable, line 36 | "Core_Tail ref_core_seq16 linkage; Operational vs Informative split (independent TX paths)" and "Tail-1 apply only if ref matches lastCoreSeq" | v0.2: Node_Pos_Full (single-packet position+quality); Node_Status (combined op+info); Alive. ref_core_seq16 / Tail-1 apply is deprecated; v0.1 RX removed per #438. |
| 4 | `docs/product/current_state.md` § Next focus, line 71 | "Persisted seq16 / snapshot semantics (post–V1-A)" | #417 (persisted seq16 + NVS) and #418 (NodeTable snapshot + restore) are **completed** and merged. This line implies they are still "next". |
| 5 | Same, line 70 | "S03 planning: #296" | S03 **execution** (#416) is complete (P0 #417–#422, plus #423–#425, #435, #438, #443). "Next focus" should not read as if S03 planning is still the main item; execution outcome is landed. |

---

## 3) Stale vs needed — audit table

| Section | Already reflects S03 | Stale | Missing / needed after #417–#425, #435, #438, #443 |
|--------|----------------------|-------|----------------------------------------------------|
| Source of truth | Yes (disclaimer intact) | — | — |
| Firmware | Boot Phase A/B/C, M1Runtime, provisioning | — | Optional: OOTB autonomous start (#423) and user profile baseline (#443) are landed; one-line mention keeps current_state accurate. |
| Radio/Protocol | Radio profiles, 2-byte header, Common prefix notion | Packet types and TX queue text (v0.1) | Replace with v0.2 packet family (Node_Pos_Full, Node_Status, Alive) and cutover-complete RX. |
| Domain/NodeTable | NodeTable as SSOT, seq16, RX semantics (accepted/dup/ooo/wrap) | Core_Tail / Tail-1 / Operational vs Informative wording | State v0.2: Node_Pos_Full, Node_Status, Alive; NodeTable normalized; legacy ref fields runtime-local only; link to packet_truth_table_v02 and migration. |
| Mobile | Unchanged | — | — |
| Docs/Process | Unchanged | — | — |
| What changed this iteration | S02 row, S03 promotion row | — | Add one row: **S03 execution** (#416) with P0 + #423–#425, #435, #438, #443 and link to umbrella. |
| Next focus | — | S03 planning #296; persisted seq16/snapshot | Remove completed work; set next to post-S03 (e.g. docs/legacy, channel/AutoPower, or S04). |

---

## 4) Implementation vs corrected Product Truth — reconciliation

- **Packet family:** Canon is v0.2 (packet_truth_table_v02, packet_migration_v01_v02). Merged implementation: #435 (v0.2 TX/RX), #438 (v0.1 RX removed). **Aligned.**
- **NodeTable:** Canon (nodetable_master_field_table_v0, snapshot_format_v0): normalized product fields only; legacy ref fields (last_core_seq16, last_applied_tail_ref*) runtime-local only; not in BLE/persistence. #419, #418 implemented. **Aligned.**
- **Boot / provisioning:** Phase A/B/C and role/radio from NVS in canon; #423 (boot phases, OOTB autonomous), #424 (radio profile baseline), #443 (user profile baseline) merged. **Aligned.**
- **Seq16:** Persisted seq16 + NVS (#417) per rx_semantics / seq_ref policy. **Aligned.**

**Conclusion:** No meaningful implementation-vs–Product Truth mismatch remains for the S03 scope. Stale text in current_state was legacy wording (v0.1), not a divergence from canon.

---

## 5) NodeTable field-origin / source audit

Per [nodetable_master_field_table_v0](docs/product/areas/nodetable/policy/nodetable_master_field_table_v0.md), every major field group has a **clear origin** and **binding**:

| Field group | Source class | Binding |
|-------------|--------------|---------|
| Identity / local (node_id, short_id, is_self, node_name, …) | identity; derived (is_self, short_id_collision) | NodeTable + BLE + persistence per master table §1. |
| Position (lat_e7, lon_e7, pos_fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small, pos_age_s) | radio (from packets); pos_age_s derived | NodeTable; product block §2. |
| Battery / survivability (battery_percent, charging, uptime_sec) | radio (from packets) | §3. |
| Radio control / role (tx_power_step, channel_throttle_step, role_id, max_silence_10s, hw_profile_id, fw_version_id) | radio (active profile) or profile (user/role) | §4. |
| Receiver-injected (last_seq, last_seen_ms, last_rx_rssi, snr_last, last_payload_version_seen) | runtime | §5; last_seq/last_seen_ms not in BLE/persistence; snr_last BLE with 127=NA. |
| Derived (last_seen_age_s, is_stale, pos_age_s) | derived | §6. |
| Legacy ref (last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, …) | runtime-local (decoder only) | §7; **not** in BLE or persistence. |

**Ambiguity:** None. All canonical NodeTable fields have a defined source (radio-driven product field, active user profile, active radio profile, runtime-injected, derived, or debug/runtime-local only). Legacy ref fields are explicitly "runtime-local decoder only" and excluded from product surface.

---

## 6) Updates applied to current_state.md

- **Last updated:** Set to 2026-03-12.
- **Iteration tag:** Set to S03 execution (post–#416).
- **Radio/Protocol:** Replaced v0.1 packet list and TX queue wording with v0.2 packet family (Node_Pos_Full, Node_Status, Alive) and cutover-complete RX; kept canon refs and added packet_truth_table_v02 / packet_migration_v01_v02.
- **Domain/NodeTable:** Replaced Core_Tail / Tail-1 / Operational vs Informative with v0.2 semantics; added link to packet_truth_table_v02; retained RX semantics and contracts list; noted legacy ref fields runtime-local only.
- **Firmware:** Added one line: OOTB autonomous start (#423) and user profile baseline (#443) landed.
- **What changed this iteration:** Added one row for S03 execution (#416): P0 #417–#422, #423–#425, #435, #438, #443 and link to umbrella.
- **Next focus:** Removed "Persisted seq16 / snapshot semantics" and refocused "S03 planning" to post-S03: docs/legacy migration, channel discovery/AutoPower (#175, #180); retained #278 and clarified PR-1–PR-3 landed.
- **Source of truth:** Left unchanged.

---

## 7) Minimal follow-through

- **\_working/README.md:** Already states iteration S03__2026-03__Execution.v1 and #416; no change required for #426.
- **\_working/ITERATION.md:** No change; first line remains iteration ID.

---

## 8) Final close recommendation

- **#426:** All exit criteria met: contract restated; 3–5 stale statements identified with evidence; stale-vs-needed table produced; implementation vs corrected Product Truth reconciled (no remaining mismatch); NodeTable field-origin audit performed (no ambiguity); current_state.md updated to merged S03 execution state; "Next focus" fixed; minimal follow-through confirmed.
- **#416:** current_state now reflects the actual merged S03 execution outcome and corrected Product Truth. Recommendation: **ready for final closure/recap** once #426 is closed and any desired umbrella comment is added.
