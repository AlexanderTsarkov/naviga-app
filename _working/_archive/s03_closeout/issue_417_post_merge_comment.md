**Implemented in PR #428.** Summary:

**What was implemented**
- **Platform** (`naviga_storage`): `load_seq16(uint16_t* out)` / `save_seq16(uint16_t value)` in namespace `"naviga"`, key `"seq16"`. NVS/Preferences stay in platform only.
- **Domain** (`BeaconLogic`): `set_initial_seq16(value)` so next `next_seq16()` returns `value + 1`. No platform/storage in domain.
- **Runtime** (`M1Runtime`): Forwards `set_initial_seq16`; exposes `get_last_sent_seq16(uint16_t* out) const` (validity separate from value; seq16 0 after wraparound is valid). Value set from sent frame in `handle_tx()` on success.
- **App** (`AppServices`): Restore after `runtime_.init()` (if restored > 0, `set_initial_seq16(restored)`). Per-TX persistence: when `get_last_sent_seq16(&sent)` returns true and (first time or `sent != last_persisted_seq16_`), call `save_seq16(sent)`. Uses `has_persisted_seq16_` so first sent value (including 0) is always persisted.

**Adapter boundary:** Platform → storage only; domain → setter only; runtime → forwarding + validity-aware accessor; app → restore + persist wiring.

**Write policy:** Per successful TX (no batching).

**Restore behavior:** Restored value > 0 ⇒ first TX after reboot uses `restored + 1`. Absent/zero ⇒ fresh start (first seq16 = 1).

**Wraparound:** seq16 == 0 after wrap is valid; validity is separate from numeric value so 0 is persisted correctly (no sentinel-zero).

**Tests:** `test_seq16_default_first_packet_is_one`, `test_seq16_set_initial_then_first_packet_is_initial_plus_one`, `test_seq16_set_initial_100_first_packet_is_101`, `test_seq16_wraparound_first_packet_is_zero`. 71 beacon_logic tests pass; devkit build OK.

Refs #296, #355.
