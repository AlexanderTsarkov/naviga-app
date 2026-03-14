## #422 completed (Path B)

**Chosen path:** **Path B** — v0.1 packet family retained; canon-aligned behavior via throttle/merge rules. No wire migration, no RX changes.

**Behavioral changes:**
- **No hitchhiking:** Operational and Informative are not enqueued in the same formation pass as Core or Alive.
- **At most one status per pass:** When status is due, at most one of Op or Info is enqueued per formation pass (prefer Op if has_battery/has_uptime, else Info).
- **Status throttle:** `min_status_interval_ms` = 30s (anti-burst), `T_status_max` = 300s (bounded periodic refresh). Status only enqueued when cadence is due (`time_for_min` or `time_for_silence`), then subject to throttle and bootstrap.
- **Bootstrap:** Up to 2 status sends allowed under bootstrap rules; `BeaconLogic::on_status_sent(now_ms)` called from M1Runtime when a TAIL2 or INFO frame is successfully sent.

**Validation:**
- `test_beacon_logic` 73/73 passed.
- `pio run -e devkit_e220_oled` passed.

**Explicit:** No v0.2 redesign in this issue (Node_Pos_Full, Node_Status not introduced). hw_profile_id / fw_version_id remain uint16.

**P0 chain:** After this PR is merged and #422 is closed, the P0 chain (#417–#422) is complete.
