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

## SF/BW/CR inference

### 2.4 kbps — step pattern analysis

Step size ~20.3 ms, tier width ~4–5 bytes.

At 2.4 kbps effective throughput, one symbol carries:
- If SF=11, BW=125 kHz: symbol duration = 2^11 / 125000 = 16.384 ms — close but not matching
- If SF=11, BW=250 kHz: symbol duration = 8.192 ms — 2–3 symbols per tier
- **Most likely:** SF=11, BW=125 kHz, CR=4/5 — symbol time 16.4 ms, with preamble overhead explaining the ~20 ms step

The ~20.3 ms step is consistent with **1 LoRa symbol at SF=11, BW=125 kHz** (16.384 ms) plus coding overhead.

### 4.8 kbps — linear pattern

Linear 1.04 ms/byte is characteristic of **FSK modulation** or LoRa at very low SF (SF=5 or SF=6) where symbol time is short enough to appear linear at byte granularity.

At 4.8 kbps: 1 byte = 8 bits, 8/4800 = 1.67 ms theoretical. Measured 1.04 ms/byte suggests effective throughput ~7.7 kbps on air (accounting for E220 framing overhead).

**Likely:** FSK mode at 4.8 kbps air rate (EBYTE uses FSK for higher speed presets on E220-400T30D).

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
