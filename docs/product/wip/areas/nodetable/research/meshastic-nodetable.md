# Research: Meshtastic NodeDB / NodeTable

Research from **local Meshtastic snapshot** for Issue #147. Used to validate and expand the Naviga NodeTable Product Contract v0.

---

## 1) Version pinning

- **Parent folder:** `Meshtastic/` (no .git at root — snapshot).
- **Firmware:** `Meshtastic/meshtastic-firmware/` — **commit `fb757f0314b3afbca79a23285979b36838aa45af`** (merge naviga-custom into naviga-update-2.7.13).
- **Android:** `Meshtastic/meshtastic-android/` — **commit `8287203db4b737b217efe3b5624f4003fba44f70`** (chore: update androidx navigation to v1.0.0-beta01).
- **iOS:** `Meshtastic/meshtastic-ios/` — **commit `51c8e16879a19cd6fa0b0808ff79690ab5b4c749`** (merge twoway-traceroute).
- **Research date:** 2025-02-13 (from local snapshot).

---

## 2) Where the node database lives

| Layer   | Location | Notes |
|---------|----------|--------|
| **Firmware** | `meshtastic-firmware/src/mesh/NodeDB.h`, `NodeDB.cpp` | Class `NodeDB`; holds `meshtastic_NodeDatabase` (vector of `meshtastic_NodeInfoLite`). Persisted to `/prefs/nodes.proto`. `NodeInfoModule` sends/receives User (port NODEINFO_APP); `PositionModule` / `DeviceTelemetry` call `updatePosition` / `updateTelemetry`. |
| **Firmware (update entry point)** | `MeshService.cpp` (every RX): `nodeDB->updateFrom(*mp)`; `FloodingRouter.cpp`: `nodeDB->updateFrom(*p)`. Position/User/Telemetry handled in their modules, which call `nodeDB->updatePosition`, `updateUser`, `updateTelemetry`. |
| **Android** | `core/database/`: `NodeInfoDao`, `MeshtasticDatabase.kt`; `core/data/`: `NodeRepository.kt`. Model: `core/database/model/Node`, `NodeEntity`; `core/model/NodeInfo.kt`. UI: `feature/node/` (NodeItem, NodeDetailsSection, LastHeardInfo), map (BaseMapViewModel, lastHeardFilter). |
| **iOS** | Protobufs: `MeshtasticProtobufs/Sources/meshtastic/mesh.pb.swift`, `deviceonly.pb.swift` — `NodeInfo` (includes `lastHeard`). Widgets reference `NodeInfoView` (nodesOnline, totalNodes). |

---

## 3) Categories of knowledge (summary)

Grouped by what Meshtastic tracks per node, not raw field dumps.

- **Identity & user/profile**  
  Node number (`num`, uint32 — primary key on mesh). User: `UserLite` — `long_name`, `short_name`, `macaddr` (6 bytes), `hw_model` (enum), `is_licensed`, `role`, `public_key`, `is_unmessagable`. Broadcast via NODEINFO_APP (User payload).

- **Device / HW & firmware**  
  `UserLite.hw_model` (HardwareModel enum: TBEAM, HELTEC, etc.). Full `User` (in NodeInfo, not only Lite) can carry more; firmware version typically from MyNodeInfo / config, not per-remote-node in NodeInfoLite.

- **Position**  
  `PositionLite`: `latitude_i`, `longitude_i` (1e-7 deg), `altitude` (m), `time` (position time), `location_source` (LocSource enum). Updated from POSITION_APP packets.

- **Telemetry / health**  
  `NodeInfoLite.device_metrics` (DeviceMetrics): battery_level, voltage, channel_utilization, air_util_tx, uptime_seconds. Updated from TELEMETRY_APP (DeviceTelemetry module).

- **Radio / channel context**  
  `snr` (float, last RX SNR), `channel` (index on which the node was heard), `via_mqtt` (true if packet came via MQTT). No explicit tx power or “radio profile” id in NodeInfoLite; that lives in config/channel, not per-node DB.

- **Routing / mesh metrics**  
  `hops_away` (hops from us), `next_hop` (last byte of node num for next hop). Set from packet `hop_start` / `hop_limit` when applicable.

- **Activity / last-heard**  
  `last_heard` (uint32, seconds since epoch or RTC time of last received packet). Updated on **every** decoded packet from that node (`updateFrom` sets `info->last_heard = mp.rx_time`). No separate TTL field; “online” is derived (see below).

- **Roles**  
  `UserLite.role` (Config_DeviceConfig_Role): CLIENT, ROUTER, REPEATER, TRACKER, SENSOR, TAK, etc. Affects behavior (e.g. REPEATER does not originate NodeInfo/Position/Telemetry).

- **Security / trust**  
  `bitfield` (LSB: is_key_manually_verified). `public_key` in UserLite for PKC. Favorite/ignored: `is_favorite`, `is_ignored` (local-only, persist across cleanups).

---

## 4) Activity model

- **Stored:** Only `last_heard` (timestamp of last RX from that node). No explicit “state” enum stored.
- **Derived “online”:** Firmware: `sinceLastSeen(n) = now - n->last_heard` (with clock skew guard). `getNumOnlineMeshNodes()` counts nodes where `sinceLastSeen(&node) < NUM_ONLINE_SECS`. **NUM_ONLINE_SECS = 2 hours** (defined in `NodeDB.cpp`).
- **Android:** `onlineTimeThreshold() = now_sec - 2*60*60`; node is “online” if `lastHeard > onlineTimeThreshold()`. Same 2‑hour threshold. Map: `lastHeardFilter` / `lastHeardTrackFilter` (user-configurable) for filtering nodes on map.
- **Events that update last_heard:** Any decoded mesh packet from that node: `NodeDB::updateFrom(mp)` runs on every RX; if `mp.rx_time` is set, `info->last_heard = mp.rx_time`. So Position, NodeInfo, Telemetry, text, routing, etc. all refresh last_heard. No separate “heartbeat” packet type.

---

## 5) Update sources (packet type → categories)

| Packet / port | Updates | Notes |
|---------------|---------|--------|
| **Any decoded** (updateFrom) | last_heard, snr, via_mqtt, hops_away | Every RX from that node. |
| **POSITION_APP** (port 3) | position (lat, lon, alt, time, location_source) | PositionModule calls `nodeDB->updatePosition(from, p)`. |
| **NODEINFO_APP** (port 4) | user (long_name, short_name, hw_model, role, etc.) | NodeInfoModule calls `nodeDB->updateUser(from, p, channel)`. |
| **TELEMETRY_APP** (port 67) | device_metrics (battery, voltage, channel_util, air_util_tx, uptime_seconds) | DeviceTelemetry calls `nodeDB->updateTelemetry(from, t)`. |
| **Local / phone** | position (self), user (owner), telemetry (self) | MeshService: updatePosition(getNodeNum(), pos, RX_SRC_LOCAL), updateUser(getNodeNum(), owner); DeviceTelemetry: updateTelemetry(getNodeNum(), telemetry, RX_SRC_LOCAL). |

---

## 6) “Why it exists” + transferability to Naviga

Naviga NodeTable contract v0 already has: Identity (DeviceId, ShortId), Human naming (networkName, localAlias), Activity (derived states, lastSeenAge), Position (2D + quality), Capabilities/HW (hwType, fwVersion), Radio context (lastSeenRadioProfile), and the 7 questions. For each Meshtastic category **not** fully explicit in that contract:

| Category | What it enables in Meshtastic | Transferability | Rationale |
|----------|------------------------------|------------------|------------|
| **Identity (node num vs long/short name)** | Node picker, routing, favorites, messaging. | **YES** | Naviga already has DeviceId + ShortId + naming; Meshtastic’s node num is analogous to a short runtime id; we keep DeviceId as primary. |
| **Human naming (long_name / short_name)** | List/map labels, canned messages. | **YES** | Aligns with Naviga networkName + localAlias + display precedence. |
| **Activity / last_heard (single timestamp, 2 hr “online”)** | Online count, list sort, map filter, “last seen X ago”. | **YES** | Naviga’s lastSeenAge + derived states (Online/Uncertain/Stale/Archived) can use same idea; thresholds can differ. |
| **Position + location_source** | Map, waypoints, “where from” (GPS vs network). | **YES** | Naviga PositionQuality “source-tagged” matches; 2D + optional altitude already in contract. |
| **DeviceMetrics (battery, voltage, uptime, channel_util)** | Battery icon, diagnostics, “who is online” quality. | **MAYBE** | Naviga Capabilities/HW + telemetry optional; add only if we need device health in UI. |
| **Roles (router/repeater/tracker/etc.)** | Defaults, rebroadcast behavior, which packets node originates. | **NO** | Naviga OOTB v0 is minimal (no mesh roles); would conflict with “no mesh” scope. |
| **Radio: SNR, channel index, via_mqtt** | Signal quality, “heard on which channel”, MQTT vs LoRa. | **MAYBE** | Naviga “lastSeenRadioProfile” is placeholder; SNR/channel could feed it later if we expose radio context. |
| **Routing: hops_away, next_hop** | Traceroute, routing UI. | **NO** | No mesh routing in Naviga v0. |
| **Favorite / ignored** | Persist “don’t evict” / “hide from list”. | **MAYBE** | Could map to “pinned” or localAlias-like; not in contract v0. |
| **Security: public_key, is_key_manually_verified** | PKC, “verified” badge. | **NO** | Out of scope for Naviga NodeTable v0. |

---

## 7) Sources (local paths + commits)

All paths relative to repo root; Meshtastic code lives under `Meshtastic/`.

- **Firmware (commit fb757f03):**  
  - `Meshtastic/meshtastic-firmware/src/mesh/NodeDB.h`, `NodeDB.cpp`  
  - `Meshtastic/meshtastic-firmware/src/mesh/generated/meshtastic/deviceonly.pb.h` (NodeInfoLite, UserLite, PositionLite, NodeDatabase)  
  - `Meshtastic/meshtastic-firmware/src/mesh/generated/meshtastic/mesh.pb.h` (NodeInfo, Position, User, etc.)  
  - `Meshtastic/meshtastic-firmware/src/mesh/generated/meshtastic/portnums.pb.h`  
  - `Meshtastic/meshtastic-firmware/src/mesh/generated/meshtastic/telemetry.pb.h` (DeviceMetrics, Telemetry)  
  - `Meshtastic/meshtastic-firmware/src/mesh/MeshService.cpp`, `FloodingRouter.cpp`  
  - `Meshtastic/meshtastic-firmware/src/modules/NodeInfoModule.cpp`, `PositionModule.cpp`  
  - `Meshtastic/meshtastic-firmware/src/modules/Telemetry/DeviceTelemetry.cpp`  
  - `Meshtastic/meshtastic-firmware/src/NodeStatus.h`  
  - `Meshtastic/meshtastic-firmware/src/mesh/generated/meshtastic/config.pb.h` (Role enum)

- **Android (commit 8287203d):**  
  - `Meshtastic/meshtastic-android/core/database/src/main/kotlin/org/meshtastic/core/database/MeshtasticDatabase.kt`  
  - `Meshtastic/meshtastic-android/core/data/src/main/kotlin/org/meshtastic/core/data/repository/NodeRepository.kt`  
  - `Meshtastic/meshtastic-android/core/model/src/main/kotlin/org/meshtastic/core/model/util/DateTimeUtils.kt` (onlineTimeThreshold)  
  - `Meshtastic/meshtastic-android/core/model/src/main/kotlin/org/meshtastic/core/model/NodeInfo.kt`  
  - `Meshtastic/meshtastic-android/feature/node/src/main/kotlin/org/meshtastic/feature/node/component/NodeItem.kt`, `NodeDetailsSection.kt`, `LastHeardInfo.kt`  
  - `Meshtastic/meshtastic-android/feature/map/src/main/kotlin/org/meshtastic/feature/map/BaseMapViewModel.kt`  
  - `Meshtastic/meshtastic-android/core/prefs/src/main/kotlin/org/meshtastic/core/prefs/map/MapPrefs.kt` (lastHeardFilter)

- **iOS (commit 51c8e168):**  
  - `Meshtastic/meshtastic-ios/MeshtasticProtobufs/Sources/meshtastic/mesh.pb.swift` (NodeInfo, lastHeard)  
  - `Meshtastic/meshtastic-ios/Widgets/WidgetsLiveActivity.swift` (NodeInfoView)
