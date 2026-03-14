# GNSS Capabilities Map v0

**Purpose:** Map u-blox NEO-M8N capabilities to Naviga layers. Issue [#43](https://github.com/AlexanderTsarkov/naviga-app/issues/43).

**Related:** [gnss_v0.md](gnss_v0.md), [hal_contracts_v0.md](hal_contracts_v0.md) § 3, [ootb_ble_v0.md](../protocols/ootb_ble_v0.md) § 1.2.

---

## 1. u-blox NEO-M8N — what it provides

### 1.1 Primary source: UBX NAV-PVT

| Field | Type | Description | Must-have (v0) | Future-useful |
|-------|------|-------------|----------------|---------------|
| **fixType** | uint8 | 0=no fix, 2=2D, 3=3D, 4=GNSS+DR | ✓ | — |
| **lat** | int32 | Latitude × 1e-7 | ✓ | — |
| **lon** | int32 | Longitude × 1e-7 | ✓ | — |
| **flags** (GNSS_FIX_OK) | bit | Valid fix per DOP/accuracy masks | ✓ | — |
| **numSV** | uint8 | Satellites in solution | — | ✓ (diagnostics) |
| **pDOP** | uint16 | Position DOP × 0.01 (no separate HDOP) | — | ✓ (diagnostics) |
| **hAcc** | uint32 | Horizontal accuracy estimate, mm | — | ✓ (quality filter) |
| **vAcc** | uint32 | Vertical accuracy estimate, mm | — | ✓ |
| **year/month/day** | — | Date (for time-only features) | — | Future |
| **hour/min/sec** | — | Time (for NTP sync, logging) | — | Future |

### 1.2 Alternative: NMEA

- **RMC / GGA:** ASCII; lat/lon, fix type, HDOP, sat count.
- **Use:** Human-readable, widely compatible; not recommended for embedded 1 Hz (parsing overhead).

---

## 2. Where each field lives in Naviga

| Field | GnssSnapshot | Radio / GEO_BEACON | BLE Health | NodeTable / BLE NodeRecord | Mobile |
|-------|--------------|--------------------|------------|----------------------------|--------|
| **fix_state** (NO_FIX/2D/3D) | ✓ (enum) | — | ✓ (gnss_state enum) | — | — |
| **pos_valid** | ✓ (bool) | ✓ (1 bit/byte) | ✓ | ✓ (flags bit1) | ✓ (posValid) |
| **lat_e7, lon_e7** | ✓ | ✓ | — | ✓ | ✓ (latE7, lonE7) |
| **last_fix_ms** | ✓ | — | — | — | — |
| **pos_age_s** | Derived | ✓ | ✓ | ✓ | ✓ |
| **sat_count** | Optional (v0) | — | Optional (future) | — | — |
| **hdop / pDOP** | Optional (v0) | — | Optional (future) | — | — |
| **hAcc** | — | — | — | — | — |

### 2.1 GnssSnapshot (firmware HAL)

**File:** `firmware/lib/NavigaCore/include/naviga/hal/interfaces.h`

- **fix_state** — GNSSFixState enum (NO_FIX, FIX_2D, FIX_3D). Map from UBX fixType 0/2/3.
- **pos_valid** — true iff FIX_2D or FIX_3D and coordinates sane.
- **lat_e7, lon_e7** — Direct from NAV-PVT lat/lon (already ×1e7).
- **last_fix_ms** — Uptime when fix acquired; used to compute pos_age_s.
- **Optional:** sat_count, hdop (from pDOP) — diagnostics only; low priority for v0.

### 2.2 Radio / GEO_BEACON

**File:** `firmware/protocol/geo_beacon_codec.h`, `firmware/src/domain/beacon_logic.cpp`

- **pos_valid** — Beacon includes coords only when pos_valid; no fix_state on air.
- **lat_e7, lon_e7, pos_age_s** — Already defined; no protocol change.

### 2.3 BLE Health characteristic

**File:** `docs/protocols/ootb_ble_v0.md` § 1.2

- **gnss_state** — NO_FIX / FIX_2D / FIX_3D (from fix_state).
- **pos_valid** — Bool for beacon usability.
- **pos_age_s** — Age in seconds.
- **Optional:** sat_count, hdop — extension bytes or future format_ver bump.

### 2.4 NodeTable / NodeRecord

**Files:** `firmware/src/domain/node_table.cpp`, `firmware/protocol/ble_node_table_bridge.cpp`

- **pos_valid** — flags bit1 in NodeRecord v1.
- **lat_e7, lon_e7, pos_age_s** — Already in record layout.

### 2.5 Mobile

**Files:** `app/lib/features/connect/connect_controller.dart`, `app/lib/features/map/map_screen.dart`

- **posValid** — From NodeRecord flags; drives map marker display.
- **latE7, lonE7** — For marker position. No change needed for real GNSS.

---

## 3. Summary

| Layer | v0 must-have | v0 optional |
|-------|--------------|--------------|
| **GnssSnapshot** | fix_state, pos_valid, lat_e7, lon_e7, last_fix_ms | sat_count, hdop |
| **GEO_BEACON** | pos_valid, lat/lon, pos_age_s | — |
| **BLE Health** | gnss_state, pos_valid, pos_age_s | sat_count, hdop |
| **NodeTable / Mobile** | No change | — |

SIM GNSS (stub) produces the same GnssSnapshot fields; real GNSS fills them from UBX. Build-time switch selects provider; domain/radio/BLE/mobile stay provider-agnostic.
