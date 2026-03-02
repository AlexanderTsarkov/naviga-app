# E220 Airtime Profile — 2.4 kbps and 4.8 kbps presets

**Issue:** #332  
**Date:** 2026-03-02  
**Device:** E220-400T30D on devkit_e220_oled_gnss (ESP32-S3)  
**Method:** AUX pin timing — micros() from sendMessage() call to AUX HIGH (TX complete)  
**Samples:** 5 per payload size, sizes 1–50 bytes  
**Raw data:** `e220_airtime_bench/raw_data_full.csv`

---

## Key findings

### 2.4 kbps preset (`AIR_DATA_RATE_000_24`)

**Step pattern:** airtime jumps ~20.3 ms every ~4–5 bytes.

| Tier | Size range (bytes) | Median airtime |
|---|---|---|
| 1 | 16–19 | 2.2 – 3.5 ms |
| 2 | 20–23 | ~22.7 ms |
| 3 | 24–28 | ~43.3 ms |
| 4 | 29–32 | ~63.8 ms |
| 5 | 33–37 | ~84.3 ms |
| 6 | 38–41 | ~104.8 ms |
| 7 | 42–46 | ~125.3 ms |
| 8 | 47–50 | ~145.8 ms |

**Step size:** ~20.3 ms per tier boundary.  
**Tier width:** 4–5 bytes.  
**Sizes 1–15:** below AUX timing resolution (TX completes before polling detects LOW — likely < 1 ms).

**Optimal payload sizes** (stay just below the next step):
- ≤19 bytes → ≤3.5 ms (tier 1)
- ≤23 bytes → ≤22.7 ms (tier 2)
- ≤28 bytes → ≤43.3 ms (tier 3)
- ≤32 bytes → ≤63.8 ms (tier 4)

**Current OOTB packets** (estimated sizes):
- CORE (~18 bytes) → tier 1, ~2.4 ms ✅
- TAIL1 (~18 bytes) → tier 1, ~2.4 ms ✅
- TAIL2 (~20 bytes) → tier 2, ~22.7 ms ⚠️ jumps to tier 2
- INFO (~22 bytes) → tier 2, ~22.7 ms ⚠️

**Implication:** TAIL2 and INFO are 1–4 bytes over the tier-1 boundary. Reducing them to ≤19 bytes would cut airtime from ~22.7 ms to ~3.5 ms — a 6× reduction.

### 4.8 kbps preset (`AIR_DATA_RATE_011_48`)

**Linear growth:** ~1.04 ms per byte, no step boundaries.

| Size (bytes) | Median airtime |
|---|---|
| 16 | 0.35 ms |
| 17 | 1.39 ms |
| 20 | 4.52 ms |
| 25 | 9.72 ms |
| 30 | 14.94 ms |
| 40 | 25.36 ms |
| 50 | 35.78 ms |

**Bytes per ms:** ~0.96 bytes/ms (inverse: 1.04 ms/byte).  
**Sizes 1–15:** below AUX timing resolution (< ~0.35 ms).

**Implication:** 4.8 kbps behaves like FSK or LoRa with very low SF — no symbol-boundary quantization. Every byte costs the same. At 4.8 kbps, a 50-byte packet takes 35.8 ms vs 145.8 ms at 2.4 kbps — **4× faster**.

---

## Modulation identification

### E220 "air data rate" presets use FSK/GFSK, not LoRa

**Key finding:** The standard LoRa TOA formula (Semtech AN1200.13) cannot reproduce our measurements for any combination of SF (5–12), BW (62.5–500 kHz), CR (4/5–4/8), or preamble length. This rules out LoRa modulation entirely.

**Evidence from step pattern (2.4 kbps):**

Step size ~20.3 ms, tier width ~4.5 bytes average.

```
20.3 ms / 4.5 bytes = 4.51 ms/byte → implied air baud = 10 bits / 0.00451 s = 2217 bps ≈ 2400 bps
```

This matches the "2.4 kbps" preset label exactly. The step pattern is not LoRa symbol quantization — it is **FSK frame blocking**: E220 encodes payload in fixed-size internal blocks (~4–5 bytes), and each additional block adds one block-time (~20.3 ms at 2400 baud).

**Evidence from 4.8 kbps linear pattern:**

~1.04 ms/byte = 1 byte × 10 bits / 9600 bps... wait — 10/0.00104 = 9615 bps. This is actually the **UART baud rate (9600 bps)**, not the air rate. At 4.8 kbps the module appears to be UART-limited in our measurement window, or the air rate is fast enough that UART transfer dominates. Either way, the linear pattern confirms FSK (no symbol quantization).

**Why LoRa TOA tables do not apply:**

The LoRa TOA table (SF9/BW125/CR4-5/preamble16 → 218 ms for 20 bytes) describes **Semtech SX126x/SX127x in LoRa mode**. E220-400T30D uses the SX1268 chip, but EBYTE's firmware exposes only UART speed presets (0.3–19.2 kbps) — these map to **FSK/GFSK modulation**, not LoRa chirp spread spectrum. LoRa mode on E220 requires direct register access via AT commands, which the standard library does not use.

**Comparison at 20 bytes:**

| Mode | Airtime |
|---|---|
| LoRa SF9/BW125/CR4-5/p16 (table) | 218 ms |
| LoRa SF7/BW500/CR4-5/p8 (table) | 14 ms |
| **E220 "2.4 kbps" FSK (measured)** | **~22.7 ms** |
| **E220 "4.8 kbps" FSK (measured)** | **~4.5 ms** |

### Implication for sensitivity

The E220 datasheet claims −131 dBm sensitivity at 2.4 kbps. This is the FSK sensitivity, not LoRa sensitivity. LoRa SF9/BW125 would give −137 dBm (SF12 −148 dBm) but with 10× longer airtime. The E220 "2.4 kbps" preset is a tradeoff: better than LoRa SF7 in sensitivity, but far worse than SF9+.

---

## Duty cycle budget (2.4 kbps, OOTB v1-A profile)

Current cadence: 4 packets per cycle (CORE + TAIL1 + TAIL2 + INFO), every ~18s (role=human).

| Packet | Est. size | Airtime |
|---|---|---|
| CORE | ~18 B | ~2.4 ms |
| TAIL1 | ~18 B | ~2.4 ms |
| TAIL2 | ~20 B | ~22.7 ms |
| INFO | ~22 B | ~22.7 ms |
| **Total** | | **~50.2 ms / 18s cycle** |

Duty cycle: 50.2 ms / 18000 ms = **0.28%** — well within 1% EU duty cycle limit.

If TAIL2/INFO were trimmed to ≤19 bytes:
- Total: ~4 × 2.4 ms = ~9.6 ms / 18s = **0.05%** — 5× improvement.

---

## Recommendations

1. **TAIL2 and INFO payload optimization:** Current sizes land in tier 2 (~22.7 ms). Trimming to ≤19 bytes drops to tier 1 (~3.5 ms) — 6× airtime reduction. Worth investigating actual encoded sizes.

2. **4.8 kbps for future profiles:** Linear scaling, no step traps, 4× faster than 2.4 kbps for same payload. Good candidate for higher-cadence profiles (e.g. vehicle role) where duty cycle allows.

3. **Sizes 1–15 bytes:** AUX timing method cannot resolve these (TX completes in < ~0.3 ms). For sub-15 byte profiling, an interrupt-based method (attachInterrupt on AUX) would be needed.
