# Sequence Number (seq16) Audit — S02

**Date:** 2026-02-26  
**Iteration:** S02__2026-03__docs_promotion_and_arch_audit  
**Work Area:** Docs (audit) / Architecture  
**Scope:** seq field: size, scope, reboot behaviour, dedup/anti-loop, wrap, epoch. Canon vs WIP vs code.

---

## 1) File Inventory — where seq/sequence is mentioned

| File | Type | What it asserts about seq |
|------|------|--------------------------|
| `docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md` | Canon contract | `seq16` (uint16, 2 B, LE) in BeaconCore §4.1; `core_seq16` in Tail-1 §4.2. Single per-node counter (stated implicitly via Alive cross-ref). |
| `docs/product/areas/nodetable/contract/alive_packet_encoding_v0.md` | Canon contract | `seq16` in Alive; **explicitly states** "same per-node seq16 counter as BeaconCore and Tails (single counter across packet types during uptime)". |
| `docs/product/areas/nodetable/policy/rx_semantics_v0.md` | Canon policy | §1 Definitions: seq16 scope = "single per-node counter across all transmitted packet types during uptime". Wrap handling: modulo arithmetic / signed difference. Duplicate/ooo rules. |
| `docs/product/areas/nodetable/policy/field_cadence_v0.md` | Canon policy | seq16 canonical (§2 note). **Stale note §5.4:** "exact encoding (e.g. seq8 vs seq16, field order) is **TBD**" — this is a **residual stale TBD** that contradicts the rest of the canon. |
| `docs/product/areas/nodetable/policy/activity_state_v0.md` | Canon policy | seq16 used for duplicate detection (§4.2); out-of-order (§4.3). |
| `docs/product/areas/nodetable/policy/link_metrics_v0.md` | Canon policy | `rxDuplicatesCount` / `rxOutOfOrderCount` (diagnostic counters). |
| `docs/product/areas/nodetable/policy/nodetable_fields_inventory_v0.md` | Canon policy | `last_seq` uint16 in NodeTable record; `lastOriginSeqSeen` stream-level metadata. |
| `docs/product/areas/nodetable/index.md` | Canon hub | §2 invariants: "seq16 is a **single per-node counter** across all packet types (BeaconCore, Tail-1, Tail-2, Alive) during uptime." |
| `docs/protocols/ootb_radio_v0.md` | Legacy doc (OOTB v0) | `seq` field: "u8 или u16" — **unresolved ambiguity**; predates canon decision. |
| `docs/firmware/ootb_node_table_v0.md` | Legacy doc (OOTB v0) | `last_seq` u8 or u16 — same ambiguity. |
| `docs/firmware/ootb_firmware_arch_v0.md` | Legacy doc | Mentions `last_seq` in NodeTable record; no size stated. |
| `docs/product/naviga_mesh_protocol_concept_v1_4.md` | Concept/WIP | `seq_counter` (no size stated); dedup key = `(origin_id, seq)`; `PacketState(origin_id, seq)` for mesh relay dedup. No reboot/epoch discussion. |
| `docs/product/naviga_join_session_logic_v1_2.md` | Concept/WIP | `session_id` used for JOIN flow; `origin_id = 63` for service packets. No seq size stated. |
| `docs/product/wip/areas/nodetable/policy/snapshot-semantics-v0.md` | WIP policy | Explicitly states `PacketState(origin_id, seq)` for dedup/ordering **belongs to Mesh protocol, not NodeTable**; MUST NOT persist. |
| `docs/product/wip/areas/nodetable/policy/restore-merge-rules-v0.md` | WIP policy | Out-of-order RX noted; no seq size. |
| `docs/product/wip/areas/nodetable/policy/source-precedence-v0.md` | WIP policy | `seq` used as tie-breaker for position/telemetry ordering; no size. |
| `docs/product/wip/areas/nodetable/research/naviga-current-state.md` | WIP research | `last_seq` uint16 in NodeTable record (matches code). |
| `firmware/src/domain/beacon_logic.h` | Code | `seq_` uint16; `out_core_seq` uint16; `out_seq` uint16. |
| `firmware/src/domain/beacon_logic.cpp` | Code | `seq_` starts at 0; increments by 1 each TX (Core or Alive); wraps naturally as uint16. Single counter for both Core and Alive. |
| `firmware/src/domain/node_table.h` | Code | `last_seq` uint16 per NodeEntry. |
| `firmware/src/domain/node_table.cpp` | Code | `seq16_order()` with modulo wrap (delta ≤ 32767 = newer). Comment: "Per rx_semantics_v0 §1". |
| `firmware/src/app/m1_runtime.h` | Code | `geo_seq()` returns uint16 (from `beacon_logic_.seq()`); `last_tx_core_seq_` uint16. `stats_` is `RadioSmokeStats` which has `last_seq` **uint32** (smoke test counter — separate from geo seq). |
| `firmware/src/app/m1_runtime.cpp` | Code | `stats_.last_seq` incremented as uint32 (instrumentation log counter, not on-air seq). `self_fields_.seq = 0` at init. |
| `firmware/src/services/radio_smoke_service.h` | Code | `last_seq` uint32 — smoke test ping/pong counter; **completely separate** from geo beacon seq. |
| `firmware/src/services/oled_status.h` | Code | `geo_seq` uint16 — display only. |
| `firmware/test/test_node_table_domain/test_node_table_domain.cpp` | Test | Tests: duplicate same seq, ooo older seq, newer seq, wrap (65535→0 newer, 0→65535 older). All uint16. |
| `firmware/test/test_beacon_logic/test_beacon_logic.cpp` | Test | seq increments 1→2 on successive TX; `fields.seq` uint16. |

---

## 2) Canon vs WIP vs Code — Comparison Table

| Dimension | Canon (promoted docs) | WIP docs | Code (firmware) | Status |
|-----------|----------------------|----------|-----------------|--------|
| **Field size** | `uint16` (seq16) — explicit in `beacon_payload_encoding_v0` §4.1, `alive_packet_encoding_v0` §3.1, `rx_semantics_v0` §1, `index.md` §2 | No contradiction in WIP | `uint16_t seq_` in `beacon_logic.h`; `uint16_t last_seq` in `node_table.h`; `uint16_t` in all tests | ✅ **Aligned** |
| **Scope (single counter)** | "single per-node counter across all packet types (BeaconCore, Tail-1, Tail-2, Alive) during uptime" — `alive_packet_encoding_v0`, `rx_semantics_v0` §1, `index.md` §2 | No contradiction | `beacon_logic.cpp`: single `seq_` used for both Core and Alive TX | ✅ **Aligned** |
| **Dedup key** | `(nodeId, seq16)` per-node global — `rx_semantics_v0` §1; `snapshot-semantics-v0` says `PacketState(origin_id, seq)` belongs to Mesh, not NodeTable | Mesh concept: `(origin_id, seq)` for relay dedup — separate layer | `seq16_order()` in `node_table.cpp` is per-node; no mesh relay dedup in code (OOTB v0 has no relay) | ✅ **Aligned** (two layers: NodeTable per-node vs Mesh relay — correctly separated) |
| **Wrap handling** | "modulo arithmetic or signed difference within a window" — `rx_semantics_v0` §1 | — | `delta = incoming - last` (uint16 subtraction); `delta ≤ 32767` = newer | ✅ **Aligned** |
| **Reboot behaviour** | **NOT STATED ANYWHERE in canon or WIP** | Not stated | `seq_ = 0` at construction (reset on power-on/reboot); no persistence, no epoch/nonce | ❌ **GAP — P0** |
| **Partition→merge** | Not stated | Not stated | Not addressed | ❌ **GAP — P1** |
| **Epoch / boot_nonce / session_id** | Not stated for seq | `session_id` in JOIN concept but for session membership, not seq epoch | Not implemented | ❌ **GAP — P1** |
| **Stale TBD in field_cadence** | `field_cadence_v0.md` §5.4 still says "seq8 vs seq16 TBD" | — | seq16 implemented | ❌ **GAP — P2 (stale text)** |
| **Smoke service seq** | Not in canon (test-only) | — | `RadioSmokeStats.last_seq` uint32 — separate, not on-air geo seq | ✅ **No conflict** (different domain) |
| **stats_.last_seq in M1Runtime** | Not in canon | — | `stats_.last_seq` uint32 — instrumentation log counter (counts TX events), not the on-air seq16 | ⚠️ **Naming confusion risk** — see Gap G4 |

---

## 3) Gap List

### G1 — P0: Reboot behaviour of seq16 not specified anywhere

**Where:** No canon doc, no WIP doc defines what happens to seq16 after reboot.  
**Code state:** `seq_ = 0` on construction → resets to 0 on every reboot.  
**Risk:** After reboot, a node restarts seq from 0 (or 1 on first TX). Any receiver that has `last_seq = N` (where N > 0) will see the new packets as **older** (via `seq16_order`) until the counter wraps past N. With wrap window = 32767, a receiver that saw seq=50000 before reboot will treat seq=1..17768 after reboot as **older** → those packets are silently dropped / not used for position update. This is a **correctness issue** for nodes that reboot frequently (e.g. power cycling).

**Required decision:** Pick ONE reboot strategy (see §4 below).

---

### G2 — P1: Partition→merge not specified

**Where:** No doc addresses what happens when two mesh partitions merge and a node's seq16 is seen from two different "epochs" (pre-partition and post-partition).  
**Code state:** No mesh relay in OOTB v0 — not yet a runtime issue.  
**Risk:** When mesh relay is added, a receiver may have cached `last_seq = X` from before partition; after merge it receives `seq = Y` from the same node via a different path. If Y < X (e.g. relay delivered an old packet), it's treated as ooo. This is correct behaviour — but the doc should confirm it's intentional and not a bug.

**Required decision:** Confirm that `seq16_order` handles this correctly (it does — ooo packets don't overwrite position) and document it explicitly.

---

### G3 — P2: Stale "seq8 vs seq16 TBD" in `field_cadence_v0.md`

**Where:** `docs/product/areas/nodetable/policy/field_cadence_v0.md` §5.4 (Freshness marker encoding note):  
> "exact encoding (e.g. seq8 vs seq16, field order) is **TBD** and will be decided in a follow-up"

**Reality:** seq16 is already decided and documented in `beacon_payload_encoding_v0` §4.1, `alive_packet_encoding_v0` §3.1, `rx_semantics_v0` §1, `index.md` §2, and implemented in code. This TBD is stale.

**Fix:** Remove the TBD note; replace with pointer to `beacon_payload_encoding_v0` §4.1.

---

### G4 — P2: Naming confusion — `stats_.last_seq` (uint32) vs on-air `seq16` (uint16) in M1Runtime

**Where:** `firmware/src/app/m1_runtime.cpp` uses `stats_.last_seq` (from `RadioSmokeStats`, uint32) as an instrumentation TX event counter, while `beacon_logic_.seq()` returns the on-air uint16 seq16.  
**Risk:** Low (different types, different purposes), but a reader of `m1_runtime.cpp` may confuse the two. The log line `"pkt tx ... seq=%u"` uses `stats_.last_seq` (uint32 event counter), not the on-air `seq16`. This means the logged `seq=` in TX logs is a monotonic event counter, not the on-air seq16.  
**Fix:** Rename `RadioSmokeStats.last_seq` → `RadioSmokeStats.tx_event_seq` or `tx_log_seq` to distinguish from on-air seq16. Or document the distinction clearly in a comment.

---

### G5 — P1: Mesh concept `(origin_id, seq)` dedup — seq size not stated

**Where:** `docs/product/naviga_mesh_protocol_concept_v1_4.md` uses `(origin_id, seq)` as the dedup key for relay but never states the size of `seq`. When mesh relay is implemented, the seq size must match the on-air seq16.  
**Fix:** Add a note in the mesh concept doc that `seq` in `PacketState(origin_id, seq)` is the same `seq16` (uint16) from the beacon payload.

---

## 4) Canonical Decisions (proposed)

### D1: seq16 always (confirmed)

- **Decision:** seq is always uint16 (seq16). No seq8.
- **Rationale:** 16-bit gives 65536 values; with a 30-second beacon interval, wrap takes ~22 days. seq8 (256 values) wraps in ~2 hours — too short for partition/merge safety. seq16 is already implemented and documented.
- **Action:** Close G3 (remove stale TBD in `field_cadence_v0.md`).

### D2: Single global seq counter per node (confirmed)

- **Decision:** One seq16 counter per node, incremented on every outgoing packet regardless of type (Core, Alive, Tail-1, Tail-2 when implemented).
- **Rationale:** Simplifies receiver logic — one ordering window per node, no per-type state. Already documented in canon and implemented.
- **Action:** No change needed. Confirm in reboot strategy doc.

### D3: Reboot strategy — RECOMMENDATION: reset + receiver tolerance window

**Three options:**

| Option | Description | Pros | Cons |
|--------|-------------|------|------|
| **A. Reset to 0 (current)** | seq resets to 0 on every reboot. Receiver uses `seq16_order` — after reboot, new packets appear "older" until counter passes last-seen value. | Zero implementation cost. Already in code. | Receiver silently drops post-reboot packets for up to 32767 steps (worst case ~11 days at 30s interval; typical case ~N/2 steps where N = last seen seq). For frequent rebooters this is a real issue. |
| **B. Persisted counter (NVS)** | seq16 is saved to NVS on every TX (or periodically); restored after reboot. | Receiver sees monotonic sequence across reboots. No epoch needed. | NVS write wear (mitigated by write-on-change + periodic flush). Requires NVS dependency in BeaconLogic or init path. |
| **C. Boot nonce / session epoch** | seq16 resets to 0, but a `boot_nonce` (e.g. 8-bit or 16-bit random or incrementing value) is added to the packet. Dedup key becomes `(nodeId, boot_nonce, seq16)`. | Clean separation of epochs. No NVS wear. | Requires protocol change (new field in payload). Adds complexity to dedup logic. Not backward-compatible with current 19-byte Core layout without adding a byte. |

**Recommendation for V1-A: Option A (reset to 0) with documented receiver tolerance.**

Rationale:
- Already implemented; no code change needed.
- For OOTB v0 (no mesh relay, direct RX only), the practical impact is: a receiver that had `last_seq = N` will accept new packets once the new seq passes `N - 32767` (mod 32768). At 30s beacon interval, a node with `last_seq = 100` at reboot will have its packets accepted by the receiver after 100 ticks = 50 minutes. This is acceptable for OOTB v0.
- **Mitigation:** Document that receivers SHOULD apply a "reboot detection heuristic": if `lastRxAt` age > `maxSilence × K` (e.g. K=3), treat any incoming packet from that node as a fresh start (reset `last_seq` to the incoming seq). This is implementation-defined but should be noted in the policy.

**Post V1-A:** Option B (persisted counter) is the cleanest upgrade path when NVS is available. Option C (boot nonce) is appropriate when mesh relay is added and cross-epoch dedup is needed.

### D4: Dedup key (confirmed)

- **NodeTable layer:** dedup key = `(nodeId, seq16)` per-node global. Implemented correctly.
- **Mesh relay layer (future):** dedup key = `(origin_id, seq16)` — same seq16 field. `PacketState` belongs to Mesh protocol, not NodeTable (confirmed in `snapshot-semantics-v0.md`).
- **Action:** Add seq16 size note to mesh concept doc (G5).

### D5: Wrap-around (confirmed)

- **Rule:** `delta = (incoming - last) mod 2^16`. If `delta ∈ [1, 32767]` → newer. If `delta = 0` → same (duplicate). If `delta ∈ [32768, 65535]` → older.
- **Implemented:** `seq16_order()` in `node_table.cpp`. Tested in `test_node_table_domain.cpp`.
- **Action:** Document this rule explicitly in `rx_semantics_v0.md` §1 (currently says "modulo arithmetic or signed difference" without the exact window).

---

## 5) PR Plan

### PR-A: Docs canonization (docs-only, no code change)

**Branch:** `issue/seq16-canon-docs` (or attach to existing iteration issue)  
**Files to change:**

1. **`docs/product/areas/nodetable/policy/field_cadence_v0.md`** — §5.4 (Freshness marker encoding note):
   - Remove: "exact encoding (e.g. seq8 vs seq16, field order) is **TBD**..."
   - Replace with: "seq16 (uint16, 2 bytes, little-endian) is canonical; defined in [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) §4.1."

2. **`docs/product/areas/nodetable/policy/rx_semantics_v0.md`** — §1 (seq16 wrap):
   - Add explicit wrap rule: "delta = (incoming − last) mod 2^16; delta ∈ [1, 32767] → newer; delta = 0 → duplicate; delta ∈ [32768, 65535] → older."

3. **`docs/product/areas/nodetable/policy/rx_semantics_v0.md`** — new §6 (Reboot behaviour):
   - Document D3 decision: reset to 0 on reboot; receiver tolerance heuristic (reset `last_seq` if `lastRxAt` age > `maxSilence × K`).
   - Note post-V1-A upgrade path (persisted counter or boot nonce).

4. **`docs/product/naviga_mesh_protocol_concept_v1_4.md`** — §7 (PacketState) or glossary:
   - Add note: "`seq` in `PacketState(origin_id, seq)` is the on-air `seq16` (uint16) from the beacon payload."

**Size:** Small. 4 file edits, no code.

---

### PR-B: Code cleanup — naming (optional, low risk)

**Branch:** `issue/seq16-naming-cleanup`  
**Files to change:**

1. **`firmware/src/services/radio_smoke_service.h`** — rename `last_seq` → `tx_event_seq` in `RadioSmokeStats`.
2. **`firmware/src/app/m1_runtime.cpp`** — update all references to `stats_.last_seq` → `stats_.tx_event_seq`; add comment distinguishing from on-air seq16.
3. **`firmware/src/app/m1_runtime.h`** — update if needed.

**Size:** Mechanical rename, 3 files. No behaviour change.

---

### PR-C (post V1-A): Reboot strategy upgrade — persisted counter

**Branch:** `issue/seq16-persisted-counter`  
**Scope:** Add NVS read/write for seq16 in BeaconLogic init path. Requires:
- `BeaconLogic::init(uint16_t restored_seq)` or NVS adapter injection.
- Write seq to NVS on each TX (or on a flush timer).
- Unit test: restored seq > 0 → first TX uses restored_seq + 1.

**Prerequisite:** NVS/Preferences adapter available in firmware.

---

## 6) Exit Criteria Checklist

- [ ] **G1 closed:** Reboot behaviour documented in `rx_semantics_v0.md` §6 (PR-A).
- [ ] **G3 closed:** Stale "seq8 vs seq16 TBD" removed from `field_cadence_v0.md` (PR-A).
- [ ] **G5 closed:** seq16 size noted in mesh concept doc (PR-A).
- [ ] **Wrap rule explicit:** `rx_semantics_v0.md` §1 states exact delta window (PR-A).
- [ ] **G4 noted:** `stats_.last_seq` naming confusion documented or renamed (PR-B, optional).
- [ ] **No remaining "seq8" or "u8 или u16" ambiguity in promoted canon docs** (grep check after PR-A).
- [ ] **Legacy docs (`ootb_radio_v0.md`, `ootb_node_table_v0.md`) marked as legacy** with pointer to canon — or explicitly note that "u8 или u16" was pre-decision and canon is seq16. (Can be done in PR-A or a separate cleanup PR.)
- [ ] **Tests:** `test_node_table_domain.cpp` already covers duplicate/ooo/wrap — no new tests needed for PR-A. PR-C will need a new test for restored seq.

---

## 7) Quick grep check (post-PR-A)

After PR-A, run:

```
grep -rn "seq8\|u8 или u16\|seq8 vs seq16\|TBD.*seq" docs/product/areas/nodetable/
```

Expected: zero results in promoted canon docs. Legacy docs (`docs/protocols/`, `docs/firmware/`) may still have "u8 или u16" — acceptable if marked as legacy.

---

## Summary

| What | Status |
|------|--------|
| seq16 (uint16) always | ✅ Canon + code aligned |
| Single global counter per node | ✅ Canon + code aligned |
| Wrap handling (modulo delta ≤ 32767) | ✅ Code correct, canon slightly underspecified → fix in PR-A |
| Dedup key (nodeId, seq16) per-node | ✅ Canon + code aligned |
| Reboot behaviour | ❌ Not documented → PR-A (document reset+tolerance) |
| Partition→merge | ✅ Handled by existing ooo logic; document explicitly → PR-A |
| Epoch / boot_nonce | Not needed for V1-A; post-V1-A option B (persisted) recommended |
| Stale "seq8 vs seq16 TBD" in field_cadence | ❌ Stale text → remove in PR-A |
| Mesh concept seq size | ❌ Not stated → add note in PR-A |
| stats_.last_seq naming confusion | ⚠️ Low risk → optional PR-B rename |
