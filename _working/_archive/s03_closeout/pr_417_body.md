## Summary

Implements **#417**: persisted seq16 + narrow NVS adapter, with boot restore semantics aligned to canon (rx_semantics_v0 §5.3).

**Umbrella:** #416. **Historical trackers:** #296, #355.

## Behavior

- **Boot:** Read persisted seq16 from platform storage (`naviga_storage`). If restored value > 0, first TX after reboot uses `restored_seq16 + 1`. If absent or zero, fresh-start preserved (first seq16 = 1).
- **Persistence:** Per-TX write policy — after each successful send, persist the seq16 that was **actually sent** (read from the sent frame payload at Common offset 7–8). No batching in this PR.

## Adapter boundary

- **Platform** (`naviga_storage`): `load_seq16` / `save_seq16`; NVS/Preferences details stay here. Namespace `"naviga"`, key `"seq16"`.
- **Domain** (`BeaconLogic`): `set_initial_seq16(value)` only; no platform includes or storage logic.
- **Runtime** (`M1Runtime`): Forwards `set_initial_seq16`; exposes `last_sent_seq16()` (set from sent frame in `handle_tx` when send succeeds).
- **App** (`AppServices`): Restores after `runtime_.init()`; persists when `runtime_.last_sent_seq16()` differs from last persisted value.

## Write policy (chosen for this PR)

**Per-TX persistence.** Rationale: safest minimal behavior for power-loss; no flush timers or batching; one key, one value. Documented in code comments and here. Batching can be a follow-up if needed.

## Tests

- `test_seq16_default_first_packet_is_one` — default start ⇒ first packet seq16 = 1.
- `test_seq16_set_initial_then_first_packet_is_initial_plus_one` — set_initial_seq16(42) ⇒ first packet seq16 = 43.
- `test_seq16_set_initial_100_first_packet_is_101` — set_initial_seq16(100) ⇒ first packet seq16 = 101.

All 70 beacon_logic tests pass; devkit build succeeds.

## Non-goals (unchanged)

- No NodeTable snapshot/restore (#418), no RX semantics (#421), no TX scheduling redesign (#420/#422), no BLE, no broad storage abstraction.

Closes #417.
