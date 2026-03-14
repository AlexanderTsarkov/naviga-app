## Summary

Implements **#417**: persisted seq16 + narrow NVS adapter, with boot restore semantics aligned to canon (rx_semantics_v0 §5.3).

**Umbrella:** #416. **Historical trackers:** #296, #355.

## Behavior

- **Boot:** Read persisted seq16 from platform storage (`naviga_storage`). If restored value > 0, first TX after reboot uses `restored_seq16 + 1`. If absent or zero, fresh-start preserved (first seq16 = 1).
- **Persistence:** Per-TX write policy — after each successful send, persist the seq16 that was **actually sent** (read from the sent frame payload at Common offset 7–8). Validity is separate from the numeric value so **seq16 == 0 after wraparound** is valid and persisted (no sentinel-zero behavior).

## Adapter boundary

- **Platform** (`naviga_storage`): `load_seq16` / `save_seq16`; NVS/Preferences details stay here. Namespace `"naviga"`, key `"seq16"`.
- **Domain** (`BeaconLogic`): `set_initial_seq16(value)` only; no platform includes or storage logic.
- **Runtime** (`M1Runtime`): Forwards `set_initial_seq16`; exposes **`get_last_sent_seq16(uint16_t* out) const`** — returns true iff a packet was successfully sent and writes its seq16 (including 0). Validity tracked via `has_last_sent_seq16_`; value in `last_sent_seq16_`. Set from sent frame in `handle_tx()` on success.
- **App** (`AppServices`): Restores after `runtime_.init()`; persists when `get_last_sent_seq16(&sent)` returns true and (first time or `sent != last_persisted_seq16_`). Uses `has_persisted_seq16_` so the first sent value (including 0) is always persisted.

## Write policy (chosen for this PR)

**Per-TX persistence.** Rationale: safest minimal behavior for power-loss; no flush timers or batching; one key, one value. Documented in code comments and here. Batching can be a follow-up if needed.

## Wraparound (seq16 == 0)

seq16 is a 16-bit counter; after 65535 the next value is 0. That value is **valid** and must be persisted. The implementation separates “has a last-sent value” from the value itself: `get_last_sent_seq16(out)` returns false only when no successful TX has happened yet; when it returns true, the stored value (including 0) is persisted. App layer uses `has_persisted_seq16_` so the first persistence (including 0) is never skipped.

## Tests

- `test_seq16_default_first_packet_is_one` — default start ⇒ first packet seq16 = 1.
- `test_seq16_set_initial_then_first_packet_is_initial_plus_one` — set_initial_seq16(42) ⇒ first packet seq16 = 43.
- `test_seq16_set_initial_100_first_packet_is_101` — set_initial_seq16(100) ⇒ first packet seq16 = 101.
- **`test_seq16_wraparound_first_packet_is_zero`** — set_initial_seq16(65535) ⇒ first packet seq16 = 0 (wraparound; valid and persisted).

71 beacon_logic tests pass; devkit build succeeds.

## Non-goals (unchanged)

- No NodeTable snapshot/restore (#418), no RX semantics (#421), no TX scheduling redesign (#420/#422), no BLE, no broad storage abstraction.

Closes #417.
