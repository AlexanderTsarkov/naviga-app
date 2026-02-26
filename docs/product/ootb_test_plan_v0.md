# OOTB Test Plan v0

**Purpose:** Сценарии проверки OOTB v0; что логировать и собирать для evidence. Issue [#16](https://github.com/AlexanderTsarkov/naviga-app/issues/16).

**Related:** [OOTB plan](OOTB_v0_analysis_and_plan.md), [OOTB Radio v0](../protocols/ootb_radio_v0.md), [POC E220 evidence](../firmware/poc_e220_evidence.md), [Logging v0](logging_v0.md).

---

## 1. Seed beacon cadence tests

**Спецификация поведения:** [OOTB Radio v0 — Beacon cadence (seed)](../protocols/ootb_radio_v0.md#4-beacon-cadence-seed) (параметры min_update_interval_s, min_movement_m, max_silence, jitter_pct; seed algorithm; airtime constraint).

Тесты — **наблюдаемость/поведение** (логи, осциллограф/анализ времени TX), не кодовые unit tests.

| ID | Название | Описание | Критерий прохождения |
|----|----------|----------|----------------------|
| **BC-1** | **Movement trigger** | При смещении позиции **≥ min_movement_m** после истечения **min_update_interval_s** с момента последней отправки → beacon **отправляется**. | В логах/на приёмной стороне фиксируется TX beacon после движения ≥ min_movement_m и паузы ≥ min_update_interval_s. |
| **BC-2** | **No movement keep-alive** | При малом смещении (movement < min_movement_m) beacon по движению **подавляется**; но если время с последнего TX **≥ max_silence** → отправляется **keep-alive beacon**. | В статичном/малоподвижном сценарии TX не чаще min_update_interval по движению, но не реже max_silence (наблюдаются keep-alive). |
| **BC-3** | **Jitter** | При двух (или более) нодах с **одинаковыми** настройками cadence наблюдается **рассинхронизация** моментов TX по времени — нет идеальной синхронной «дроби» (одновременных передач каждый период). | Интервалы между TX разных нод не совпадают стабильно; в логах/анализе виден разброс моментов отправки (jitter_pct). |
| **BC-4** | **Airtime verify (TBD)** | Измерить/зафиксировать **время передачи одного GEO_BEACON** в эфире (от начала до конца передачи). Цель по спецификации: **≤ 300 ms**. | Результат измерения записан в evidence/logs; значение ≤ 300 ms (или зафиксировано отклонение и причина для TBD). |

**Артефакты:** логи TX (timestamp, причина: movement / keep-alive); при необходимости — осциллограф или радио-анализ для BC-4. Ссылка на параметры и алгоритм: [ootb_radio_v0.md § 3](../protocols/ootb_radio_v0.md#4-beacon-cadence-seed).

---

## 1a. Contention handling tests (seed)

**Спецификация:** [OOTB Radio v0 — Contention handling (seed)](../protocols/ootb_radio_v0.md#5-contention-handling-seed) (LBT, jitter, bounded backoff/retry, no busy-wait).

Тесты — **поведенческая проверка** снижения коллизий и устойчивости при занятом канале; логи и наблюдение за видимостью нод.

| ID | Название | Описание | Критерий прохождения |
|----|----------|----------|----------------------|
| **CH-1** | **Jitter prevents persistent collisions** | Два донгла с **одинаковым номинальным интервалом** beacon (одинаковые cadence). Проверить, что **jitter** предотвращает устойчивые коллизии: **обе ноды видны** на приёмной стороне (или взаимно в NodeTable) в течение времени наблюдения; нет ситуации «одна нода постоянно перебивает другую». **KPI (измеримо):** за **5 минут** каждая нода видит другую **≥ N раз** (N TBD, например 10–20 наблюдений в NodeTable или по логам RX); обе ноды стабильно в списке. | В логах/NodeTable обе ноды периодически наблюдаются; интервалы TX двух нод не совпадают стабильно (рассинхронизация за счёт jitter). **KPI выполнен:** каждая из двух нод видит вторую ≥ N раз за 5 мин. См. [Radio v0 § 5.2](../protocols/ootb_radio_v0.md#52-software-level-randomization). |
| **CH-2** | **LBT enabled in OOTB public profile** | Подтвердить, что в OOTB Public profile (channel 1) **LBT включён**. Способ проверки: **config readback** (если доступен через BLE/сервисный режим) или **лог прошивки** (событие/флаг «LBT enabled» при инициализации или при смене профиля). Задокументировать, **как именно** подтверждается (команда, поле в логе, скриншот). | LBT включён для channel 1 (public); в тестовой документации или отчёте зафиксирован способ проверки (readback / лог) и результат. См. [Radio v0 § 5.1](../protocols/ootb_radio_v0.md#51-lbt-listen-before-talk). |
| **CH-3** | **Backoff/retry bounded** | При занятом канале или сбое TX: **backoff и retry ограничены** (1–2 retry для beacon); нет бесконечных циклов повторных попыток. В логах видны переходы: busy → backoff → retry или skip (пропуск цикла beacon). | Логи показывают ограниченное число retry и либо успех, либо skip; отсутствуют признаки бесконечного retry (watchdog не срабатывает; счётчик retry в логе не растёт неограниченно). См. [Radio v0 § 5.2](../protocols/ootb_radio_v0.md#52-software-level-randomization). |
| **CH-4** | **Stability under temporary congestion** | Имитация **временной перегрузки канала** (например, несколько устройств на одном канале, или генератор помех на короткий период). Устройство не падает, **watchdog не срабатывает**; после снятия перегрузки работа восстанавливается. | Нет крашей, нет watchdog reset во время и после перегрузки; beacon и приём восстанавливаются. См. [Radio v0 § 5](../protocols/ootb_radio_v0.md#5-contention-handling-seed). |
| **CH-5** | **tx_busy/backoff observed, no stalls** | При работе двух (или более) нод на одном канале **tx_busy / backoff** должны **иногда** наблюдаться (логи или счётчики tx_busy, события RADIO_TX_BUSY_BACKOFF / RADIO_TX_SKIP_CYCLE; см. [logging_v0.md § 2.1](logging_v0.md#21-event-ids-seed--contention-handling)), но при этом **нет бесконечных retry и нет «залипания»** (обе ноды продолжают успешно передавать и видеть друг друга; tx_ok растёт, нет длительных периодов без успешного TX). | В логах/счётчиках зафиксированы отдельные случаи tx_busy или skip; при этом retry ограничены, обе ноды видны друг другу (KPI CH-1 выполнен), нет индикации stall (успешные TX продолжаются). См. [Radio v0 § 5.2](../protocols/ootb_radio_v0.md#52-software-level-randomization), [Logging v0 event IDs](logging_v0.md#21-event-ids-seed--contention-handling). |

**Артефакты:** логи TX/RX и событий LBT/backoff/retry (в т.ч. RADIO_TX_* event_id); для CH-1 — подсчёт «видит другую ноду ≥ N раз за 5 мин»; для CH-2 — описание способа проверки LBT (readback/лог); для CH-4 — сценарий перегрузки и логи до/после; для CH-5 — выдержки логов/счётчиков tx_busy и tx_ok. Ссылки: [Radio v0 § 5 Contention handling](../protocols/ootb_radio_v0.md#5-contention-handling-seed), [Firmware arch § TX scheduling](../firmware/ootb_firmware_arch_v0.md#1-scheduler--tick-model-seed), [Logging v0 § 2.1](logging_v0.md#21-event-ids-seed--contention-handling).

---

## 2. BLE Telemetry v0 verification

**Спецификация:** [OOTB BLE v0 — Telemetry v0 (seed): DeviceInfo + Health](../protocols/ootb_ble_v0.md#1-telemetry-v0-seed-deviceinfo--health) (DeviceInfo characteristic § 1.1, Health characteristic § 1.2).

Тесты — **поведенческая проверка** (приложение читает/получает NOTIFY, отображает данные), не unit tests.

| ID | Название | Описание | Критерий прохождения |
|----|----------|----------|----------------------|
| **BLE-1** | **DeviceInfo READ** | Приложение может **прочитать** характеристику DeviceInfo и отображает: node_id / short_id, firmware_version, band_id, radio_module_model, channel_min/channel_max, power_min/power_max (или диапазон), capabilities (has_gnss, has_radio). | После подключения по BLE приложение успешно читает DeviceInfo и показывает перечисленные поля (или их подмножество по UI; все поля из spec доступны при чтении). |
| **BLE-2** | **Health READ** | Приложение может **прочитать** характеристику Health и отображает: uptime_s, gnss_state, pos_valid, pos_age_s, battery_mv, radio_tx_count, radio_rx_count, last_rx_rssi. | После подключения приложение успешно читает Health и показывает перечисленные поля. |
| **BLE-3** | **Health NOTIFY (events only)** | При смене состояния GNSS (NO_FIX → FIX_2D/FIX_3D или FIX → NO_FIX) срабатывает **Health NOTIFY**. При подключении BLE допустима опциональная «начальная» нотификация Health (зафиксировать ожидаемое поведение в spec). В idle (без смены состояния) **периодических** нотификаций нет — подтвердить отсутствие спама. | NOTIFY приходит при смене gnss_state; при подключении — начальная нотификация или нет (согласно документированному поведению); в стабильном состоянии без событий — нет потока нотификаций. |
| **BLE-4** | **Logging tie-in (optional)** | Соответствующие события также попадают в логи прошивки: смена GNSS fix (fix acquired / fix lost), подключение BLE. | В логах (UART или экспорт по BLE) видны события GNSS fix/lost и BLE connect; при необходимости — сверка с моментами Health NOTIFY. |

**Артефакты:** скриншоты/логи приложения (DeviceInfo, Health READ); лог моментов Health NOTIFY и событий GNSS/BLE; при необходимости — лог прошивки. Ссылки на spec: [DeviceInfo § 1.1](../protocols/ootb_ble_v0.md#11-deviceinfo-characteristic), [Health § 1.2](../protocols/ootb_ble_v0.md#12-health-characteristic).

---

## 2a. Persistent log (offline then dump) — Issue #20

**Спецификация:** [Logging v0](logging_v0.md) (persistent ring in flash, batching, serial dump).

**Сценарий:** Устройство работает **offline** (без подключённого USB/serial). За это время происходят события: GNSS fix acquired / fix lost, radio RX/TX, NodeTable (add, grey transition, eviction), при последующем подключении — BLE READ (DeviceInfo, Health, NodeTableSnapshot). После **повторного подключения** serial (USB) выполняется **dump логов** (см. [logging_v0.md § 4 Log retrieval](logging_v0.md#4-log-retrieval)).

| ID | Название | Описание | Критерий прохождения |
|----|----------|----------|----------------------|
| **LOG-1** | **Offline log + dump** | Устройство логирует события в персистентный ring (flash) пока **не подключено** по USB. После подключения по serial выполняется выгрузка лога. В выгруженном дампе присутствуют события: **GNSS fix acquired / fix lost**, **radio RX/TX** (beacon sent, packet received), **NodeTable** (add node, grey transition, evict), **BLE read** (DeviceInfo, Health, NodeTableSnapshot — при наличии подключения приложения после reconnect). | Дамп по serial после reconnect содержит перечисленные типы событий с корректными t_ms/event_id; порядок и полнота — в пределах retention (~30 min) и размера кольца. См. [logging_v0.md](logging_v0.md). |

**Артефакты:** сценарий «offline N минут + reconnect + dump»; сохранённый вывод serial dump с аннотацией событий. Ссылка: [Logging v0](logging_v0.md).

---

## 3. Versioning & compatibility checks (seed)

**Purpose:** Поведенческая проверка устойчивости к неизвестным/новым версиям и лишним байтам; не строгие unit tests. Спецификация: [OOTB Radio v0 — Radio frame format (v0)](../protocols/ootb_radio_v0.md#3-radio-frame-format-v0), [OOTB BLE v0 — Telemetry + Compatibility rules](../protocols/ootb_ble_v0.md#1-telemetry-v0-seed-deviceinfo--health).

| ID | Область | Описание | Критерий прохождения |
|----|---------|----------|----------------------|
| **VC-1** | **Radio** | **Unknown msg_type** — пакет с неизвестным msg_type **игнорируется**. Устройство не падает; NodeTable не портится (нет записи от неизвестного типа). | При приёме пакета с неизвестным msg_type устройство продолжает работать; NodeTable и логи без некорректных записей. См. [ootb_radio_v0.md § 3.1 Rules](../protocols/ootb_radio_v0.md#31-header-every-frame). |
| **VC-2** | **Radio** | **Unsupported MAJOR ver** для известного msg_type — пакет **игнорируется**. Устройство не падает; NodeTable не обновляется по несовместимой версии. | При приёме GEO_BEACON с неподдерживаемой MAJOR версией пакет отбрасывается; устройство стабильно. См. [ootb_radio_v0.md § 3.1 Rules](../protocols/ootb_radio_v0.md#31-header-every-frame). |
| **VC-3** | **Radio** | **GEO_BEACON v1 core** парсится; **лишние хвостовые байты** (extensions) **игнорируются** без ошибок (forward compatibility). _(Note: field list `node_id, pos_valid, position, pos_age_s, seq` reflects legacy codec layout. Canon target is packed24 v0 per [#301](https://github.com/AlexanderTsarkov/naviga-app/issues/301); test case to be updated in PR-B.)_ | При приёме GEO_BEACON с дополнительными байтами после core полей устройство корректно разбирает core и не падает; лишние байты не влияют на NodeTable/логи. См. [ootb_radio_v0.md § 3.2 GEO_BEACON](../protocols/ootb_radio_v0.md#32-geo_beacon-msg_type). |
| **VC-4** | **BLE** | **DeviceInfo** содержит **ble_schema_ver** и **format_ver**; **Health** содержит **format_ver** (первое поле payload). | При чтении DeviceInfo и Health приложение получает format_ver (и в DeviceInfo — ble_schema_ver); значения соответствуют spec. См. [ootb_ble_v0.md § 1.1, § 1.2](../protocols/ootb_ble_v0.md#11-deviceinfo-characteristic). |
| **VC-5** | **BLE** | **Клиент/приложение** успешно читает телеметрию даже при **лишних хвостовых байтах** в payload (имитация: парсер на стороне приложения игнорирует хвост, или прошивка в dev-режиме добавляет dummy-байты). | При payload с дополнительными байтами после известных полей приложение читает только известные поля и отображает их корректно; крашей/ошибок нет. См. [ootb_ble_v0.md § 1.3 Compatibility rules](../protocols/ootb_ble_v0.md#13-compatibility-rules). |

**Артефакты:** логи приёма (Radio: отброшенные msg_type/ver); логи/скриншоты приложения (BLE: чтение DeviceInfo/Health с format_ver и при наличии лишних байтов). Ссылки: [Radio frame format § 3](../protocols/ootb_radio_v0.md#3-radio-frame-format-v0), [BLE Telemetry § 1](../protocols/ootb_ble_v0.md#1-telemetry-v0-seed-deviceinfo--health), [BLE Compatibility § 1.3](../protocols/ootb_ble_v0.md#13-compatibility-rules).

---

## 4. NodeTable behavioral tests

**Спецификация:** [OOTB NodeTable v0 — UI state policy & capacity/eviction](../firmware/ootb_node_table_v0.md#4-ui-state-policy-seed-two-state--capacity--eviction) (expected_interval_s, grace_s, NORMAL/GREY, max_nodes, eviction rules).

Тесты — **поведенческая проверка** NodeTable (логика NORMAL/GREY, вытеснение); не unit tests.

| ID | Название | Описание | Критерий прохождения |
|----|----------|----------|----------------------|
| **NT-1** | **NORMAL → GREY transition** | При **expected_interval_s** = глобальный max_silence_s из radio cadence и **grace_s** = max(2, round(expected_interval_s × 0.25)) нода переходит в **GREY** только после **(expected_interval_s + grace_s)** без пакетов от этой ноды. | Без приёма пакетов от удалённой ноды в течение > (expected_interval_s + grace_s) её запись в NodeTable (или отображение в UI) переходит в GREY; до истечения этого интервала — NORMAL. См. [NodeTable v0 § 4.1 UI state](../firmware/ootb_node_table_v0.md#41-ui-state-normal-vs-grey). |
| **NT-2** | **No flicker** | Имитация **редкой потери пакета** (один пропущенный beacon при регулярном потоке): нода должна оставаться **NORMAL** благодаря grace_s (не переходить в GREY из-за одного пропуска). | При периодическом приёме пакетов с интервалом ≤ expected_interval_s с одним пропуском нода остаётся NORMAL; кратковременного перехода в GREY нет. См. [NodeTable v0 § 4.1](../firmware/ootb_node_table_v0.md#41-ui-state-normal-vs-grey). |
| **NT-3** | **Eviction** | Заполнить таблицу до **max_nodes (100)** разными node_id (включая self). Добавить новую ноду: вытесняются **сначала самые старые GREY** записи (LRU по last_seen); **self (is_self = true) никогда не вытесняется**. | При max_nodes и появлении новой ноды удаляется запись с is_self=false, состоянием GREY и минимальным last_seen; self остаётся в таблице. См. [NodeTable v0 § 4.2 Capacity & eviction](../firmware/ootb_node_table_v0.md#42-capacity--eviction). |

**Артефакты:** логи NodeTable (last_seen, состояние NORMAL/GREY); при eviction — список записей до/после добавления новой ноды. Ссылки: [NodeTable § 4.1 UI state](../firmware/ootb_node_table_v0.md#41-ui-state-normal-vs-grey), [NodeTable § 4.2 Eviction](../firmware/ootb_node_table_v0.md#42-capacity--eviction).

---

## 5. NodeTableSnapshot paging verification (Issue #16)

**Спецификация:** [OOTB BLE v0 — NodeTableSnapshot characteristic](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic) (request/response semantics, NodeRecord v1 layout); [OOTB NodeTable v0 — UI state & record fields](../firmware/ootb_node_table_v0.md) (spec #12).

**Seed note:** Нет NOTIFY при изменении NodeTable; приложение обновляет снапшот по необходимости **повторным чтением** (poll via reads). См. [ootb_ble_v0.md § 1.3 Notes](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic).

Тесты — **поведенческая проверка** постраничного чтения NodeTable через BLE; приложение читает характеристику NodeTableSnapshot и восстанавливает список нод.

| ID | Название | Описание | Критерий прохождения |
|----|----------|----------|----------------------|
| **NTS-1** | **Paging basic** | При **>10 нодах** в NodeTable приложение читает **page 0** и получает **snapshot_id**, **total_nodes**, **page_count**. Затем читает все страницы **0 .. page_count−1** и **восстанавливает полный список** нод (без дубликатов, без пропусков по ожидаемому total_nodes). | После чтения page 0 приложение имеет snapshot_id, total_nodes, page_count; после чтения страниц 0..page_count−1 собранный список содержит ровно total_nodes записей и совпадает по составу node_id с ожидаемой NodeTable. |
| **NTS-2** | **Limits: 50 nodes** | Создать **50 нод** в NodeTable (включая self). Ожидается **5 страниц** при page_size=10. | При чтении NodeTableSnapshot: total_nodes=50, page_count=5; приложение успешно читает 5 страниц и получает 50 записей. |
| **NTS-3** | **Limits: 100 nodes** | Создать **100 нод** (max_nodes). Ожидается **10 страниц** при page_size=10. | total_nodes=100, page_count=10; приложение читает 10 страниц и получает 100 записей. |
| **NTS-4** | **Record integrity** | Каждая запись в ответе содержит поля **NodeRecord v1**: node_id, short_id, **flags** (is_self, pos_valid, is_grey), last_seen_age_s, lat_e7/lon_e7 при pos_valid, pos_age_s, last_rx_rssi, last_seq. **Self entry** присутствует в снапшоте и имеет **flags.is_self=1**. | Все записи на всех страницах имеют корректный формат NodeRecord v1; ровно одна запись с is_self=1; при pos_valid=1 координаты и pos_age_s присутствуют. См. [ootb_ble_v0.md § 1.3 NodeRecord v1](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic). |
| **NTS-5** | **Grey flag behavior** | Ноды, для которых **(now − last_seen) > expected_interval_s + grace_s**, должны иметь **flags.is_grey=1**. | В снапшоте записи «stale» нод (без приёма пакетов дольше expected_interval_s + grace_s) имеют is_grey=1; «живые» ноды — is_grey=0. См. [NodeTable v0 § 4.1 UI state](../firmware/ootb_node_table_v0.md#41-ui-state-normal-vs-grey) (spec #12). |
| **NTS-6** | **Stability: new snapshot on page 0** | Повторное чтение **page 0 с snapshot_id=0** (или эквивалент «новый снапшот») создаёт **новый snapshot_id** на устройстве (или документировано повторное использование того же id). Приложение должно корректно обрабатывать оба варианта (использовать возвращённый snapshot_id для последующих страниц). | Повторный «start snapshot» (read page 0, snapshot_id=0) либо возвращает новый snapshot_id, либо документированное поведение (reuse); приложение не ломается и успешно читает все страницы по актуальному snapshot_id. |

**Артефакты:** логи/скриншоты приложения (чтение страниц 0..N, полученные snapshot_id/total_nodes/page_count, список node_id по страницам); для NTS-4/NTS-5 — примеры сырых записей с полями flags; для NTS-6 — сценарий повторного read page 0 и результат. Ссылки: [ootb_ble_v0.md § 1.3 NodeTableSnapshot](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic), [NodeTable v0](../firmware/ootb_node_table_v0.md) (spec #12).

---

## 6. Seed robustness checklist

**Purpose:** Pass/fail пункты для проверки устойчивости seed-реализации (парсинг, лимиты, дедупликация, paging). Каждый пункт — критерий «прошёл / не прошёл».

| # | Item | Pass | Fail | Spec |
|---|------|------|------|------|
| **RC-1** | **Drop unknown msg_type** | Пакет с неизвестным msg_type отбрасывается; устройство не падает; NodeTable не обновляется по такому пакету. | Пакет обрабатывается или вызывает сбой. | [ootb_radio_v0.md § 3.1 Rules](../protocols/ootb_radio_v0.md#31-header-every-frame) |
| **RC-2** | **Drop unsupported MAJOR version** | Пакет с неподдерживаемой MAJOR версией (известный msg_type) отбрасывается; устройство стабильно. | Пакет парсится или вызывает сбой. | [ootb_radio_v0.md § 3.1 Rules](../protocols/ootb_radio_v0.md#31-header-every-frame) |
| **RC-3** | **Length checks before parsing** | Перед разбором payload выполняется проверка длины (минимум под core GEO_BEACON); при нехватке байт пакет отбрасывается без чтения за границей. | Парсинг без проверки длины; возможен buffer overread / сбой. | [ootb_radio_v0.md § 3.2 GEO_BEACON](../protocols/ootb_radio_v0.md#32-geo_beacon-msg_type) |
| **RC-4** | **Dedup by (node_id, seq)** | Повторный приём beacon с тем же (node_id, seq) не создаёт дубликат и не перезаписывает запись некорректно; обновление только при новом seq (или по политике «не старее текущего»). | Дубликаты или некорректные обновления по одному и тому же (node_id, seq). | [ootb_radio_v0.md § 3.2](../protocols/ootb_radio_v0.md#32-geo_beacon-msg_type), [NodeTable v0](../firmware/ootb_node_table_v0.md) |
| **RC-5** | **NodeTable eviction when >100** | При заполнении таблицы (≥ max_nodes) вытесняется **самая старая GREY** запись (LRU по last_seen); **self (is_self=true) никогда не вытесняется**. | Вытесняется self или не-GREY; или таблица растёт >100. | [ootb_node_table_v0.md § 4.2 Capacity & eviction](../firmware/ootb_node_table_v0.md#42-capacity--eviction) |
| **RC-6** | **BLE paging stable for 50+ nodes** | NodeTableSnapshot при 50+ нодах: приложение читает все страницы (0..page_count−1), получает полный набор записей без крашей и без потери/дубликатов; total_nodes и page_count согласованы. | Краш, неверный page_count, пропуски или дубликаты записей при 50+ нодах. | [ootb_ble_v0.md § 1.3 NodeTableSnapshot](../protocols/ootb_ble_v0.md#13-nodatablesnapshot-characteristic) |
