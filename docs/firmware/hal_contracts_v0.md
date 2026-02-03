# HAL contracts v0 (OOTB)

**Purpose:** Интерфейсы железа для firmware OOTB v0. Domain-слой не зависит от конкретного железа — только от этих интерфейсов. Реализации: реальные драйверы или моки (для тестов/ПК/без железа). См. [OOTB plan](../product/OOTB_v0_analysis_and_plan.md) (раздел 3.5 — полный объём архитектуры).

**Принцип:** Domain не знает про NMEA, UART, serial, конкретные чипы — только про IRadio, IBleTransport, IGnss, ILog.

---

## 1. IRadio

- **Ответственность:** отправка/приём пакетов по радио (GEO_BEACON и т.п.); конфиг (частота, мощность, при необходимости CCA/LBT); снапшот статуса и capability info для BLE telemetry (см. § 6).
- **Методы (концептуально):** init, **send(payload) → RadioTxResult**, receive / poll или callback, setPower, setFrequency (или общий setConfig); **getRadioStatusSnapshot()** (tx_count, rx_count, last_rx_rssi; см. § 1.1 счётчики); **getRadioCapabilityInfo()** (band_id, channel_min/max, power_min/max, module_model). Детали — в [ootb_radio_v0.md](../protocols/ootb_radio_v0.md) и [poc_e220_evidence.md](poc_e220_evidence.md).
- **Реализации:** реальный драйвер (например E220-400T30D), mock для тестов.

### 1.1. Radio TX API result normalization (Issue #13)

**Цель:** единый контракт результата TX и счётчиков для диагностики/логирования; вышележащие слои (BeaconTxTask) ведут себя одинаково независимо от отличий UART-библиотек и модулей.

**Результат отправки (enum RadioTxResult):**

| Значение | Описание |
|----------|----------|
| **OK** | Пакет успешно передан в эфир. |
| **BUSY_OR_CCA_FAIL** | Канал занят или CCA/LBT не прошёл; TX не выполнен. |
| **FAIL** | Другая ошибка (таймаут, сбой модуля, неверный ответ и т.п.). |

Метод **send(payload)** возвращает **RadioTxResult**. Реализация HAL обязана **отображать** специфичные коды/состояния конкретной библиотеки или модуля (E220, другой UART/SPI) в эти три исхода, чтобы domain не зависел от деталей железа.

**Счётчики для диагностики и логирования (экспонируются через getRadioStatusSnapshot() или отдельный getRadioTxDiagnostics()):**

| Счётчик | Тип | Описание |
|---------|-----|----------|
| **tx_attempts** | uint32 | Всего попыток отправки (каждая вызов send считается одной попыткой). |
| **tx_ok** | uint32 | Успешные отправки (результат OK). |
| **tx_busy** | uint32 | Попытки, завершившиеся BUSY_OR_CCA_FAIL. |
| **tx_fail** | uint32 | Попытки, завершившиеся FAIL. |
| **tx_retries_used** | uint32 | Число повторных попыток (retry) после busy/fail в рамках одной «логической» beacon-отправки (для согласования с ограниченным retry в [Radio v0 § 5.2](../protocols/ootb_radio_v0.md#52-software-level-randomization)). |

**Примечание:** Различия UART-библиотек и модулей (разные коды ошибок, наличие/отсутствие CCA) **должны маппиться** в RadioTxResult и эти счётчики, чтобы поведение BeaconTxTask (backoff, ограниченный retry, skip цикла) было единообразным.

---

## 2. IBleTransport

- **Ответственность:** BLE-транспорт: подключение, отправка/получение данных (снапшот NodeTable, события, настройки). Контракт формата — [ootb_ble_v0.md](../protocols/ootb_ble_v0.md).
- **Методы (концептуально):** init, startAdvertising / startScan, onConnected / onDisconnected, sendSnapshot, notifyUpdate, receiveFromApp. Детали — в BLE v0 spec.
- **Реализации:** реальный BLE-стек (ESP32), mock для тестов без железа.

---

## 3. IGnss / GNSSProvider (Issue #13)

**Источник координат в OOTB v0 — донгл GNSS.** См. [ADR: position source](../adr/ootb_position_source_v0.md), [gnss_v0.md](gnss_v0.md).

- **Ответственность:** предоставление текущей позиции и состояния фикса (NO_FIX / FIX_2D / FIX_3D). Domain и beacon не знают про NMEA/serial — только читают через **GNSSProvider** (реализуется IGnss), используя единый тип **GNSSSnapshot**.

### 3.1. GNSSProvider abstraction — enum and struct

**Правила:** Вышележащие слои (cadence, beacon, NodeTable) зависят **только от GNSSSnapshot**. GNSS provider может быть **stub** или **реальное железо** без изменения архитектуры — контракт один и тот же.

**Enum GNSSFixState:**

| Значение | Описание |
|----------|----------|
| **NO_FIX** | Нет фикса. |
| **FIX_2D** | 2D фикс (широта/долгота). |
| **FIX_3D** | 3D фикс (с высотой, если применимо). |

**Struct GNSSSnapshot (seed, обязательные поля):**

| Поле | Тип | Описание |
|------|-----|----------|
| **fix_state** | GNSSFixState | Текущее состояние фикса. |
| **pos_valid** | bool | true, когда FIX_2D или FIX_3D и координаты осмысленны (достаточно для beacon). |
| **lat_e7** | int32 | Широта × 1e7 (микрограды). |
| **lon_e7** | int32 | Долгота × 1e7 (микрограды). |
| **last_fix_ms** | uint32 | **Монотонный uptime_ms** (мс с включения) момента последнего валидного фикса; используется для вычисления pos_age_s: (now_ms - last_fix_ms) / 1000. |

**Future / TBD (документировать, в seed не обязательны):** sat_count, hdop, speed, course — могут быть добавлены в расширение снапшота позже.

**Метод провайдера:** **getSnapshot() → GNSSSnapshot**. IGnss реализует этот контракт (реальный драйвер или stub заполняют структуру из железа или детерминированно).

### 3.2. IGnss methods (conceptual)

- **getSnapshot()** — основной метод: возвращает **GNSSSnapshot** (fix_state, pos_valid, lat_e7, lon_e7, last_fix_ms). Cadence/beacon/NodeTable используют только его.
- **getPosition()** — возвращает структуру позиции (lat, lon, alt?, fixType, pos_valid, …); может быть реализован поверх getSnapshot() или оставлен для совместимости. Формат полей — см. [gnss_v0.md](gnss_v0.md).
- **getFixType()** — NO_FIX | FIX_2D | FIX_3D; эквивалент snapshot.fix_state.
- **isValidPosition()** — true только если фикс достаточен для beacon (FIX_2D или FIX_3D); эквивалент snapshot.pos_valid.

**pos_age_s** для BLE Health вычисляется из (uptime_now_ms - last_fix_ms) / 1000 или хранится отдельно; TBD если ещё не в [gnss_v0.md](gnss_v0.md).

**Реализации:**

- **Реальный драйвер:** чтение с GNSS-модуля (UART/NMEA или другой протокол) — внутри HAL; маппинг в GNSSSnapshot, domain видит только снапшот.
- **Stub implementation:** детерминированная генерация координат вокруг заданной точки (center, radius, speed); заполняет GNSSSnapshot. См. [gnss_v0.md](gnss_v0.md) § Stub mode.

---

## 4. ILog

- **Ответственность:** кольцевой буфер событий и экспорт (UART и/или BLE characteristic). События: TX/RX, decode ok/err, NodeTable update, BLE notify, GNSS (cold start, fix acquired, fix lost, stale). См. [gnss_v0.md](gnss_v0.md) § Logging requirements, [OOTB plan](../product/OOTB_v0_analysis_and_plan.md) Phase 2.2.
- **Методы (концептуально):** log(level, event, message/payload), getRingBuffer(), exportToUart / exportToBle (TBD).
- **Реализации:** реальный вывод в UART/BLE, mock (in-memory) для тестов.

---

## 5. Моки и тестирование

- Каждый интерфейс допускает **mock-** реализацию для unit/интеграционных тестов без железа.
- Domain- и protocol-слой тестируются с подставленными моками; реальные HAL подключаются только на целевом устройстве.
- Stub IGnss — частный случай мока с детерминированной «позицией» для разработки beacon и NodeTable без GNSS-модуля.

---

## 6. Telemetry contract for BLE v0

**Purpose:** Зафиксировать контракт HAL/domain, необходимый для заполнения BLE характеристик [DeviceInfo и Health](../protocols/ootb_ble_v0.md#1-telemetry-v0-seed-deviceinfo--health), без утечки деталей железа в приложение. BLE-слой **подтягивает** данные из провайдеров; новые поведения не вводятся — только форма контракта.

### 6.1. Откуда BLE берёт данные

- **DeviceInfo** — из **capabilities/identity provider** (редко меняющиеся: идентичность, возможности модуля).
- **Health** — из **status provider** (снапшоты состояния: радио, GNSS, батарея, uptime).
- **NodeTableSnapshot** — из **NodeTable snapshot provider** (см. § 6.4): create_snapshot, get_snapshot_page; BLE вызывает этот контракт для заполнения характеристики NodeTableSnapshot.

Провайдеры реализуются поверх HAL/domain (например, агрегация IRadio + IGnss + источник node_id + батарея + NodeTable); BLE не обращается к железу напрямую.

### 6.2. Contract shape: что должно быть доступно

**Node identity (capabilities/identity provider):**

| Данные | Тип | Описание |
|--------|-----|----------|
| **node_id** | uint64 | Уникальный идентификатор ноды (NodeID); источник — ESP32 unique id; см. [NodeTable v0 — Node identity](ootb_node_table_v0.md#1-node-identity). |
| **short_id** | uint16 или правило | Младшие 16 бит node_id (4 hex chars). Либо провайдер отдаёт short_id, либо правило: `short_id = node_id & 0xFFFF`. |

**Radio status snapshot (status provider → IRadio):**

| Поле | Тип | Описание |
|------|-----|----------|
| **tx_count** | uint32 | Счётчик отправленных пакетов. |
| **rx_count** | uint32 | Счётчик принятых пакетов. |
| **last_rx_rssi** | int8 | RSSI последнего принятого пакета, дБм. |

**Radio capability info (capabilities/identity provider → IRadio):**

| Поле | Тип | Описание |
|------|-----|----------|
| **band_id** | enum | 433 / 868 / … (без утечки частот в Hz). |
| **channel_min** / **channel_max** | uint16 или аналог | Диапазон каналов модуля. |
| **power_min** / **power_max** (или список) | TBD | Поддерживаемый диапазон мощности. |
| **module_model** | string / enum | Модель радио-модуля (например E220-400T30D). |

**GNSS status snapshot (status provider → IGnss):**

| Поле | Тип | Описание |
|------|-----|----------|
| **gnss_state** | enum | NO_FIX / FIX_2D / FIX_3D. |
| **pos_valid** | bool | Валидная позиция для beacon. |
| **pos_age_s** | uint16 | Возраст последней валидной позиции, с. TBD если ещё не в контракте IGnss. |

**Battery (status provider):**

| Поле | Тип | Описание |
|------|-----|----------|
| **battery_mv** | uint16 | Напряжение батареи, мВ. **TBD:** отдельный интерфейс (например IBattery) или чтение через существующий HAL — пока в контракте как обязательное поле для Health; реализация — TBD. |

**Uptime:** `uptime_s` (uint32) — время работы с включения; источник (system HAL или domain) — TBD, но в контракте для Health.

**Device_type, firmware_version:** для DeviceInfo; источник (build-time или runtime) — TBD; в контракте как поля без привязки к конкретному HAL.

### 6.3. Итог

- HAL/domain предоставляют перечисленные данные через провайдеры (identity/capabilities + status snapshots); BLE-слой только читает их и формирует GATT-ответы.
- Поля, для которых источник ещё не определён, помечены TBD, но входят в контракт, чтобы BLE spec и реализация BLE-моста не зависели от деталей железа.

### 6.4. NodeTable snapshot contract (for BLE NodeTableSnapshot characteristic) — Issue #13

**Purpose:** Контракт, который BLE вызывает для постраничного чтения NodeTable ([NodeTableSnapshot characteristic](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic)). Без кода — только форма контракта.

**Методы (концептуально):**

- **create_snapshot() → snapshot_id.** Создаёт снапшот NodeTable на текущий момент; возвращает идентификатор снапшота (uint16 или аналог). BLE вызывает при первом запросе страницы (например page 0 с «новым снапшотом»).
- **get_snapshot_page(snapshot_id, page_index, page_size = 10) → { total_nodes, page_count, records[] }.** Возвращает одну страницу записей для данного снапшота. **total_nodes** — общее число записей в снапшоте; **page_count** — число страниц (ceil(total_nodes / page_size)); **records[]** — массив записей в формате **NodeRecord v1** (см. [ootb_ble_v0.md § 1.3 NodeRecord v1](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic)). Все **timestamp’ы** в NodeTable и в этом контракте — **uptime_ms** (монотонные мс с включения); при экспорте в BLE в записях передаётся **last_seen_age_s** (производный возраст), а не сырой timestamp.
- **release_snapshot(snapshot_id)** — **FUTURE/TBD.** Опционально освободить ресурсы снапшота; в seed может не реализовываться.

**Правила:**

- **Self entry** должна входить в записи снапшота (одна запись с is_self = true); порядок записей (в т.ч. self на странице) — TBD (например self всегда первая, или по порядку last_seen).
- **is_grey** в flags каждой записи вычисляется по политике NodeTable (spec #12; [ootb_node_table_v0.md § 4.1 UI state](../firmware/ootb_node_table_v0.md#41-ui-state-normal-vs-grey)): expected_interval_s, grace_s, переход в GREY при (now - last_seen) > expected_interval_s + grace_s.
- **Records** возвращаются в **NodeRecord v1** layout (node_id, short_id, flags, last_seen_age_s, lat_e7, lon_e7, pos_age_s, last_rx_rssi, last_seq) — см. [ootb_ble_v0.md § 1.3](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic).
- **Seed: best effort** — снапшот может быть не строго консистентным (страницы могут отражать слегка разные моменты); допустимо для seed. Строгая консистенция — FUTURE.
