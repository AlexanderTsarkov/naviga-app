# WIP: NodeTable — Define & Research

Product Contract v0 for [Issue #147](https://github.com/AlexanderTsarkov/naviga-app/issues/147). No promotion to canon yet.

---

## 1) Definition

NodeTable is the **single source of truth / knowledge about all nodes (including self)**. Other components emit observations or events; NodeTable stores best-known facts and derived state used for product and UX decisions.

---

## 2) What product needs from NodeTable (the 7 questions)

1. **Identity** — Who is this node? (primary key, display id)
2. **Activity** — Is this node currently active / stale / gone?
3. **Position** — Where is this node (and how good is the fix)?
4. **Capabilities / HW** — What can this node do? What hardware/firmware?
5. **Radio context** — How was it last seen (profile, power, etc.)?
6. **Trust / freshness** — How fresh and reliable is each fact?
7. **Debuggability** — Enough stable identifiers and metadata for logs and support.

---

## 3) Identity (v0)

- **DeviceId** (primary key) = ESP32-S3 MCU ID. In firmware this is the node identity (e.g. `node_id`, uint64).
- **ShortId** = CRC16(DeviceId). Display-only; may collide across nodes.
- **ShortId collision handling:** UI disambiguation via suffixes, e.g. `6DA2:1`, `6DA2:2`. DeviceId remains the source of truth.

---

## 4) Human naming (v0)

- **networkName** — Broadcast by the node; optional; has priority when present.
- **localAlias** — User-defined on the phone for a DeviceId.
- **Effective display name precedence:** `networkName` > `localAlias` > `ShortId` (+ suffix if collision).

---

## 5) Activity (v0)

- **activityState** is **derived**, not stored.
- Four states: **Online** / **Uncertain** / **Stale** / **Archived**.
- Derived from **lastHeard** (RX-based) and policy thresholds.
- NodeTable provides **lastSeenAge** for UI (e.g. “last seen 30s ago”).

---

## 6) Position (v0)

- **2D required;** altitude optional.
- **PositionQuality:** age required; optional satCount, hdop, accuracy, fixType; **source-tagged** (e.g. GNSS vs network).

---

## 7) Capabilities / HW (v0)

- **hwType** (if known) + optional **fwVersion**.
- Capabilities may be derived from hwType/version lookup tables (e.g. “supports X”).

---

## 8) Radio context (v0)

- **lastSeenRadioProfile** includes profile and txPower when available.
- Multiple profile kinds: e.g. UART presets vs full LoRa params.
- Store normalized **RadioProfileId** + **RadioProfileKind**.
- UI shows only valid controls; mapping tables exist (WIP).

---

## 9) Open questions (short list)

- networkName propagation cadence and payload.
- Exact activity thresholds (Online / Uncertain / Stale / Archived).
- Radio profile mapping ownership: firmware vs mobile vs backend.

---

## Promotion criteria (WIP → canon)

When decisions above are stable and reflected in implementation, promote to `docs/product/areas/nodetable.md`.

---

## Links / References

- Research: [research/](research/)
- Issue: [#147 NodeTable — Define & Research (Product WIP)](https://github.com/AlexanderTsarkov/naviga-app/issues/147)
