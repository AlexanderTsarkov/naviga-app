# Map SDK and map data strategy (ADR)

**Status:** Accepted  
**Date:** 2026-02  
**Issue:** [#6](https://github.com/AlexanderTsarkov/naviga-app/issues/6)

---

## 1) Context & requirements

- **Offline-first:** The product must work with **zero mobile coverage** (hunting, hiking, wilderness). Maps and navigation cannot depend on live connectivity for core use.
- **No paid SDKs / no API keys:** The final product must not require paid map SDKs or API keys as a dependency. Users should be able to use the app without signing up for third-party map services.

---

## 2) Decision

- **Map SDK:** **flutter_map** for the Flutter mobile app.
- **Tile/data strategy:** Online tiles acceptable for OOTB v0 (dev/demo only); roadmap to offline tile packs (MBTiles / PMTiles) for the final product.

---

## 3) Why flutter_map

- **Flexible tile providers:** Supports arbitrary tile URLs (OSM, self-hosted, or later offline sources).
- **Offline options later:** Can integrate MBTiles/PMTiles and local tile storage without changing the core SDK.
- **No vendor lock-in:** Open source; no mandatory API keys or commercial terms for basic usage.
- **Flutter ecosystem fit:** Widely used, maintained, and compatible with our stack.

---

## 4) OOTB v0 approach

- **Online tiles** are acceptable for early demo and development.
- **Initial dev provider:** OSM Standard (e.g. OpenStreetMap tiles) as the default tile source for quick iteration.
- **OSM tile usage policy:** OSM’s default tile servers are **not** for heavy production use. For production we will either run our own tile server or use a proper tile provider; OOTB v0 is explicitly for dev/POC only.

---

## 5) Offline roadmap

- **Plan:** Support **offline tile packs** (MBTiles or PMTiles) so users can pre-download regions and use the map without connectivity.
- **Where packs will live:** Stored locally on device (app storage or user-selected directory); format TBD (MBTiles/PMTiles).
- **User flow (later):** User downloads or selects a region; app uses the local pack when offline. **Not implemented in OOTB v0** — this document outlines intent only.

---

## 6) Alternatives considered

| Option | Outcome |
|--------|--------|
| **MapLibre GL** | Considered; flutter_map chosen for simpler tile-based workflow and offline path without GL-specific lock-in. |
| **Mapbox** | Rejected: API keys, pricing, and vendor lock-in are not acceptable for an offline-first, no-key product. |

---

## 7) Consequences

- **Later tickets** that implement the map screen (OOTB v0 and beyond) will use **flutter_map** and the tile provider strategy defined here.
- **OOTB v0:** Implement map with online OSM tiles; document in code/config that this is dev-only.
- **Future work:** Offline tile pack support (MBTiles/PMTiles), region selection, and storage will depend on this decision and be tracked in separate issues.
