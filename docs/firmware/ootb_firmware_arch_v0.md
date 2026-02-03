# OOTB Firmware architecture v0

**Purpose:** Слои, границы и модель выполнения прошивки OOTB v0. Issue [#11](https://github.com/AlexanderTsarkov/naviga-app/issues/11).

**Related:** [HAL contracts](hal_contracts_v0.md), [OOTB plan § 3.5](../product/OOTB_v0_analysis_and_plan.md), [Radio v0](../protocols/ootb_radio_v0.md), [BLE v0](../protocols/ootb_ble_v0.md).

---

## 1. Scheduler / tick model (seed)

**Модель:** кооперативная планировка **без RTOS**. База — периодический тик + событийно-управляемые части.

- **Base tick:** единый базовый тик (предлагается **100 ms**). По тику вызываются периодические задачи; между тиками — обработка событий (RX, BLE).
- **Event-driven parts:** приём по радио и BLE обрабатываются по событиям/поллингему в рамках того же цикла; задачи не прерывают друг друга (cooperative).

**Задачи (архитектурный уровень):**

| Задача | Тип | Описание |
|--------|-----|----------|
| **GNSSTask** | periodic | Опрос **GNSSProvider** (getSnapshot()) с частотой **1 Hz**; обновление **последнего GNSSSnapshot + timestamp**. Обновление **self-записи в NodeTable** из этого снапшота (позиция, pos_valid, last_seen). Запуск по базовому тику (подпериод 1 s). |
| **RadioRxTask** | frequent | Поллинг/приём по IRadio; парсинг GEO_BEACON; обновление NodeTable (записи удалённых нод). Вызывается часто (каждый тик или по событию RX). |
| **BeaconTxTask** | periodic | Решение «отправлять ли beacon» по cadence (движение, max_silence, jitter); при положительном решении — формирование payload и IRadio.send(). **Использует только снапшот** (pos_valid, lat/lon, pos_age из последнего GNSSSnapshot); **не обращается к GNSS напрямую**. Запуск по тику. |
| **BleTask** | event-driven | Обработка BLE: подключение, запросы READ (DeviceInfo, Health, NodeTableSnapshot). Чтение данных из провайдеров (identity, status, NodeTable snapshot); формирование ответов. Без блокирующих вызовов. |

**GNSS specifics (seed):**

- **GNSSTask** опрашивает **GNSSProvider** (IGnss.getSnapshot()) с периодом **1 Hz** и сохраняет последний **GNSSSnapshot** + время обновления. **NodeTable self entry** обновляется из этого снапшота (lat_e7, lon_e7, pos_valid, last_fix_ms → pos_age).
- **BeaconTxTask** читает **только этот снапшот** (pos_valid, lat_e7/lon_e7, pos_age); вызовов GNSS/IGnss из BeaconTxTask нет.
- См. [HAL § 3 GNSSProvider / GNSSSnapshot](hal_contracts_v0.md#31-gnssprovider-abstraction--enum-and-struct).

**TX scheduling (BeaconTxTask):**

- **Should-send decision:** BeaconTxTask на каждом тике вычисляет **should_send_now** по правилам cadence (min_update_interval_s / min_time_s, movement / min_distance_m, max_silence; см. [Radio v0 § 4 Beacon cadence](../protocols/ootb_radio_v0.md#4-beacon-cadence-seed)). Дефолт Sprint 1 («Human» profile): min_distance_m=25, min_time_s=20, max_silence_s=80 — см. [ootb_scope_v0.md § Sprint 1 default cadence](../product/ootb_scope_v0.md#1-ootb-sprint-breakdown).
- **Before TX:** к запланированному моменту отправки применяется **jitter** (случайное смещение), затем выполняется попытка TX.
- **LBT and retry:** если у модуля включён LBT — используется проверка канала перед TX. Если модуль/библиотека сообщает **busy** или **send fail:** применяется **ограниченный случайный backoff** и **ограниченное число retry** (1–2 для beacon; см. [Radio v0 § 5 Contention handling](../protocols/ootb_radio_v0.md#5-contention-handling-seed)).
- **Non-blocking:** повторные попытки **планируются на следующий тик (или следующий допустимый слот)**, а не выполняются в цикле ожидания (no busy-wait). За один тик — одна попытка TX или один шаг backoff/retry; следующий тик решает, повторять ли попытку или выйти из цикла и пропустить beacon.

**Правило:** задачи **не блокируют** выполнение; каждая задача — ограниченное по времени выполнение (bounded execution). Долгие операции (например, ожидание ответа от радио) не допускаются в основном цикле; только поллинг или короткие callback-подобные шаги.

**Потоки данных (концептуально):**

- **GNSS → snapshot → self state → beacon decision:** GNSSTask опрашивает GNSSProvider (getSnapshot()) 1 Hz → обновляет последний GNSSSnapshot и **self-запись в NodeTable** (позиция, pos_valid, last_seen/pos_age) → BeaconTxTask читает **снапшот/self-состояние** и cadence-параметры → решение TX и формирование GEO_BEACON (без прямых вызовов GNSS).
- **Radio RX → parse → NodeTable:** IRadio (receive/poll) → парсинг GEO_BEACON → обновление/вставка записи в NodeTable (node_id, position, last_seen, last_rx_rssi, last_seq).
- **BLE reads → snapshots:** Запросы READ от приложения → BleTask запрашивает у провайдеров (DeviceInfo из identity, Health из status, NodeTableSnapshot из create_snapshot / get_snapshot_page) → возврат payload в BLE-транспорт.

**Примечание (seed):** В seed используется **GNSSStub** provider (детерминированная позиция для разработки). Позже замена на **u-blox M8N UART** provider выполняется **без переработки** cadence и NodeTable — контракт GNSSSnapshot и поток данных остаются теми же.

**Timebase (seed):** Вся внутренняя временная логика использует **монотонный uptime**; wall-clock не используется (избегаем смешивания с временем GNSS/телефона).

- **now_ms** — единственный источник «текущего времени» для логики: **Timebase** = платформа (например millis() / esp_timer в мс с включения). Все сравнения и таймауты опираются на now_ms.
- **Производные возраста:** age_s = (now_ms - event_ms) / 1000 (секунды с момента события). Используется для pos_age_s, last_seen_age_s, cadence intervals и т.д.

Без кода — только архитектурное описание; детали реализации (очереди, приоритеты внутри тика) — TBD при реализации.

---

## 2. Common utils layout (seed)

Краткая раскладка областей кода (концептуально; без кода):

| Область | Содержимое |
|---------|------------|
| **common/types** | GeoPoint (lat_e7, lon_e7), NodeId, типы для timestamps (uptime ms и т.п.). |
| **common/math/geo_distance** | Расстояние и bearing: geo_distance_m(), geo_bearing_deg() (и при необходимости geo_distance_bearing()); контракт — [geo_utils_v0.md](geo_utils_v0.md). |
| **common/proto** | Упаковка/разбор радио-фреймов (GEO_BEACON и т.д.); packing/parsing по [Radio v0](../protocols/ootb_radio_v0.md). |
| **platform** | Специфика ESP32 (инициализация, таймеры, UART/SPI для драйверов). |
| **app** | Доменная логика и сервисы: NodeTable, beacon cadence, BLE-сервисы (DeviceInfo, Health, NodeTableSnapshot). |
| **drivers** | Реализации HAL: радио-модуль (E220 и т.п.), GNSS provider (stub или u-blox M8N). |

**Использование geo:**

- **Beacon cadence** использует **geo_distance_m()** для оценки **min_distance** (сравнение текущей позиции self с позицией на момент последнего TX); см. [geo_utils_v0.md](geo_utils_v0.md).
- Слои **UI/telemetry** позже могут использовать **geo_bearing_deg()** (направление на ноду); в **seed** отображение bearing не требуется.
