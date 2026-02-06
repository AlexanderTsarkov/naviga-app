# Issue #76 Radio Channel Access (Bench Plan)

**Issue:** #76 — [OOTB v0] Radio: channel access (LBT/CAD) + collision mitigation  
**Date:** TBD  
**Tester:** TBD  
**Firmware build:** TBD (commit / branch / binary name)  
**Hardware env:** esp32dev / devkit_e220_oled

## 1) Goal
Compare **jitter-only baseline** vs **jitter + channel sense** (if supported) on real modules.

## 2) Defaults and how to enable
- **Default behavior:** channel sense **OFF** (jitter-only).
- **Enable channel sense:** TBD (future flag/compile-time option).
- **Current hardware notes:**
  - **E220 UART:** channel sense may be **UNSUPPORTED** (no reliable LBT/CAD in current driver).
  - **Future SPI radios (e.g. LLCC68):** expected to support CAD.

## 3) Bench setup
- 2 devices (then 3–5 devices), same band/channel
- Antennas attached, stable power/USB
- Optional BLE client to observe NodeTable pages

## 4) What to observe
- **UART logs (logging v0):**
  - RADIO_TX_OK / RADIO_TX_ERR
  - RADIO_RX_OK / RADIO_RX_ERR
  - DECODE_OK / DECODE_ERR
  - NODETABLE_UPDATE
- **Radio behavior:**
  - beacon update latency (time between updates)
  - missed beacons or storm symptoms (repeated TX with little RX)
- **BLE (optional):**
  - NodeTable pages reflect peers over time

## 5) Test procedure
1. Flash **jitter-only baseline** build to all devices.
2. Run for 5–10 minutes; capture UART logs.
3. Repeat with **jitter + channel sense** enabled (if supported on hardware).
4. Compare: missed beacons, average update latency, collision symptoms.

## 6) Results (fill after run)
- **Baseline (jitter-only):** TBD
- **Jitter + sense:** TBD
- **Notes / anomalies:** TBD
