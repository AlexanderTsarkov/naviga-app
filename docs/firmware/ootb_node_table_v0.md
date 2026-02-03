# OOTB NodeTable v0 — спецификация

**Purpose:** Структура записи о ноде, типы/единицы, правила stale, лимиты и вытеснение. Контракт с приложением по BLE. Issue [#12](https://github.com/AlexanderTsarkov/naviga-app/issues/12).

**Related:** [OOTB plan](../product/OOTB_v0_analysis_and_plan.md), [OOTB BLE v0](../protocols/ootb_ble_v0.md).

---

## 1. Node identity

- **NodeID source:** ESP32 unique id (MAC или chip-derived), стабилен между перезагрузками. В OOTB v0 — единственный идентификатор ноды на эфире и в NodeTable.
- **NodeID wire format:** 64-bit integer. В документации и логах допускается hex-представление (например `0x0123456789ABCDEF` или 16 hex-символов).
- **Short ID (display id):** см. § 1a. Идентификатор для отображения в UI и логах; **никогда не используется как первичный ключ** в протоколе.
- **Factory reset policy:** NodeID **не меняется** при factory reset; сбрасываются только настройки и профили (в т.ч. «Back to public network»).
- **Note for future:** человеко-читаемый alias/имя возможен, но **не должен** рассылаться непрерывно по эфиру; предпочтительный подход — маппинг alias на стороне приложения (TBD). Механизмы BLE для alias не заданы в v0.

---

## 1a. Short ID (display id)

**Решения:**

- **Канонический идентификатор ноды:** **node_id** (uint64). Используется в протоколе и хранилище (NodeTable, GEO_BEACON); единственный первичный ключ.
- **Отображаемый идентификатор:** **short_id** (uint16), вычисляется как **CRC16-CCITT** над байтами node_id (8 байт, порядок — little-endian или big-endian; зафиксировать в реализации). Формат в UI — 4 hex-символа (например «ABCD»).
- **short_id** — только для удобства UI и логов; **никогда не используется как первичный ключ** в протокольной логике.

**Обработка коллизий:**

- **Обнаружение:** внутри NodeTable при наличии двух разных **node_id**, дающих один и тот же **short_id**, считать коллизию.
- **Хранение:** на уровне записи — флаг **short_id_collision** (bool) **или** вести счёт/индекс группы коллизий для UI (сколько записей с данным short_id).
- **Условные обозначения в UI:**
  - без коллизии: показывать **«ABCD»** (4 hex);
  - при коллизии: **«ABCD-2»**, **«ABCD-3»** (индекс внутри группы коллизий) **или** **«ABCD*»**;
  - опция **«показать полный ID»** — вывести node_id в hex (16 символов) по запросу пользователя.

**Замечания по реализации:**

- При необходимости избегать зарезервированных значений short_id (**0x0000**, **0xFFFF**): если CRC16-CCITT даёт такое значение, применять **детерминированное преобразование** (например, XOR с константой или подстановка); метод зафиксировать в реализации/документации, если используется.

---

## 2. Self entry

**NodeTable включает запись о себе (self):** одна запись с **is_self = true** (собственный node_id, позиция из GNSS/stub, last_seen обновляется при TX). Остальные записи — удалённые ноды (is_self = false). Self-запись никогда не вытесняется при eviction (см. § 4).

---

## 3. NodeTable record fields (seed, MUST)

Запись о ноде в NodeTable и в контракте с приложением (BLE snapshot/events). Минимальный обязательный набор для seed:

| Поле | Тип | Описание |
|------|-----|----------|
| **node_id** | uint64 | Уникальный идентификатор ноды (NodeID); см. § 1. |
| **short_id** | uint16 (derived) | **CRC16-CCITT** над байтами node_id; для UI/log (4 hex). При коллизиях — см. § 1a (short_id_collision или индекс в группе для отображения). |
| **is_self** | bool | true — запись о себе; false — удалённая нода. |
| **last_seen_ms** | uint32 или uint64 | Время последнего наблюдения: **монотонный uptime_ms** (мс с включения устройства). Хранится для каждой записи. |
| **pos_valid** | bool | Есть ли валидная позиция (от отправителя или у self — из GNSS). |
| **lat_e7** / **lon_e7** | int32 (если pos_valid) | Широта/долгота в 1e-7 градусах (микрограды). Передаются/хранятся только при pos_valid == true. |
| **pos_age_s** | uint16 | Возраст позиции в секундах (как принято в beacon или вычисленный display age). |
| **last_rx_rssi** | int8 | RSSI последнего принятого пакета от этой ноды, дБм (для remote); для self может быть N/A или 0. |
| **last_seq** | u8 или u16 | Последний известный seq из beacon; для дедупликации и порядка. |
| **short_id_collision** | bool (optional) | true, если в NodeTable есть другая запись с тем же short_id; для UI (показать «ABCD*» или индекс). См. § 1a. |

Дополнительные поля (battery, alias и т.п.) — см. § 5 Future notes и [OOTB plan](../product/OOTB_v0_analysis_and_plan.md).

---

## 4. UI state policy (seed: two-state) & capacity / eviction

### 4.1. UI state (NORMAL vs GREY)

- **Состояния (seed):** **NORMAL** — нода «живая»; **GREY** — нода недавно не наблюдалась (stale).
- **expected_interval_s** = глобальный **max_silence_s** из настроек radio cadence (seed); см. [ootb_radio_v0.md § 4 Beacon cadence](../protocols/ootb_radio_v0.md#4-beacon-cadence-seed) (max_silence = min_update_interval_s × max_silence_multiplier).
- **grace_s** = max(2, round(expected_interval_s × 0.25)).
- **GREY** если: `(now_ms - last_seen_ms) > (expected_interval_s + grace_s) × 1000` (now_ms и last_seen_ms — монотонный uptime в мс; порог в секундах переводится в мс для сравнения).

Правило проверяемо без кода: по last_seen_ms и now_ms вычислить возраст в с: age_s = (now_ms - last_seen_ms) / 1000; сравнить age_s с expected_interval_s + grace_s.

### 4.2. Capacity & eviction

- **max_nodes** = 100 (константа конфига для seed).
- **Когда таблица полная:** вытеснять сначала **самые старые GREY** записи (LRU по last_seen_ms); **self (is_self = true) никогда не вытеснять**. Если свободных GREY нет — политика TBD (отказать в добавлении новой ноды или вытеснить самую старую non-self; для seed допустимо «отказать в добавлении»).

Правило проверяемо: при достижении max_nodes и появлении новой ноды — удаляется запись с is_self=false, состоянием GREY и минимальным last_seen_ms.

**Экспорт в BLE/приложение (seed):** при отдаче записей в NodeTableSnapshot предпочтительно передавать **last_seen_age_s** (возраст в секундах: (now_ms - last_seen_ms) / 1000), а не сырой last_seen_ms — один консистентный timebase, приложение не зависит от uptime устройства.

---

## 5. Future notes

- **Per-node expected_interval:** планируется как расширение радио (remote_max_silence_s и т.п.); в core seed не входит — используется один глобальный expected_interval_s из cadence settings.
- **Alias/name:** свой alias (self) может храниться на устройстве; alias удалённых нод в seed обрабатываются на стороне приложения (app-local mapping). Будущая синхронизация alias по эфиру (control messages) — вне scope seed.
