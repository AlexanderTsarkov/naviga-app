# #417 Persisted seq16 + NVS adapter — implementation-ready findings

**Issue:** [#417](https://github.com/AlexanderTsarkov/naviga-app/issues/417) (S03 P0: Persisted seq16 + NVS adapter; links #296, #355)  
**Scope:** Narrow implementation plan only. No code changes in this document.

---

## 1. Current behavior as found in code

- **Seq16 source of truth:** `BeaconLogic::seq_` (member `uint16_t seq_ = 0` in `firmware/src/domain/beacon_logic.h` line 206). No other copy; single global per-node counter.
- **Generation:** `BeaconLogic::next_seq16()` (`beacon_logic.cpp` lines 19–22): increments `seq_`, returns new value. First call returns 1.
- **Usage:** Every enqueued packet (Core, Alive, Tail1, Tail2, Info) gets a distinct seq16 via `next_seq16()` at formation time. Used in:
  - Legacy API: `build_tx()` (line 79) for Core/Alive.
  - Slot API: `update_tx_queue()` (lines 139, 154, 171, 189, 207) for all five types.
- **Boot/init:** `BeaconLogic` is default-constructed (seq_ = 0). `M1Runtime` owns it (`m1_runtime.h` line 70). `M1Runtime::init()` does not set or read seq; it only sets `min_interval_ms` and `max_silence_ms`. `AppServices::init()` (app_services.cpp) runs Phase A (HW), Phase B (load_pointers, role profile from `naviga_storage`), then Phase C and calls `runtime_.init(...)` at line 297. No seq16 load anywhere.
- **Persistence today:** None. `naviga_storage.cpp/h` implement Preferences/NVS for role/radio pointers and role profile record only; no seq16 key or API. No references to #296 or #355 in firmware.
- **First TX:** After boot, first formation uses seq_ = 0 → first `next_seq16()` returns 1, so first packet carries seq16 = 1.

---

## 2. Desired behavior for #417

- **Restore:** On boot, if a non-zero seq16 is found in NVS, the first TX after boot must use `restored_value + 1` (canon: [rx_semantics_v0.md §5.3](docs/product/areas/nodetable/policy/rx_semantics_v0.md) — “First TX after reboot uses `restored_seq + 1`”).
- **Absent/zero:** If no key or value is 0, keep current behavior: first packet has seq16 = 1.
- **Write:** Persist the last sent seq16 so the next boot can restore it; write policy to be chosen below.

---

## 3. Proposed adapter boundary

- **Domain/BeaconLogic:** Must not depend on NVS, Preferences, or any platform. It may only receive an “initial seq” value via a setter (so next `next_seq16()` returns initial + 1).
- **Platform:** Owns NVS read/write for seq16. Existing `naviga_storage` is the right place: same namespace `"naviga"`, same Preferences pattern. Add:
  - `bool load_seq16(uint16_t* out)` — read one key (e.g. `"seq16"`); return true if key present and value valid; if missing/invalid, leave *out unchanged and return false.
  - `bool save_seq16(uint16_t value)` — write value to NVS.
- **App layer (wiring):** `AppServices` already calls `load_pointers()` and `runtime_.init()`; it must:
  - After `runtime_.init()`, call `load_seq16()`; if it returns true and `*out > 0`, call `runtime_.set_initial_seq16(*out)`.
  - After a successful TX (see below), call `save_seq16(runtime_.geo_seq())` when the current seq has changed from last persisted.
- **Runtime (M1Runtime):** Thin pass-through: `set_initial_seq16(uint16_t)` that forwards to `beacon_logic_.set_initial_seq16(value)`. No storage or platform dependency in M1Runtime.
- **BeaconLogic:** Add `void set_initial_seq16(uint16_t value)` setting `seq_ = value`. Call only before any `next_seq16()`; typically once at init. So after restore 42, first `next_seq16()` returns 43.

**Narrowest seam:** Platform = `naviga_storage` (load/save seq16). Domain = BeaconLogic (set_initial_seq16). Wiring = AppServices (load after init, save after send). No IRadio or HAL change; no new platform interface beyond two functions in existing storage module.

---

## 4. Boot/init flow for restored_seq16

1. **Phase A/B** (unchanged): HW bring-up, `load_pointers()`, role profile load, etc.
2. **Phase C (before first TX):** After `runtime_.init(...)`:
   - `uint16_t restored = 0; (void)load_seq16(&restored);`
   - If `restored > 0`: `runtime_.set_initial_seq16(restored)`.
3. **First formation:** When `handle_tx()` → `update_tx_queue()` → `next_seq16()`, the returned value is restored+1 when restore was applied, else 1.
4. No change to formation order, TX scheduling, or RX path.

---

## 5. Recommended write policy and rationale

- **Recommendation: persist after every successful send (per-TX).**
- **Rationale:**
  - **Correctness:** Smallest safe choice. After we send a packet with seq16 S, we must persist S so that after reboot we restore S and send S+1. Any batched/flush policy risks losing the last sent value on power loss before flush.
  - **Implementation:** One write per actually transmitted packet. At ~30 s cadence that’s ~2 writes/min. No timer or flush logic.
  - **Wear:** NVS typically ~100k cycles per key. At 2880 writes/day, one key lasts ~34 days; for a single uint16 key this is acceptable for #417; batched flush can be a later optimization if needed (and documented as tradeoff).
- **Where to trigger write:** In app layer only. After `runtime_.tick(now_ms)` returns, if a send occurred this tick, persist. Option A: have `M1Runtime::tick()` or `handle_tx()` accept an optional “on_sent_seq16” callback. Option B (simpler, no API change): in `AppServices::tick()`, after `runtime_.tick()`, if `runtime_.geo_seq() != last_persisted_seq_`, then `save_seq16(runtime_.geo_seq())` and set `last_persisted_seq_ = runtime_.geo_seq()`. The value returned by `geo_seq()` after a send is exactly the seq16 that was placed in the sent packet (formation called `next_seq16()` once per enqueued packet; we send one packet per tick). So Option B is correct and keeps persistence out of M1Runtime/domain.
- **Edge:** If we never send (e.g. no fix, no silence yet), we don’t write; on next boot we restore 0 or last written value. No double-write on same seq.

---

## 6. Files likely to change

| File | Change |
|------|--------|
| `firmware/src/platform/naviga_storage.h` | Declare `bool load_seq16(uint16_t* out);` and `bool save_seq16(uint16_t value);`. |
| `firmware/src/platform/naviga_storage.cpp` | Implement with Preferences, namespace `"naviga"`, key e.g. `"seq16"`. |
| `firmware/src/domain/beacon_logic.h` | Declare `void set_initial_seq16(uint16_t value);`. |
| `firmware/src/domain/beacon_logic.cpp` | Implement setter: `seq_ = value;` (only before any next_seq16()). |
| `firmware/src/app/m1_runtime.h` | Declare `void set_initial_seq16(uint16_t value);`. |
| `firmware/src/app/m1_runtime.cpp` | Implement: `beacon_logic_.set_initial_seq16(value);`. |
| `firmware/src/app/app_services.cpp` | After `runtime_.init()`: load_seq16; if restored > 0, `runtime_.set_initial_seq16(restored)`. In `tick()`, after `runtime_.tick()`: if `runtime_.geo_seq() != last_persisted_seq_`, then `save_seq16(runtime_.geo_seq())`, `last_persisted_seq_ = runtime_.geo_seq()`. Add member `uint16_t last_persisted_seq_ = 0`. |
| `firmware/src/app/app_services.h` | Add `uint16_t last_persisted_seq_ = 0;` (if not in .cpp only). |

No changes to: protocol codecs, NodeTable, RX path, BLE, TX rules/scheduling logic, or other persistence (NodeTable snapshot #418).

---

## 7. Planned tests

- **Unit (BeaconLogic):** With `set_initial_seq16(100)`, first `next_seq16()` returns 101. Second call returns 102. No call to set_initial_seq16 after first next_seq16 (document as precondition).
- **Unit (BeaconLogic):** Default construction; first `next_seq16()` returns 1 (existing behavior).
- **Unit (integration-style):** Form one Core frame after `set_initial_seq16(42)`; decode payload and assert seq16 in Common prefix is 43. (Can be in existing test_beacon_logic or a small new test.)
- **Optional (platform):** If we add a test-only or mock storage path, “load 5 → set_initial → first packet has 6” in app/runtime test; otherwise cover by BeaconLogic test + manual boot check.

No change to RX semantics tests (node_table_domain, beacon_logic RX tests); they already cover dedupe/ooo/wrap.

---

## 8. Risks / things to avoid

- **Do not** add NVS, Preferences, or platform includes to `BeaconLogic` or `M1Runtime` or any domain code.
- **Do not** persist inside `handle_tx()` or `BeaconLogic`; keep write in app layer only.
- **Do not** change formation order, packet types, or trigger rules (#420/#422 out of scope).
- **Do not** touch NodeTable snapshot/restore (#418), RX apply rules (#421), or BLE.
- **Avoid** expanding scope: no “persistence service,” no generic key-value API; only seq16 load/save in existing storage module.
- **Reuse:** No existing seq16 persistence code found; #296/#355 are not present in tree. Implement the above from scratch, aligned with existing `load_pointers`/`save_pointers` pattern in `naviga_storage`.

---

## Canon references (read-only)

- **rx_semantics_v0.md §5.1:** “In V1-A the sender's seq16 counter starts at 0 on every power-on / reboot (no persistence). The first outgoing packet carries seq16 = 1.”
- **rx_semantics_v0.md §5.3:** “Persisted counter (recommended next step): Store seq16 in NVS; restore on boot. First TX after reboot uses `restored_seq + 1`. Eliminates the tolerance problem entirely with no protocol change.”
- **seq_ref_version_link_metrics_v0.md:** Defines last_seq, last_core_seq16, ref_core_seq16; no sender persistence there (receiver-side).
- **identity_naming_persistence_eviction_v0.md §4:** NodeTable persistence/eviction is separate; “Current reality: RAM-only.”
