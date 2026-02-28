# OOTB Radio v0 — спецификация

**Purpose:** Формат и правила радиоканала OOTB v0 (GEO_BEACON, дефолты, пресеты). Issue [#14](https://github.com/AlexanderTsarkov/naviga-app/issues/14).

**Related:** [Radio profile presets v0](radio_profile_presets_v0.md), [ADR radio band strategy](../adr/radio_band_strategy_v0.md).

---

## 1. Scope (кратко)

- Один тип пакета — **GEO_BEACON** (формат полей, версия, порядок байт — отдельные разделы этого документа или TBD).
- Дефолты параметров (интервал beacon, max_silence, и т.д.) — в этом документе или по ссылкам.
- **Radio defaults & presets** — ниже; детали пресетов и маппинг на железо — в [radio_profile_presets_v0.md](radio_profile_presets_v0.md).

---

## 2. Radio defaults & presets

- **Пользователь управляет:** **Channel + Power** (и при необходимости **SpeedPreset** позже). Прямые параметры LoRa (частота в Hz, SF, BW, CR) в UI/протоколе не экспонируются — только абстракции (channel_id, power_level_id, speed_preset_id). См. [radio_profile_presets_v0.md](radio_profile_presets_v0.md).
- **Default power:** **module_min** — минимальная мощность, поддерживаемая модулем (не максимальная). См. [ADR radio band strategy](../adr/radio_band_strategy_v0.md).
- **Seed note:** в seed допустимо фиксировать **channel_id=1** для скорости разработки; поле **channel_id** в контракте и структурах обязательно присутствует, чтобы в prod поддерживать смену канала через приложение.

Конкретные значения OOTB public profile (profile_id, band_id, channel_id, speed_preset_id, power_level_id) и маппинг на UART/SPI — в [radio_profile_presets_v0.md](radio_profile_presets_v0.md).

---

## 3. Radio frame format (v0)

**Decision:** [#304](https://github.com/AlexanderTsarkov/naviga-app/issues/304) — 2-byte frame header (7+3+6 bit layout) precedes every payload. Supersedes the earlier 3-byte header sketch (`msg_type | ver | flags`) which was never implemented.

> **Historical note:** An earlier draft of this section described a 3-byte header (`msg_type | ver | flags`). That draft was never implemented on-air. The canonical header is the 2-byte layout defined below. The `ver` and `flags` fields from the draft are retired; forward-compat is handled via the `reserved` bits and `payloadVersion` inside the payload.

### 3.0. Packet anatomy & invariants

Every on-air frame follows this structure:

```
[2-byte header] [Common(always)] [Useful payload]
```

**Common(always)** — present in every `Node_*` packet, at the start of every payload:

| Offset | Field | Bytes | Notes |
|--------|-------|-------|-------|
| 0 | `payloadVersion` | 1 | Payload format version; `0x00` = v0. Unknown → discard. |
| 1–6 | `nodeId48` | 6 | NodeID48 (6-byte LE MAC48); sender identity. |
| 7–8 | `seq16` | 2 | Global per-node sequence counter (uint16 LE); see below. |

**`seq16` invariants (normative):**
- `seq16` is **one global per-node counter** shared across ALL `Node_*` packet types. Every packet a node transmits — regardless of `msg_type` — increments the same counter.
- **Dedupe key:** `(nodeId48, seq16)`. Receivers MUST deduplicate on this pair only. Dedupe MUST NOT inspect payload type or content.
- `Node_OOTB_Core_Tail` (`0x03`) carries **two** seq fields: its own `seq16` in Common (unique packet id) **and** `ref_core_seq16` in its Useful payload (references the `seq16` of the `Node_OOTB_Core_Pos` packet it supplements). These are distinct fields; `seq16 ≠ ref_core_seq16`.

**Minimum payload size:** 9 bytes (Common only). On-air minimum: 11 bytes (2-byte header + 9-byte payload).

### 3.0a. Canonical packet naming

Canonical alias format: **`<Originator>_<Scope>_<Intent>`**

| `msg_type` | Canonical alias | Legacy wire enum | Notes |
|---|---|---|---|
| `0x01` | `Node_OOTB_Core_Pos` | `BEACON_CORE` | Position-bearing; 15 B payload |
| `0x02` | `Node_OOTB_I_Am_Alive` | `BEACON_ALIVE` | Alive-bearing, non-position; 9–10 B payload |
| `0x03` | `Node_OOTB_Core_Tail` | `BEACON_TAIL1` | Core-attached extension; 11 B min payload |
| `0x04` | `Node_OOTB_Operational` | `BEACON_TAIL2` | Operational slow state; 11 B min payload |
| `0x05` | `Node_OOTB_Informative` | `BEACON_INFO` | Informative/static config; 11 B min payload |

**Naming rules:**
- Canon docs and policy MUST use the `<Originator>_<Scope>_<Intent>` alias as the primary name.
- Legacy `BEACON_*` wire enum names may appear parenthetically for firmware/wire traceability.
- Do not invent new `BEACON_*` names beyond those in this registry.
- Originator: `Node` | `Mesh`. Scope: `OOTB` | `Session` | `User` (+ `Diag`/`Debug` optional). Intent: action or meaning word.

### 3.1. Header (every frame)

Every on-air frame = **[header: 2 bytes][payload: 0–63 bytes]**.

#### Bit layout (16-bit LE word)

```
Bits [15:9]  msg_type    — 7 bits  (0x00–0x7F; 128 packet families)
Bits [8:6]   reserved    — 3 bits  (MUST be 0b000 on send; receiver MUST ignore)
Bits [5:0]   payload_len — 6 bits  (0–63; count of payload bytes that follow)
```

The 16-bit word is little-endian: `H = wire_byte_0 | (wire_byte_1 << 8)`.

#### Wire packing (encode)

```
wire byte 0 = ((reserved & 0x3) << 6) | payload_len   // H & 0xFF  (low byte)
wire byte 1 = (msg_type << 1) | (reserved >> 2)        // (H >> 8) & 0xFF  (high byte)
```

#### Decode formulas

```
H           = wire_byte_0 | (wire_byte_1 << 8)
msg_type    = (H >> 9) & 0x7F
reserved    = (H >> 6) & 0x07
payload_len = H & 0x3F
```

#### Golden example

`msg_type = 0x01` (BEACON_CORE), `reserved = 0`, `payload_len = 15`:

```
H = (0x01 << 9) | 15 = 0x020F
wire byte 0 = 0x020F & 0xFF       = 0x0F
wire byte 1 = (0x020F >> 8) & 0xFF = 0x02
Wire: [0x0F, 0x02]
```

#### Validity rules

| Condition | Action |
|-----------|--------|
| `msg_type == 0x00` | Drop packet |
| `msg_type` not in registry | Drop packet |
| `reserved != 0b000` | Accept (ignore reserved bits; forward-compat) |
| Actual received payload bytes ≠ `payload_len` | Drop packet |
| `payloadVersion` unknown for known `msg_type` | Discard payload; do not update NodeTable |
| Payload shorter than minimum for that `msg_type` | Drop packet |

### 3.2. msg_type registry (v0)

| `msg_type` | Canonical alias | Legacy wire enum | Payload contract | Notes |
|---|---|---|---|---|
| `0x00` | (reserved) | — | — | MUST NOT be used; drop on receive |
| `0x01` | `Node_OOTB_Core_Pos` | `BEACON_CORE` | [beacon_payload_encoding_v0 §4.1](../product/areas/nodetable/contract/beacon_payload_encoding_v0.md) | Position-bearing; 15 B payload |
| `0x02` | `Node_OOTB_I_Am_Alive` | `BEACON_ALIVE` | [alive_packet_encoding_v0 §3](../product/areas/nodetable/contract/alive_packet_encoding_v0.md) | Alive-bearing, non-position; 9–10 B payload |
| `0x03` | `Node_OOTB_Core_Tail` | `BEACON_TAIL1` | [tail1_packet_encoding_v0 §3](../product/areas/nodetable/contract/tail1_packet_encoding_v0.md) | Core-attached extension; 11 B min payload |
| `0x04` | `Node_OOTB_Operational` | `BEACON_TAIL2` | [tail2_packet_encoding_v0 §3](../product/areas/nodetable/contract/tail2_packet_encoding_v0.md) | Operational slow state (dynamic runtime); 11 B min payload |
| `0x05` | `Node_OOTB_Informative` | `BEACON_INFO` | [info_packet_encoding_v0 §3](../product/areas/nodetable/contract/info_packet_encoding_v0.md) | Informative/static config; 11 B min payload |
| `0x06`–`0x7F` | (reserved) | — | — | Reserved for future types; drop on receive |

### 3.3. Layer separation

`msg_type` (in the 2-byte frame header) and `payloadVersion` (first byte of every payload) are **independent versioning axes**:

- **`msg_type`** identifies the packet family. Dispatch happens at the framing layer before any payload parsing.
- **`payloadVersion`** (uint8, first byte of payload) determines the entire payload layout for that `msg_type`. The payload byte offsets defined in the canon contracts are **not affected** by the frame header.

Unknown `msg_type` → drop the entire frame. Unknown `payloadVersion` for a known `msg_type` → discard payload (do not update NodeTable state from that packet).

### 3.4. On-air size summary

| Canonical alias | Legacy enum | Header (B) | Payload min (B) | Total min on-air (B) | LongDist budget (B) |
|---|---|---|---|---|---|
| `Node_OOTB_Core_Pos` | `BEACON_CORE` | 2 | 15 | **17** | 24 ✓ |
| `Node_OOTB_I_Am_Alive` | `BEACON_ALIVE` | 2 | 9 | **11** | 24 ✓ |
| `Node_OOTB_Core_Tail` | `BEACON_TAIL1` | 2 | 11 | **13** | 24 ✓ |
| `Node_OOTB_Operational` | `BEACON_TAIL2` | 2 | 11 | **13** | 24 ✓ |
| `Node_OOTB_Informative` | `BEACON_INFO` | 2 | 11 | **13** | 24 ✓ |

All packets fit within the LongDist budget (24 B) at minimum size. Minimum payload is 9 B (Common only: payloadVersion + nodeId48 + seq16); minimum on-air is 11 B.

---

## 4. Beacon cadence (seed)

### 4.1. Parameters

| Параметр | Default | Suggested | Описание |
|----------|---------|-----------|----------|
| **min_update_interval_s** | TBD | 5 s | Минимальный интервал между отправками beacon при достаточном движении. |
| **min_movement_m** | TBD | 50 m | Минимальное смещение позиции (м) для отправки beacon по движению. |
| **max_silence_multiplier** | TBD | 6 | Множитель: **max_silence = min_update_interval_s × max_silence_multiplier**; при превышении — keep-alive beacon. |
| **jitter_pct** | TBD | ±20% | Разброс времени следующей отправки для снижения коллизий. |

Все дефолты — TBD; suggested values приведены для seed/тестов.

### 4.2. Seed algorithm (псевдоописание)

- **GNSS updates** могут приходить чаще, чем cadence TX; beacon отправляется по правилам ниже, а не на каждое обновление GNSS.
- **На каждом тике:** если `time_since_last_tx >= min_update_interval_s` и `movement >= min_movement_m` → **TX beacon**.
- Если **movement < min_movement_m:** обычный TX по движению не делаем; но если `time_since_last_tx >= max_silence` → **TX keep-alive beacon** (чтобы нода не считалась пропавшей).
- **Jitter:** к моменту следующей запланированной отправки применяется jitter (jitter_pct), чтобы уменьшить одновременные передачи у нескольких нод.

Полная модель адаптивной перегрузки/backoff (airtime utilization ladder и т.п.) описана в Mesh concept doc (`docs/product/`, концептуальная спецификация протокола v1.4); seed реализует только базовые параметры без обратной связи по загрузке эфира. **Future:** авто-адаптация (airtime utilization) — вне scope seed.

### 4.3. Airtime constraint

- **GEO_BEACON airtime target ≤ 300 ms** (TBD: проверить в тестах). Ограничение на длительность передачи одного beacon в эфире.

### 4.4. TX queue priority classes (normative)

The firmware TX queue assigns each pending packet type a **priority class**. At most one frame is sent per tick; the slot with the highest priority is selected first. Within the same priority class, the slot with the **highest `replaced_count`** is selected (most-starved-first fairness); ties are broken by **oldest `created_at_ms`** (FIFO).

| Priority | Level name | Packet types | Rationale |
|----------|------------|--------------|-----------|
| **P0** | `P0_HIGH` | `Node_OOTB_Core_Pos` (0x01), `Node_OOTB_I_Am_Alive` (0x02) | Must send; drives liveness and position. Starvation of P0 is not permitted. |
| **P1** | `P1_MID` | `Node_OOTB_Core_Tail` (0x03) | Best-effort, time-bound to the Core sample it qualifies. Sent before P2 to maximise Core_Tail usefulness. |
| **P2** | `P2_LOW` | `Node_OOTB_Operational` (0x04), `Node_OOTB_Informative` (0x05) | Best-effort slow state. Both are equal priority; fairness via `replaced_count` prevents starvation of either. |
| **P3** | `P3_OPPORTUNISTIC` | *(reserved; no current packet type)* | Reserved for future opportunistic or diagnostic packet types (e.g. `Node_OOTB_Diag`). Sent only when no P0–P2 slots are pending. |

**Slot model invariants:**
- One slot per packet type (5 slots total for v0).
- Replacement (re-enqueue before send) increments `replaced_count` and **preserves** `created_at_ms` (fairness: age from first enqueue).
- `Core_Tail` slot is always replaced together with `Core_Pos` when a new Core is formed before the old one is sent.
- Global `seq16` is assigned at **formation time** (enqueue), not at dequeue time. Every enqueued packet consumes the next counter value.

**Relation to field_cadence_v0 tiers:** The priority classes above map to the delivery tiers in [field_cadence_v0.md](../product/areas/nodetable/policy/field_cadence_v0.md) as follows: Tier A → P0 (Core/Alive), Tier B → P1 (Core_Tail), Tier C → P2 (Operational/Informative). The degrade-under-load order in field_cadence_v0 §4 (keep Tier A, reduce Tier B, drop Tier C first) is enforced by this priority ordering.

---

## 5. Contention handling (seed)

**Решения для снижения коллизий на канале (OOTB Public, channel 1 и при необходимости private).**

### 5.1. LBT (Listen Before Talk)

- **По умолчанию:** в OOTB Public profile (channel 1 «public») **LBT включён** — перед TX выполняется проверка занятости канала (sense before send).
- **Примечание по железу:** серия **E220** поддерживает LBT / определение активности канала перед передачей; см. пользовательскую документацию модуля (E220 user manual — разделы по LBT / channel busy detection) и [poc_e220_evidence.md § CCA/LBT](../firmware/poc_e220_evidence.md).

### 5.2. Software-level randomization

- **Jitter на запланированный beacon TX:** к моменту отправки применяется случайное смещение, например **0..500 ms** или **±10–20%** от интервала (согласовать с [§ 4.1 jitter_pct](#41-parameters)); цель — разнести во времени одновременные передачи разных нод.
- **При занятом канале / неудачной отправке:**
  - Применить **небольшой случайный backoff** и **повторить попытку** ограниченное число раз: для beacon **максимум 1–2 retry**.
  - Если после retries канал по-прежнему занят — **пропустить этот цикл beacon** (не спамить повторными попытками); следующий beacon по обычному cadence.

### 5.3. Future (not seed)

- **Slotting / TDMA** — зарезервировано для последующей mesh-оптимизации; в seed не входит.

---

## 6. Network/Session identity (seed)

- **Seed rule:** Network ID == **channel_id**. Один канал — одна «сеть» в смысле видимости пакетов.
- **Reserved channels:**
  - **public_channel_id = 1** — OOTB Public network.
  - **private channels = 2..N** (N TBD по модулю/конфигу).
- **Network modes:**
  - **Public mode:** channel_id зафиксирован на 1; пользователь не может выбрать канал в этом режиме.
  - **Private mode:** пользователь выбирает channel_id в диапазоне 2..N.
- **Seed security:** Private — «без защиты» (как открытое радио). Любое устройство на том же канале участвует в обмене; шифрование/аутентификация — вне scope seed.

См. [radio_profile_presets_v0.md](radio_profile_presets_v0.md) (OOTB public profile: channel_id=1).

---

## 7. Reset

- **«Back to Public»** — сброс **только радио** на OOTB public profile (channel=1 + дефолтные пресеты). Не меняет NodeID и пользовательскую идентичность. См. [radio_profile_presets_v0.md § 2 OOTB public profile](radio_profile_presets_v0.md#2-ootb-public-profile).
- **«Factory reset»** — полный сброс (точный объём — TBD позже); отдельно от «Back to Public».
