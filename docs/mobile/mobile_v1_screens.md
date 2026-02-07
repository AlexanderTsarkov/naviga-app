# Mobile v1 (Android-first, Flutter) — screens & scope

**Status:** Draft (v1)  
**Target:** Android-first (iOS follow-up later)  
**Source of truth:** BLE v0 spec + NodeTable v0

---

## 1) MVP scope

**Goal:** A minimal, reliable Android Flutter app that connects to the dongle over BLE and visualizes the NodeTable + map, aligned with current firmware/BLE capabilities.

**Tabs (bottom nav):** Connect · My Node · Nodes · Map · Settings

**Key screens:**
- **Node Details** (reachable from Nodes list, with “Show on map”).

---

## 2) Non-goals (explicitly out of scope)

- Messages / chat
- Tracks / routes / breadcrumbs
- Offline maps (tile packs, caching)
- BLE config/write actions (channel/power/profile changes)
- Multi-device management or ownership/claiming flows

---

## 3) Data scope (BLE-only, read-only)

App uses **only data exposed by the connected node**:

**DeviceInfo (READ):**
- Identity: `node_id`, `short_id`
- Radio info: band, channel range, current channel, network mode
- Capabilities: GNSS / radio flags
- Firmware version, module model

**Health (READ + NOTIFY):**
- GNSS state, position validity + age
- Battery (mV), radio RX/TX counters, last RSSI
- Uptime-based time fields only

**NodeTableSnapshot (READ pages, no NOTIFY):**
- Paginated snapshot of NodeTable records
- `last_seen_age_s`, `pos_valid`, `pos_age_s`, `last_rx_rssi`, `short_id_collision`

**Important constraints:**
- No BLE writes/config in v1.
- NodeTable updates are **pull-based** (manual refresh / periodic polling).

---

## 4) Screens

### 4.1 Connect
**Purpose:** Pairing and connection state.

**Content:**
- Permissions (Bluetooth, Location as required by Android)
- Scan list (filtered to Naviga dongles)
- Connection state machine: scanning → connecting → connected → reconnecting
- DeviceInfo preview after connection (model, band, channel, firmware)

**Primary actions:**
- Scan / Stop
- Connect / Disconnect
- Retry / Reconnect

---

### 4.2 My Node
**Purpose:** “Self” telemetry from the connected node.

**Content:**
- Identity: short_id + full node_id (copy)
- GNSS state, pos_valid, pos_age
- Battery (mV), uptime, last RSSI
- Radio counters (TX/RX)

**Notes:**
- All values read-only; no settings changes.

---

### 4.3 Nodes
**Purpose:** NodeTable list.

**List item fields:**
- short_id (collision indicator if needed)
- last_seen_age_s
- pos_valid + pos_age_s
- last_rx_rssi

**Actions:**
- Tap → **Node Details**
- Pull to refresh (new snapshot)

---

### 4.4 Node Details
**Purpose:** Focused view of a single node.

**Content:**
- Identity (short_id + full node_id)
- last_seen_age_s
- pos_valid + pos_age_s
- last_rx_rssi
- Location (lat/lon if pos_valid)

**Actions:**
- “Show on map” (centers map on node marker)

---

### 4.5 Map
**Purpose:** Spatial view of nodes.

**Map SDK:** `flutter_map` (online tiles only for v1)

**Markers:**
- Nodes with `pos_valid = true`
- Highlight “self” marker if available
- Marker subtitle: short_id + pos_age_s

**Notes:**
- No offline tiles, no tracks.
- If `pos_valid = false`, node stays list-only.

---

### 4.6 Settings
**Purpose:** App-level settings only.

**Content:**
- About (app version, device info summary)
- Map source (read-only for v1; default OSM)
- Debug / diagnostics toggle (logs on/off)
- Legal / OSS notices

**Notes:**
- No BLE config or channel/power controls in v1.

---

## 5) UX/behavior notes

- **Android-first** UI/permissions flow; iOS parity later.
- **Local cache (lightweight):** store last successful NodeTable snapshot for continuity across reconnects; no historical timeline or tracks.
- **Error states:** clear UX for “no BLE”, “no permissions”, “not connected”.

---

## 6) Definition of Done (doc-level)

- All screens above are defined with minimal fields/actions.
- Data scope matches current BLE v0 contract.
- Non-goals explicitly documented and enforced in issues.
