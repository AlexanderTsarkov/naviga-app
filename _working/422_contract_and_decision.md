# #422 — Packetization / canon-behavior transition: contract and decision

**Issue:** #422. **Iteration:** S03 (first line of _working/ITERATION.md).  
**Canon:** packet_context_tx_rules_v0, packet_sets_v0, nodetable_master_field_table_v0.

---

## 1) Restated contract (#422)

- **Scope:** Choose and implement **exactly one** of:
  - **Path A (v0.2 redesign):** Node_Pos_Full + Node_Status; encoding and TX semantics per canon; Node_Status lifecycle (bootstrap, urgent/threshold/hitchhiker, T_status_max); align payload/NodeTable to canonical names including `uptime_10m`.
  - **Path B (v0.1 retained + canon-aligned behavior):** Keep current packet family (Core, Alive, Tail1, Operational, Informative); add throttle/merge rules so **effective behavior** matches canon (status interval, no forbidden hitchhiking, equivalent canon behavior); align payload/NodeTable to canonical normalized names including `uptime_10m`.

- **Mandatory:** Active-values plane (payloads and NodeTable use active profile values only; no profile refs in product field set). Definition of done: Node_Status lifecycle for redesign path; `uptime_10m` and normalized field names in payload/NodeTable for both paths. No silent changes to `hw_profile_id` / `fw_version_id` width (remain uint16).

- **Non-goals:** Do not reopen #417–#421; do not re-debate Product Truth unless real contradiction; do not leave old Core/Tail/Op/Info as implicit canon if Path A is chosen; do not mix packetization with BLE/app/UI; do not hide migration/compat in comments only.

---

## 2) Decision: Path B (v0.1 retained + throttle/merge)

**Chosen path:** **Path B.**

**Justification:**

1. **Minimum complete change:** Path B does not introduce new wire formats (Node_Pos_Full, Node_Status) or new msg_types. RX decoders and apply paths stay unchanged; wire contracts and TX/RX symmetry are preserved.
2. **Canon behavior achievable:** packet_context_tx_rules_v0 §2a (Node_Status lifecycle) can be mapped onto the existing Op (0x04) + Info (0x05) packets by:
   - Using a **single status timing budget:** one logical “status” send per `min_status_interval_ms` (30s), at least one per `T_status_max` (300s).
   - **No hitchhiking:** Do not enqueue Operational or Informative in the same formation pass in which Core or Alive is enqueued.
   - **Bootstrap:** Allow up to 2 status sends (Op or Info) under bootstrap rules (first at first opportunity, second no earlier than min_status_interval_ms after the first).
   - **Merge semantics:** At most one of Op or Info enqueued per formation pass when status is due; repeated changes within the interval merge into the existing slot (replace-in-place).
3. **Risk:** Path A would require new encoders/decoders, new msg_type or payloadVersion, and Pos_Quality 2-byte layout (still TBD in §4 of packet_context_tx_rules_v0). Path B avoids wire and RX changes.
4. **Field widths:** Path B keeps hw_profile_id and fw_version_id as uint16; no change.

**Out of scope for this issue:**

- New packet types Node_Pos_Full and Node_Status (Path A).
- Changing Core/Tail wire layout or removing Tail.
- Backward-compat RX for legacy vs new packets (N/A for Path B).
- BLE/app/UI or unrelated work.

---

## 3) Packetization truth table (Path B)

| Aspect | Current v0.1 (code) | Target (Path B) | Canon reference |
|--------|----------------------|----------------|------------------|
| Position | Core_Pos + Core_Tail (same pass); one seq16 each; ref_core_seq16 in Tail | Unchanged | packet_sets_v0 §1 |
| Alive | !pos_valid && time_for_silence | Unchanged | packet_context_tx_rules_v0 §1 |
| Status (Op+Info) | Same cadence gate as Core; Op and Info enqueued when gate + has_* | Separate status timing: min_status_interval_ms (30s), T_status_max (300s); at most one status enqueue per pass; no enqueue when Core/Alive enqueued this pass | packet_context_tx_rules_v0 §2, §2a |
| Hitchhiking | Op/Info can be enqueued same pass as Core | Forbidden: do not enqueue Op/Info in pass where Core or Alive is enqueued | §2a |
| Status bootstrap | None | Up to 2 status sends; second not before min_status_interval_ms after first | §2a |
| NodeTable / payload names | uptime_sec on wire; master table cites uptime_10m as canon name | Explicit alignment: product/canon name uptime_10m; storage remains uptime_sec where used | nodetable_master_field_table_v0 §3 |
| hw/fw IDs | uint16 | uint16 (no change) | packet_sets_v0 §4.2 |

---

## 4) Migration / implementation plan (Path B)

- **Packet names and roles:** Unchanged. Core (0x01), Alive (0x02), Tail1 (0x03), Operational (0x04), Informative (0x05).
- **Status timing in BeaconLogic:**
  - Add `last_status_tx_ms_`, `last_status_enqueue_ms_`, `min_status_interval_ms_` (30s), `T_status_max_` (300s), `status_bootstrap_count_` (0..2).
  - When forming: if Core or Alive was enqueued this pass → skip Op and Info formation (no hitchhiking).
  - When forming Op/Info: only if `(now - last_status_tx_ms_ >= min_status_interval_ms_)` or bootstrap (count < 2 and second not before min_status_interval_ms after first); and force enqueue if `(now - last_status_enqueue_ms_ >= T_status_max_)`.
  - At most one of Op or Info enqueued per formation pass when status is due (e.g. prefer Op if has_battery||has_uptime else Info).
- **Callback when status sent:** M1Runtime calls BeaconLogic when a P3 packet (Op or Info) is successfully sent, so BeaconLogic can set `last_status_tx_ms_`.
- **RX / apply:** No change; still apply Core, Alive, Tail1, Op, Info per existing rules.
- **Active-values:** Already in force; SelfTelemetry and payloads use active values from runtime.
- **Normalized names:** Document and ensure NodeTable/master table use `uptime_10m` as canon product name; implementation may keep `uptime_sec` in storage with clear mapping.
- **Tests:** Status throttle (no Op/Info same pass as Core); min_status_interval and T_status_max behavior; bootstrap (2 sends); no hitchhiking.

---

## 5) Exit criteria checklist

- [x] #422 contract restated
- [x] Path chosen: v0.1 + throttle/merge (Path B)
- [x] Packetization truth table produced
- [x] Migration/implementation plan written
- [x] Minimal complete #422 transition implemented
- [x] Tests added/updated and passing
- [x] Devkit build passing
- [x] Docs/inventory synchronized
- [x] Final close recommendation prepared
