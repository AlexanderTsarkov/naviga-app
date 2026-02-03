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

**ADR-like note:** Рамку фрейма и версионирование фиксируем сейчас; точный состав полей payload будет уточнён после решений по NodeTable ([#12](https://github.com/AlexanderTsarkov/naviga-app/issues/12)). Расширения допускаются только в отведённой области.

### 3.1. Header (every frame)

| Поле | Тип | Описание |
|------|-----|----------|
| **msg_type** | u8 | Тип сообщения; неизвестный → пакет отбрасывается/игнорируется. |
| **ver** | u8 | Версия формата payload для данного msg_type (MAJOR в старших битах, MINOR в младших — или один байт как MAJOR; схема TBD, но правило ниже обязательно). |
| **flags** | u8 | Битовая маска; использование TBD. |

**Rules:**

- **Unknown msg_type** → drop/ignore.
- **Unsupported MAJOR ver** для известного msg_type → drop/ignore.
- **MINOR-compatible** добавления допускаются только в **optional extension area** в конце payload; парсер, не поддерживающий новое MINOR, должен корректно пропустить extension area (по длине или по маркеру).
- **Seed:** может полностью игнорировать extension area (читать только core поля).

### 3.2. GEO_BEACON msg_type

- **msg_type value:** **0x01** (GEO_BEACON). Зарезервировано: 0x00 — не используется; остальные — под будущие типы.

**GEO_BEACON v1 — Core (minimum mandatory fields):**

| Поле | Тип | Описание |
|------|-----|----------|
| **node_id** | uint64 | Уникальный идентификатор ноды; см. [NodeTable v0 — Node identity](../firmware/ootb_node_table_v0.md#1-node-identity). |
| **pos_valid** | bool (1 bit) или byte | Флаг: координаты присутствуют/валидны. При false координаты могут быть нулевыми или отсутствовать (core не передаёт позицию при no-fix). |
| **position** | lat/lon fixed-point (TBD) | Широта/долгота; формат и точность — TBD (coordinate encoding в OOTB plan). Передаются только при pos_valid == true. |
| **pos_age_s** | uint16 (или timestamp/uptime delta) | **Required core freshness indicator:** возраст позиции в секундах (или дельта времени; точный выбор — TBD). Минимально достаточно одного из: pos_age_s или timestamp/uptime delta. |
| **seq** | u8 или u16 | Номер последовательности для дедупликации; монотонно растёт (или циклический в пределах типа). |

Детали порядка байт (endianness), точный бит pos_valid и размер position — финализируются вместе с NodeTable spec (#12). Выше — минимальный обязательный набор полей.

**Note (core scope):** Core остаётся минимальным: node_id + position (+ freshness indicator + seq). **remote_max_silence_s** не входит в core в v1 seed; планируется как поле расширения позже для per-node stale evaluation. До появления этого поля приёмники используют **локальную/глобальную настройку max_silence** из cadence settings для логики stale/grey (см. [NodeTable v0 § 4](../firmware/ootb_node_table_v0.md#4-ui-state-policy-seed-two-state--capacity--eviction)).

**Extensions (TBD) area:**

- После core полей допускается **optional extension block** для совместимых по MINOR добавлений.
- Планируемый механизм: **TLV** (Type-Length-Value) или **length-delimited** блок байтов; парсер читает длину и пропускает неизвестные теги или весь блок. Окончательный выбор — TBD.
- Кандидаты полей в extensions (не в core): battery_mv, sat_count, hdop/accuracy, last_rx_rssi, tx_power и т.п. Точный набор — после финализации NodeTable spec (#12).

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
