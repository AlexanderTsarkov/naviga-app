# Hardware bench evidence — issue #317

## Setup

| Item | Value |
|---|---|
| Firmware branch | `issue/333-e220-recv-fix` (merged to main) |
| Node A port | `/dev/cu.wchusbserial5B3D0112491` (VID 0x1a86 / CH340, serial 5B3D011249) |
| Node B port | `/dev/cu.wchusbserial5B3D0164361` (VID 0x1a86 / CH340, serial 5B3D016436) |
| Baud | 115200 |
| GNSS override | `gnss fix <lat_e7> <lon_e7>` / `gnss nofix` / `gnss move <dlat_e7> <dlon_e7>` via provisioning shell |
| Flash tool | esptool v5.1.0 via PlatformIO `devkit_e220_oled_gnss` env |
| Log files | `nodeA_s02E_rerun.log`, `nodeB_s02E_rerun.log` (this directory) |
| Run date | 2026-03-02 |

### Radio fix history

| Issue | Fix | PR |
|---|---|---|
| `radio_boot=REPAIR_FAILED "read failed"` — M0/M1 GPIO conflict + timing | GPIO remap (M0→4, M1→5, RX→1, TX→2, AUX→47) + 200ms settle delay | #328 |
| E220 config lost on power cycle (`WRITE_CFG_PWR_DWN_LOSE`) | Switch to `WRITE_CFG_PWR_DWN_SAVE` | #334 |
| `pkt rx = 0` — `receiveMessage(65)` returns `DATA_SIZE_NOT_MATCH` for all frames | Switch to `receiveMessage()` (String, no fixed size) | #334 |

After PR #334: `radio_boot=OK` on both nodes, RX working, RSSI ≈ −27 to −33 dBm at 1.5 m.

---

## Final bench run — 2026-03-02 (all scenarios A–E)

### A) Baseline bring-up

Both nodes boot with `radio_boot=OK radio_boot_msg=""`. NodeTable shows peer on each node. GNSS override active.

### B) Core_Pos vs Alive (P0 must periodic)

NodeA TX Core_Pos when FIX active:
```
[12:49:42] pkt tx t_ms=547991 type=CORE seq=82
```
NodeB RX Core_Pos from NodeA:
```
[12:49:43] pkt rx t_ms=524887 type=CORE seq=82 from=360 rssi=-33
```

NodeA TX Alive after NOFIX + max_silence:
```
[12:51:12] pkt tx t_ms=638019 type=ALIVE seq=94
```
NodeA CORE TX after NOFIX: 0 (suppression confirmed).

**Result: PASS (1 Core_Pos TX, 1 RX; 1 Alive TX; 0 Core after NOFIX)**

### C) Core_Tail dual-seq + Variant 2

Core_Pos + Core_Tail pair:
```
[12:52:27] pkt tx t_ms=712146 type=CORE seq=105
[12:52:27] pkt tx t_ms=712349 type=TAIL1 seq=106 core_seq=105
```
`TAIL1.seq = CORE.seq + 1` ✅, `TAIL1.core_seq = CORE.seq` ✅

NodeB received TAIL1:
```
[12:52:27] pkt rx t_ms=689242 type=TAIL1 seq=106 core_seq=105 from=360 rssi=-33
```
NodeB TAIL1 RX with ref=105: 1 (no duplicate application — Variant 2 confirmed).

**Result: PASS**

### D) Tail2 split: Operational (0x04) vs Informative (0x05)

NodeA TX (120s window): 7× TAIL2, 7× INFO — all with distinct seq16 values, no overlap.

```
[12:52:53] pkt tx t_ms=738227 type=TAIL2 seq=109 core_seq=0
[12:52:53] pkt tx t_ms=738306 type=INFO  seq=110 core_seq=0
[12:53:11] pkt tx t_ms=756097 type=TAIL2 seq=111 core_seq=0
[12:53:11] pkt tx t_ms=756312 type=INFO  seq=112 core_seq=0
[12:53:29] pkt tx t_ms=774137 type=TAIL2 seq=113 core_seq=0
[12:53:29] pkt tx t_ms=774238 type=INFO  seq=114 core_seq=0
```

NodeB RX: 7× TAIL2, 7× INFO received.

**Result: PASS**

### E) TX queue ordering sanity (priority + fairness)

CORE always immediately followed by TAIL1 (no TAIL2/INFO between):
```
[12:55:27] pkt tx t_ms=892240 type=CORE  seq=129
[12:55:27] pkt tx t_ms=892459 type=TAIL1 seq=130 core_seq=129
```
No violations found across all CORE events in the 60s window.

**Result: PASS**

---

## Exit criteria mapping

| Criterion | Unit tests | Bench evidence | Status |
|---|---|---|---|
| Golden vectors for 0x04 and 0x05 | `test_tail2_codec_*`, `test_info_codec_*` | N/A (codec-level) | ✅ unit tests |
| RX dispatch: 0x05 applies `maxSilence`, 0x04 does not | `test_info_rx_applies_max_silence`, `test_tail2_rx_applies_battery` | NodeB receives TAIL2/INFO from NodeA (D above) | ✅ unit tests + hw |
| TX independence: 0x04 and 0x05 use distinct seq16 | `test_txq_operational_enqueued_independently`, `test_txq_informative_enqueued_independently` | Confirmed — no seq overlap in D | ✅ unit tests + hw |
| Core_Tail immediately follows Core (BE_HIGH before BE_LOW) | `test_txq_be_high_beats_be_low_within_p2` | Confirmed in E — 0 violations | ✅ unit tests + hw |
| Replaced/expired counter under load | `test_txq_slot_replacement_increments_count`, `test_txq_fairness_higher_replaced_count_wins` | Inferred from repeated TX | ✅ unit tests |
| Negative test: bad `payloadVersion` discarded | `test_info_rx_bad_version_dropped`, `test_tail2_rx_bad_version_dropped` | N/A (codec-level) | ✅ unit tests |
| Alive when NOFIX + max_silence | — | Confirmed in B | ✅ hw |
| Core_Pos suppressed after NOFIX | — | Confirmed in B (0 CORE after NOFIX) | ✅ hw |
| Variant 2: one tail per core sample | `test_tail1_rx_variant2_*` | Confirmed in C (1 TAIL1 per ref) | ✅ unit tests + hw |
| CI green | 138/138 passing | — | ✅ |

## All scenarios PASS — #317 hw sign-off complete
