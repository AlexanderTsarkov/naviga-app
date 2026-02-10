# OOTB BLE v0 — спецификация

**Purpose:** GATT-контракт между донглом и приложением OOTB v0: сервисы, характеристики, форматы. Issue [#15](https://github.com/AlexanderTsarkov/naviga-app/issues/15).

**Related:** [OOTB plan](../product/OOTB_v0_analysis_and_plan.md), [NodeTable v0](../firmware/ootb_node_table_v0.md), [Radio v0](ootb_radio_v0.md).

---

## 1. Telemetry v0 (seed): DeviceInfo + Health

Минимальный набор характеристик для seed: идентификация устройства и телеметрия состояния. Конфигурация (Channel, Power, профили) в seed не экспонируется отдельной характеристикой — см. seed constraint ниже.

---

### 1.1. DeviceInfo characteristic

- **Properties:** READ
- **Purpose:** статические или редко меняющиеся данные об устройстве и возможностях (identity, capabilities).

**Payload rules:** Первое поле в payload — **format_ver** (u8). Приложение/клиенты должны **игнорировать неизвестные хвостовые байты** для forward compatibility (или использовать TLV в будущем; seed читает только известные поля).

**Fields (order in payload):**

| Поле | Тип | Описание |
|------|-----|----------|
| **format_ver** | u8 | **Первое поле.** Версия формата payload этой характеристики; при неизвестном значении клиент читает только известные поля или отбрасывает хвост. |
| **ble_schema_ver** | u8 | Версия GATT/payload-схемы для этого сервиса (общий индикатор совместимости). |
| **radio_proto_ver** | u8 (optional) | Опционально для seed: простой индикатор версии радио-протокола (минимально — один байт; структура — TBD при необходимости). |
| **node_id** | uint64 | Уникальный идентификатор ноды (NodeID); см. [NodeTable v0 — Node identity](../firmware/ootb_node_table_v0.md#1-node-identity). |
| **short_id** | uint16 | Младшие 16 бит node_id для UI/logging (4 hex chars). |
| **device_type** | enum | Тип устройства: `dongle` для seed. |
| **firmware_version** | string или bytes | Версия прошивки; выбрать один формат и зафиксировать кодировку (например UTF-8 string или фиксированный байтовый формат). TBD. |
| **radio_module_model** | string / enum | Модель радио-модуля (например E220-400T30D). |
| **band_id** | enum | Диапазон: 433 / 868 / … (см. [radio profile presets](radio_profile_presets_v0.md)). |
| **power_min** / **power_max** | или список | Минимальная и максимальная поддерживаемая мощность (или список уровней — TBD). |
| **channel_min** / **channel_max** | uint16 или аналог | Диапазон каналов, поддерживаемый модулем. |
| **network_mode** | enum | PUBLIC / PRIVATE; см. [Radio v0 § 6 Network/Session identity](../protocols/ootb_radio_v0.md#6-networksession-identity-seed). |
| **channel_id** | uint16 | Текущий канал (1 = OOTB Public; 2..N = private). |
| **public_channel_id** | uint16 | Константа **1** — OOTB Public network. |
| **private_channel_min** | uint16 | Минимальный канал для private: **2**. |
| **private_channel_max** | uint16 | Максимальный канал для private: **N** (TBD по возможностям модуля). |
| **capabilities** | bitmap | Битовая маска: `has_gnss`, `has_radio`; остальные биты — только если известны. |

**Binary layout (format_ver = 1):** Источник упаковки — `firmware/protocol/ble_node_table_bridge.cpp` → `BleNodeTableBridge::update_device_info()`. Все многобайтовые числа — **little-endian**. Строки — **length-prefixed**: u8 длина, затем N байт UTF-8 (без null-terminator). Поле **radio_proto_ver** присутствует в payload только если прошивка устанавливает `include_radio_proto_ver` (seed: да).

| Offset | Size | Type | Endian/Encoding | Field name | Notes |
|--------|------|------|------------------|------------|-------|
| 0 | 1 | u8 | — | format_ver | Должен быть 1 для данной раскладки. |
| 1 | 1 | u8 | — | ble_schema_ver | Версия схемы GATT/payload. |
| 2 | 1 | u8 | — | radio_proto_ver | Опционально; при отсутствии парсер пробует без него (см. app). |
| 3 | 8 | uint64 | LE | node_id | NodeID; см. NodeTable v0. |
| 11 | 2 | uint16 | LE | short_id | Display id (4 hex). |
| 13 | 1 | u8 | — | device_type | 1 = dongle (seed). |
| 14 | 1 | u8 | — | firmware_version_len | Длина строки в байтах. |
| 15 | N | bytes | UTF-8 | firmware_version | N = firmware_version_len. |
| 15+N | 1 | u8 | — | radio_module_model_len | Длина строки. |
| 16+N | M | bytes | UTF-8 | radio_module_model | M = radio_module_model_len. |
| 16+N+M | 2 | uint16 | LE | band_id | 433 / 868 / … |
| 18+N+M | 2 | uint16 | LE | power_min | Минимальная мощность. |
| 20+N+M | 2 | uint16 | LE | power_max | Максимальная мощность. |
| 22+N+M | 2 | uint16 | LE | channel_min | Диапазон каналов. |
| 24+N+M | 2 | uint16 | LE | channel_max | |
| 26+N+M | 1 | u8 | — | network_mode | 0 = Public, 1 = Private (TBD). |
| 27+N+M | 2 | uint16 | LE | channel_id | Текущий канал (1 = Public). |
| 29+N+M | 2 | uint16 | LE | public_channel_id | Константа 1. |
| 31+N+M | 2 | uint16 | LE | private_channel_min | Обычно 2. |
| 33+N+M | 2 | uint16 | LE | private_channel_max | |
| 35+N+M | 4 | uint32 | LE | capabilities | Битовая маска. |

**Validation (example dump):** Реальный дамп с устройства (hex, по строкам для читаемости):

```
01 01 00 bc 23 6f 75 dc 3c 00 00 9f de 01
12 6f 6f 74 62 2d 37 34 2d 6d 31 2d 72 75 6e 74 69 6d 65
04 45 32 32 30
b1 01 00 00 00 00 00 ff 00 00 01 00 01 00 02 00 00 00 00 00 00 00 00
```

Расшифровка: format_ver=1, ble_schema_ver=1, radio_proto_ver=0, node_id=0x003CDC756F23BC, short_id=0xDE9F, device_type=1, firmware_version="ootb-74-m1-runtime" (len=18), radio_module_model="E220" (len=4), band_id=433 (0x01B1 LE), power_min=0, power_max=0, channel_min=0, channel_max=255, network_mode=0, channel_id=1, public_channel_id=1, private_channel_min=2, private_channel_max=0, capabilities=0. Раскладка совпадает с прошивкой и парсером приложения.

**Seed note:** BLE в первой итерации **read-only**; поля network_mode, channel_id и диапазоны — **информационные** (приложение только отображает текущее состояние; смена режима/канала через BLE не входит в seed).

**Time fields (seed):** Любые поля времени в DeviceInfo/Health выражаются как **uptime** (секунды/мс с включения) или **age_s** (возраст в секундах). Wall-clock (реальное время) в seed не передаётся — один консистентный timebase, без смешивания с GNSS/телефоном.

---

### 1.2. Health characteristic

- **Properties:** READ + NOTIFY
- **Notify behavior (seed):** уведомления только при **важных событиях**, без периодического спама (например: смена gnss_state, значимое изменение battery, порог pos_age; точные триггеры — TBD).

**Payload rules:** Первое поле в payload — **format_ver** (u8). Приложение/клиенты должны **игнорировать неизвестные хвостовые байты** для forward compatibility (или TLV в будущем); seed читает только известные поля.

**Fields (order in payload):**

| Поле | Тип | Описание |
|------|-----|----------|
| **format_ver** | u8 | **Первое поле.** Версия формата payload этой характеристики; при неизвестном значении клиент читает только известные поля или отбрасывает хвост. |
| **uptime_s** | uint32 | Монотонный uptime: время работы с включения, секунды (источник — тот же Timebase now_ms / 1000). |
| **gnss_state** | enum | NO_FIX / FIX_2D / FIX_3D; см. [gnss_v0](../firmware/gnss_v0.md). |
| **pos_valid** | bool | Есть ли валидная позиция для beacon. |
| **pos_age_s** | uint16 | Возраст последней валидной позиции, секунды. |
| **battery_mv** | uint16 | Напряжение батареи, мВ (или единицы — TBD). |
| **radio_tx_count** | uint32 | Счётчик отправленных пакетов (beacon и т.п.). |
| **radio_rx_count** | uint32 | Счётчик принятых пакетов. |
| **last_rx_rssi** | int8 | RSSI последнего принятого пакета, дБм. |
| **last_error_code** | uint16 (optional) | **FUTURE/TBD.** Код последней ошибки для диагностики. |

---

### 1.3. NodeTableSnapshot characteristic

- **UUID:** `6e4f0003-1b9a-4c3a-9a3b-000000000001`
- **Properties:** READ + WRITE (seed: no NOTIFY).
- **Purpose:** приложение получает снапшот NodeTable **страницами** по **10 записей** (seed: константа page_size = 10). Seed: **best effort** — страницы могут отражать слегка разные моменты; допустимо.

**Request protocol (app side):**

- Приложение **пишет** запрос (4 байта) в характеристику и затем **читает** ответ.
- **Request payload (LE):** `snapshot_id` u16 + `page_index` u16.
- Пример первого запроса: `snapshot_id=0, page_index=0` → `00 00 00 00`.
- Приложение читает страницу 0, берёт `snapshot_id` из ответа и затем запрашивает страницы `1..N` тем же `snapshot_id`.
- Если `snapshot_id` в ответе изменился или не совпадает с запросом — **перезапуск** чтения с `snapshot_id=0, page_index=0`.

**Response fields (seed, per page):**

| Поле | Тип | Размер (байт) | Описание |
|------|-----|----------------|----------|
| **snapshot_id** | uint16 | 2 | Идентификатор снапшота; генерируется устройством; меняется при создании нового снапшота. |
| **total_nodes** | uint16 | 2 | Общее число записей в NodeTable на момент снапшота. |
| **page_index** | uint16 | 2 | Индекс страницы (0-based). |
| **page_size** | uint8 | 1 | Размер страницы в записях; **в seed фиксирован 10**. |
| **page_count** | uint16 | 2 | Число страниц (ceil(total_nodes / page_size)). |
| **record_format_ver** | u8 | 1 | Версия формата NodeRecord (v1 = 1). |
| **records[]** | NodeRecord v1[] | переменный | Массив записей (до page_size штук). |

**NodeRecord v1 (compact binary, порядок полей и размеры):**

| Поле | Тип | Размер (байт) | Описание |
|------|-----|----------------|----------|
| **node_id** | uint64 | 8 | Уникальный идентификатор ноды. |
| **short_id** | uint16 | 2 | Display id (4 hex); см. [NodeTable v0 § 1a](../firmware/ootb_node_table_v0.md#1a-short-id-display-id) (CRC16-CCITT над node_id). |
| **flags** | uint8 | 1 | bit0 = is_self, bit1 = pos_valid, bit2 = is_grey (UI state), **bit3 = short_id_collision** (1 если в NodeTable есть коллизия short_id); остальные зарезервированы. |
| **last_seen_age_s** | uint16 | 2 | **Производное значение:** секунд с момента last_seen (вычисляется как (now_ms - last_seen_ms) / 1000 на устройстве). Не абсолютное время — именно возраст «как давно видели»; seed использует монотонный uptime, в BLE отдаём только age_s. |
| **lat_e7** | int32 | 4 | Широта × 1e7; 0 если pos_valid = 0. |
| **lon_e7** | int32 | 4 | Долгота × 1e7; 0 если pos_valid = 0. |
| **pos_age_s** | uint16 | 2 | Возраст позиции в секундах. |
| **last_rx_rssi** | int8 | 1 | RSSI последнего приёма от этой ноды, дБм. |
| **last_seq** | uint16 | 2 | Последний известный seq из beacon. |

**Размер одной записи NodeRecord v1:** 8+2+1+2+4+4+2+1+2 = **26 байт**. Порядок полей и типы фиксированы для v1.

**Notes:**

- Seed: **no NOTIFY** при изменении NodeTable; приложение обновляет снапшот по необходимости повторным чтением (новый snapshot_id).
- **Future:** NOTIFY при обновлении нод (node-updated events) — **FUTURE/TBD**; в seed не входит.
- Legacy Page0..Page3 характеристики **удалены**; используется единая request-based характеристика.

**App/UI notes (NodeRecord v1):**

- Список нод по умолчанию отображает **short_id** в виде 4 hex-символов (например «ABCD»).
- Если установлен флаг **short_id_collision** (flags bit3): показывать **«short_id-2»**, **«short_id-3»** (индекс в группе коллизий) или **«short_id*»**; предоставить опцию **«показать полный ID»** — отображение **node_id** в hex (16 символов).
- Изменения только в BLE/UI; **радио-протокол не меняется** (GEO_BEACON и т.д. по-прежнему используют только node_id).

---

### 1.4. Compatibility rules

- **Minor updates:** не удалять характеристики; не удалять и не менять порядок существующих полей.
- **New fields:** добавлять только в конец payload или в отдельную extension area (TLV или length-delimited — TBD).
- **Old apps:** читают только известные им поля; неизвестные хвостовые байты игнорируют (forward compatibility).
- **format_ver:** при неизвестном format_ver клиент может читать только поля, общие для всех известных версий, или отбрасывать характеристику; seed читает только известные поля по своей версии.

### 1.5. Seed constraint: No Config characteristic

- В **seed** отдельная характеристика **Config** (изменение Channel, Power, профилей через BLE) **не входит** в минимальный контракт. Телеметрия v0 — **DeviceInfo**, **Health**, **NodeTableSnapshot** (read-only). Смена канала/мощности/профиля в prod OOTB будет добавлена позже (отдельная характеристика или сервис).

### 1.6. UX notes (DeviceInfo: network mode & channel)

- **Public/Private toggle semantics:** В режиме **Public** канал зафиксирован на 1 (пользователь не может выбрать канал). В режиме **Private** доступен выбор канала в диапазоне 2..N (функция выбора канала в UI может появиться позже; в seed приложение **отображает текущие** network_mode и channel_id из DeviceInfo). Write/config в seed не добавляем.
