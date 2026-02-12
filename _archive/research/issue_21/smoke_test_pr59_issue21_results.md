# Hardware smoke test results — PR #59 / Issue #21

**Date:** 2025-02-04  
**Env:** `devkit_e220_oled`  
**Boards:** Two ESP32-S3 (CH340 serial), same firmware flashed on both.

---

## 1) Ports

| Role | Port | Serial (USB) |
|------|------|----------------|
| **PORT_A** | `/dev/cu.wchusbserial5B3D0112491` | 5B3D011249 |
| **PORT_B** | `/dev/cu.wchusbserial5B3D0164361` | 5B3D016436 |

---

## 2) Flashing

- **PORT_A:** `pio run -e devkit_e220_oled -t upload --upload-port /dev/cu.wchusbserial5B3D0112491` → **SUCCESS**
- **PORT_B:** `pio run -e devkit_e220_oled -t upload --upload-port /dev/cu.wchusbserial5B3D0164361` → **SUCCESS**

---

## 3) Serial monitoring

- `pio device monitor` was not used (requires a real TTY; failed with `termios.error: (19, 'Operation not supported by device')` in this environment).
- Serial was captured with Python + pyserial. Boot logs were captured after a DTR reset (DTR low → high) to get the first ~40 lines including role.

---

## 4) Validation

### Roles (from serial boot logs)

- **Board on PORT_A** (GPIO8 strapped to GND): **INIT (PING)** ✓  
- **Board on PORT_B** (no strap): **RESP (PONG)** ✓  

### Key serial lines — PORT_A (INIT / PING)

```
ESP-ROM:esp32s3-20210327
...
=== Naviga OOTB skeleton ===
fw: ootb-21-radio-smoke
hw_profile: devkit_e220_oled
role: INIT (PING)
=== Node identity ===
full_id_u64: 0x00003CDC756F23BC
full_id_mac: 3CDC756F23BC
short_id: EA3E
BLE: started (Naviga service)
GNSS: FIX acquired
SELF_POS: updated reason=FIRST_FIX ...
tick: 1038
...
```

### Key serial lines — PORT_B (RESP / PONG)

```
ESP-ROM:esp32s3-20210327
...
=== Naviga OOTB skeleton ===
fw: ootb-21-radio-smoke
hw_profile: devkit_e220_oled
role: RESP (PONG)
=== Node identity ===
full_id_u64: 0x00009C139EABBAA0
full_id_mac: 9C139EABBAA0
short_id: 4C5B
BLE: started (Naviga service)
GNSS: FIX acquired
SELF_POS: updated reason=FIRST_FIX ...
tick: 1015
...
```

### OLED and TX/RX/SEQ

- The firmware does **not** log individual PING/PONG or TX/RX events to Serial; only the OLED shows **ROLE**, **radio** (OK/ERR), **tx**, **rx**, **seq**.
- **You need to read the OLED on both boards after ~10 s** to confirm:
  - **Board A (INIT):** ROLE = `INIT (PING)`, RAD = OK, **tx** and **rx** and **seq** advancing.
  - **Board B (RESP):** ROLE = `RESP (PONG)`, RAD = OK, **tx** and **rx** and **seq** advancing.

If TX/RX/SEQ advance on both within ~10 s, PING/PONG over the E220 link is confirmed.

---

## 5) Summary

| Check | Result |
|-------|--------|
| PORT_A / PORT_B identified | ✓ |
| Same firmware on both | ✓ |
| PORT_A role INIT (PING) in serial | ✓ |
| PORT_B role RESP (PONG) in serial | ✓ |
| Serial evidence of firmware + identity | ✓ |
| OLED ROLE / RAD / TX / RX / SEQ | **Please confirm on hardware** |

**Conclusion:** Serial logs confirm correct roles (INIT on strapped board, RESP on unstrapped board) and same OOTB smoke firmware on both. Final confirmation of PING/PONG is by observing TX/RX/SEQ (and RAD state) on the OLED after ~10 s.

---

## If it had failed

Planned follow-up:

1. **Capture first 40 serial lines** from each board after boot (as done above).
2. **Report OLED:** ROLE, RAD state, TX, RX, SEQ for each board.
3. **Next two debugging steps:**  
   - Check E220 wiring (TX/RX/AUX/M0/M1, 3.3 V, GND) and that both modules are on the same channel/sub-band.  
   - Verify mode pins (M0/M1) for transparent mode and power supply stability under TX.
