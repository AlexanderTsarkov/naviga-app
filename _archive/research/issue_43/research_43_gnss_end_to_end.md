# Issue #43 Research: GNSS Position Flow End-to-End

**Purpose:** Repo-wide research of current GNSS position flow (firmware → domain/radio → BLE → mobile) and proposal for minimal integration of real u-blox NEO-M8N GNSS via UART, while keeping SIM GNSS as a build-time dev tool.

**Inputs:** Hardware pins GPS_TX→GPIO15, GPS_RX→GPIO16. GNSS v0 target: `pos_valid` + `fix_type` (NO_FIX/FIX_2D/FIX_3D); optional: sats, hdop, age_ms.

---

## 1. Current State — File Pointers

### 1.1 GNSS abstraction and contracts

| Location | Role |
|----------|------|
| `firmware/lib/NavigaCore/include/naviga/hal/interfaces.h` | `IGnss` interface, `GnssSnapshot` struct (has_fix, lat_e7, lon_e7) |
| `docs/firmware/hal_contracts_v0.md` § 3 | Contract spec: fix_state, pos_valid, last_fix_ms (not all present in code) |
| `docs/firmware/gnss_v0.md` | GNSS v0 behavior and data model |
| `docs/adr/ootb_position_source_v0.md` | ADR: position source = dongle GNSS |

**Actual GnssSnapshot (interfaces.h):**

```cpp
struct GnssSnapshot {
  bool has_fix;
  int32_t lat_e7;
  int32_t lon_e7;
};
```

**Doc contract (hal_contracts_v0.md):** `fix_state` (GNSSFixState), `pos_valid`, `lat_e7`, `lon_e7`, `last_fix_ms`. **Gap:** implementation has only `has_fix`; no `last_fix_ms`; no explicit `fix_state` enum.

### 1.2 Where GNSS is composed (not M1Runtime)

**GNSS is not injected into M1Runtime.** Composition happens in `AppServices`:

| File | Role |
|------|------|
| `firmware/src/app/app_services.cpp` | Uses `GnssStubService gnss_stub` directly (line 27). `init()` calls `gnss_stub.init(full_id)`. `tick()`: `gnss_stub.tick()` → `gnss_stub.latest()` → `self_policy.evaluate()` → `runtime_.set_self_position()` |
| `firmware/src/services/gnss_stub_service.h` | `GnssStubService` with `GnssSample` (has_fix, lat_e7, lon_e7) |
| `firmware/src/services/gnss_stub_service.cpp` | Deterministic random walk, 1 Hz, base Rostov Veliky |
| `firmware/src/services/self_update_policy.cpp` | Cadence: min_movement_m=25m, max_silence=72s; drives when to update self position |

**M1Runtime** never touches GNSS; it receives position via `set_self_position(bool pos_valid, lat_e7, lon_e7, pos_age_s, now_ms)`.

### 1.3 GNSS emulation/stub details

| Aspect | Current behavior |
|--------|------------------|
| **File** | `firmware/src/services/gnss_stub_service.cpp` |
| **Output** | `GnssSample`: has_fix, lat_e7, lon_e7 (no fix_state, no last_fix_ms, no pos_age) |
| **Fix simulation** | `kForceNoFix = false` — always has_fix. Optional NO_FIX documented in `docs/firmware/gnss_stub_v0.md` but not implemented |
| **Update rate** | 1 Hz (once per second) |
| **Movement** | LCG-based random walk; step 5–40 m; base lat/lon Rostov Veliky |
| **Enable** | Always on when `gnss_stub` is used. No build-time switch between stub and real GNSS |

### 1.4 Build-time profiles

| Profile | Define | Pins | GNSS |
|---------|--------|------|------|
| `devkit_e220_oled` | `HW_PROFILE_DEVKIT_E220_OLED` | gps_rx=-1, gps_tx=-1 | caps.has_gnss=false |
| `devkit_e220_oled_gnss` | `HW_PROFILE_DEVKIT_E220_OLED_GNSS` | gps_rx=16, gps_tx=15 | caps.has_gnss=true |

- **Files:** `firmware/src/hw_profiles/devkit_e220_oled.cpp`, `devkit_e220_oled_gnss.cpp`, `get_hw_profile.cpp`
- Both profiles currently use the same `gnss_stub` — no real GNSS path exists.

---

## 2. Position data propagation

### 2.1 Firmware internal

| Step | File | Flow |
|------|------|------|
| 1 | `app_services.cpp` | `gnss_stub.tick()` → `gnss_stub.latest()` → GnssSample |
| 2 | `app_services.cpp` | `self_policy.evaluate(now_ms, sample)` → SelfUpdateDecision |
| 3 | `app_services.cpp` | On decision: `runtime_.set_self_position(has_fix, lat_e7, lon_e7, 0, now_ms)` |
| 4 | `m1_runtime.cpp` | `set_self_position()` → `node_table_.update_self_position()` or `touch_self()`; updates `self_fields_` (GeoBeaconFields) |
| 5 | `domain/node_table.cpp` | Self entry: pos_valid, lat_e7, lon_e7, pos_age_s, last_seen_ms |

**Data model (self):** `domain::NodeEntry` / `GeoBeaconFields`: pos_valid, lat_e7, lon_e7, pos_age_s.

### 2.2 Radio / beacon packet

| File | Structure |
|------|-----------|
| `firmware/protocol/geo_beacon_codec.h` | `GeoBeaconFields`: node_id, pos_valid (u8), lat_e7, lon_e7, pos_age_s, seq |
| `firmware/protocol/geo_beacon_codec.cpp` | GEO_BEACON v1: 24 bytes; pos_valid at offset 11 |
| `firmware/src/domain/beacon_logic.cpp` | `build_tx()` uses self_fields_; `on_rx()` decodes → `table.upsert_remote(pos_valid, lat_e7, lon_e7, ...)` |

**Docs:** `docs/protocols/ootb_radio_v0.md` § 3.2 — GEO_BEACON core: node_id, pos_valid, position, pos_age_s, seq.

### 2.3 Domain NodeTable

| File | Fields |
|------|--------|
| `firmware/src/domain/node_table.h` | `NodeEntry`: pos_valid, lat_e7, lon_e7, pos_age_s, last_seen_ms |
| `firmware/src/domain/node_table.cpp` | `get_snapshot_page()` writes NodeRecord v1 (flags bit1 = pos_valid) |

### 2.4 BLE snapshot layout

| Char | Spec | Implementation |
|------|------|-----------------|
| DeviceInfo | `docs/protocols/ootb_ble_v0.md` § 1.1 | `ble_node_table_bridge.cpp` → `update_device_info()` |
| Health | § 1.2: uptime_s, gnss_state, pos_valid, pos_age_s, battery, radio stats | **Not implemented** — BleNodeTableBridge has no `update_health()`; BleEsp32Transport has no `set_health()` |
| NodeTableSnapshot | § 1.3 | `update_node_table()` → NodeRecord v1 |

**NodeRecord v1 (26 bytes):** node_id, short_id, flags (bit1=pos_valid), last_seen_age_s, lat_e7, lon_e7, pos_age_s, last_rx_rssi, last_seq.

### 2.5 Mobile parsing and map

| File | Role |
|------|------|
| `app/lib/features/connect/connect_controller.dart` | `BleNodeTableParser.parsePage()` — extracts posValid from flags bit1 |
| `app/lib/features/map/map_screen.dart` | `_nodesWithValidPos()`: `r.posValid` && non-zero lat/lon && sanity check (-90..90, -180..180) |
| `app/lib/features/nodes/node_table_cache.dart` | `NodeRecordV1` with `posValid` |

**Mobile does not simulate GNSS** — it only displays what comes from NodeTable via BLE. Self position is included in NodeTable when firmware has valid position.

---

## 3. Proposed target v0 contract

### 3.1 GnssSnapshot v0 (aligned with docs)

| Field | Type | Description |
|-------|------|-------------|
| fix_state | GNSSFixState | NO_FIX / FIX_2D / FIX_3D |
| pos_valid | bool | true iff FIX_2D or FIX_3D and coordinates usable for beacon |
| lat_e7 | int32 | Latitude × 1e7 |
| lon_e7 | int32 | Longitude × 1e7 |
| last_fix_ms | uint32 | Uptime ms of last valid fix; for pos_age_s = (now_ms - last_fix_ms)/1000 |

**Optional (low-cost):** sat_count, hdop, age_ms — only if trivial to obtain from module.

### 3.2 IGnss integration point

- **Inject** `IGnss*` into the app layer (e.g. `AppServices` or a thin GNSSTask equivalent).
- **Replace** direct `GnssStubService` usage with `IGnss::get_snapshot()`.
- **Build-time switch:** `GNSS_PROVIDER_STUB` (default for dev) vs `GNSS_PROVIDER_UBLOX` — selects implementation.

---

## 4. Recommended integration points

### 4.1 Firmware: UART read + parse

- **Pins:** GPS_TX (module) → MCU RX = GPIO16 (`gps_rx`); GPS_RX (module) → MCU TX = GPIO15 (`gps_tx`).
- **Protocol:** Prefer **UBX** over NMEA for embedded 1 Hz:
  - Binary, fixed layout (e.g. NAV-PVT).
  - Direct numeric values (lat, lon, fix type, num sat, hdop).
  - Configurable output rate via CFG-RATE.
  - NMEA is ASCII and heavier to parse.
- **New file:** `firmware/src/platform/ublox_m8n_gnss.cpp` (or `drivers/`) — UART init, UBX parse, map to GnssSnapshot.
- **Map:** UBX fix type → GNSSFixState; NAV-PVT has numSat, hdop, age (if needed).

### 4.2 Domain / radio

- **GeoBeaconFields** and **NodeTable** already carry pos_valid, lat_e7, lon_e7, pos_age_s — no structural changes.
- **GnssSnapshot → self_fields_:** Ensure `pos_age_s` is computed from `last_fix_ms` when real GNSS is used (stub currently passes 0).

### 4.3 BLE

- **Implement Health characteristic** per spec: gnss_state (enum), pos_valid, pos_age_s. Source: status provider fed by IGnss.
- **Add:** `IBleTransport::set_health()` (or equivalent) and wire from BleNodeTableBridge / status aggregation.
- NodeTableSnapshot already carries pos_valid; no layout change.

### 4.4 Mobile

- **No changes** — pos_valid and coords already drive map via NodeRecord v1.
- **Constraint:** Mobile must not become the GNSS simulator; all position comes from dongle.

### 4.5 Build-time switch

| Option | Mechanism |
|--------|------------|
| **Preferred** | `build_flags`: `-DGNSS_PROVIDER_STUB` (default) or `-DGNSS_PROVIDER_UBLOX` |
| **Alternative** | `platformio.ini` env `devkit_e220_oled` (stub) vs `devkit_e220_oled_gnss_real` (ublox) — only build ubx impl for GNSS profile |

---

## 5. Risks and unknowns

| Risk | Mitigation |
|------|-------------|
| UBX init/config at boot | May need CFG-RATE / CFG-MSG to set 1 Hz; document or script for factory config |
| Cold start / TTFF | Real GNSS can be slow; UX may show "no fix" for minutes; logging and BLE Health help |
| Power | M8N draw; check against power budget if battery-operated |
| UART conflict | LoRa E220 uses different UART; ensure GPS uses dedicated UART (e.g. UART2) |
| GnssSnapshot vs GnssSample | Unify on GnssSnapshot with fix_state/pos_valid/last_fix_ms; migrate GnssStubService and SelfUpdatePolicy to produce/consume it |
| Health not implemented | Extra work to add Health characteristic and wire gnss_state; can be phased |

---

## 6. Draft implementation sub-issues (A–D)

### Sub-issue A: GNSS HAL alignment and build switch

**Scope IN**

- Extend `GnssSnapshot` (interfaces.h) with `fix_state`, `pos_valid`, `last_fix_ms`; keep `lat_e7`, `lon_e7`.
- Add `GNSSFixState` enum (NO_FIX, FIX_2D, FIX_3D).
- Migrate `GnssStubService` to output full `GnssSnapshot` (or `GnssSample` → snapshot mapping in app layer).
- Introduce build flag `GNSS_PROVIDER_STUB` | `GNSS_PROVIDER_UBLOX`; stub is default.
- Refactor `AppServices` to use `IGnss*` (injected) instead of `GnssStubService` directly.

**Scope OUT**

- Real UART driver.
- BLE Health implementation.
- Mobile changes.

**Files:** `interfaces.h`, `gnss_stub_service.*`, `app_services.*`, `platformio.ini`, `get_hw_profile.cpp` (if needed for capability).

---

### Sub-issue B: u-blox NEO-M8N UART driver

**Scope IN**

- New `UbloxM8nGnss` (or similar) implementing `IGnss`.
- UART init on GPIO15 (TX), GPIO16 (RX) using HW profile pins.
- UBX NAV-PVT (or NAV-POSLLH + NAV-STATUS) parsing at 1 Hz.
- Map fix type → GNSSFixState; fill lat_e7, lon_e7, last_fix_ms, pos_valid.
- Optional: sat_count, hdop if trivial from NAV-PVT.
- Compile only when `GNSS_PROVIDER_UBLOX` is set.
- Unit test with canned UBX payloads (no hardware).

**Scope OUT**

- CFG-RATE / CFG-MSG configuration (assume module pre-configured or document manually).
- NMEA parsing.
- Multi-constellation / advanced features.

**Files:** New `ublox_m8n_gnss.*`, `hw_profile.h` (pins already present for gnss profile).

---

### Sub-issue C: BLE Health and GNSS status

**Scope IN**

- Add Health characteristic support to `BleEsp32Transport` (`set_health` or equivalent).
- Extend `BleNodeTableBridge` (or status provider) to aggregate: uptime_s, gnss_state, pos_valid, pos_age_s, battery_mv (stub if no ADC), radio stats.
- Source gnss_state, pos_valid, pos_age_s from `IGnss::get_snapshot()` or cached snapshot.
- Implement binary layout per `docs/protocols/ootb_ble_v0.md` § 1.2.
- Seed: READ + NOTIFY on gnss_state change (per spec).

**Scope OUT**

- Full battery ADC integration (placeholder ok).
- Config characteristic.
- Mobile Health UI (can read raw; UI polish separate).

**Files:** `ble_esp32_transport.*`, `ble_node_table_bridge.*`, protocol spec.

---

### Sub-issue D: Integration and verification

**Scope IN**

- Wire `UbloxM8nGnss` as `IGnss` when `GNSS_PROVIDER_UBLOX` + `HW_PROFILE_DEVKIT_E220_OLED_GNSS`.
- Ensure `SelfUpdatePolicy` and `set_self_position` receive correct `pos_age_s` from real GNSS (last_fix_ms → age).
- Verify: beacon TX with pos_valid=0 when NO_FIX; pos_valid=1 when FIX; coordinates in NodeTable and BLE.
- Manual test: real M8N, outdoor fix, BLE NodeTable refresh, map display.
- Doc: `docs/firmware/gnss_ublox_m8n_v0.md` — pins, UBX messages, build envs.

**Scope OUT**

- Automated hardware-in-loop tests.
- NMEA fallback.
- Accuracy/HDOP filtering logic (v0 accepts first valid fix).

**Files:** `app_services.*`, `main.cpp` / app init, new doc.

---

## 7. Summary

| Layer | Current | Change |
|-------|---------|--------|
| **Firmware HAL** | GnssSnapshot minimal; GnssStubService used directly | Extend snapshot; inject IGnss; add UbloxM8nGnss impl |
| **Domain/radio** | pos_valid, coords already in place | Compute pos_age_s from last_fix_ms when real GNSS |
| **BLE** | DeviceInfo, NodeTableSnapshot only | Add Health with gnss_state, pos_valid, pos_age_s |
| **Mobile** | Uses posValid from NodeRecord | No change |
| **Build** | HW profile selects pins; both use stub | Add GNSS_PROVIDER_STUB | UBLOX switch |

**Minimal path:** A (HAL + build switch) → B (UART driver) → D (integration). C (Health) can follow or run in parallel.
