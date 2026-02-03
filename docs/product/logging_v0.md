# Logging v0 (OOTB seed)

**Purpose:** Решения по персистентному логу прошивки OOTB v0: носитель, формат записей, батчинг, извлечение. Issue [#20](https://github.com/AlexanderTsarkov/naviga-app/issues/20).

**Related:** [HAL contracts — ILog](../firmware/hal_contracts_v0.md#4-ilog), [Test Plan — logging tests](ootb_test_plan_v0.md), [gnss_v0 § Logging](../firmware/gnss_v0.md).

---

## 1. Storage (seed)

- **Носитель:** ESP32-S3 **flash** для персистентных логов (N16 ⇒ 16 MB flash доступно; под лог выделяется отдельная область).
- **Структура:** **персистентный ring buffer в flash** фиксированного размера.
- **Размер:** предлагается **512 KB**; конфигурируемо (compile-time или runtime TBD).
- **Retention target:** «последние ~30 минут» приблизительно, в зависимости от размера кольца и частоты событий (оценка по среднему размеру записи и event rate).

---

## 2. Binary log record format (seed)

Одна запись в кольце (binary, little-endian):

| Поле | Тип | Описание |
|------|-----|----------|
| **t_ms** | uint32 | Uptime в миллисекундах на момент записи. |
| **event_id** | uint16 | Идентификатор типа события (enum; см. список событий в firmware/ILog или отдельный реестр). |
| **level** | uint8 | Уровень (debug / info / warn / error; коды TBD). |
| **len** | uint8 | Длина payload в байтах (0..255). |
| **payload** | byte[len] | Опциональные данные события (node_id, seq, rssi, коды и т.д.). |

Записи подряд без разделителей; границы записей определяются по len (следующая запись начинается с t_ms после payload).

### 2.1. Event IDs (seed) — contention handling

Для обработки contention и диагностики TX в лог пишутся события (event_id из реестра). Минимальный набор по contention (см. [Radio v0 § 5](../protocols/ootb_radio_v0.md#5-contention-handling-seed), [HAL § 1.1 RadioTxResult](../firmware/hal_contracts_v0.md#11-radio-tx-api-result-normalization-issue-13)):

| event_id (пример) | Описание |
|-------------------|----------|
| **RADIO_TX_SCHEDULED** | BeaconTxTask запланировал отправку (should_send_now = true); попытка TX будет выполнена. |
| **RADIO_TX_OK** | send() вернул OK; пакет передан. |
| **RADIO_TX_BUSY_BACKOFF** | send() вернул BUSY_OR_CCA_FAIL; применён backoff, будет retry или skip. |
| **RADIO_TX_SKIP_CYCLE** | После retries канал занят; цикл beacon пропущен (no spam). |

Конкретные числовые значения event_id (enum) — в реестре firmware/ILog. **TX counters** (tx_attempts, tx_ok, tx_busy, tx_fail, tx_retries_used) логируются **периодически** (например раз в N секунд) или **по запросу** (команда dump/диагностика), чтобы можно было проверить наличие busy/backoff и отсутствие бесконечных retry. См. [HAL § 1.1 счётчики](../firmware/hal_contracts_v0.md#11-radio-tx-api-result-normalization-issue-13).

---

## 3. Batching (seed)

- **Буфер в RAM:** события сначала пишутся в RAM-буфер.
- **Flush:** сброс в flash:
  - по таймеру: каждые **5–10 секунд** (конкретное значение TBD);
  - по **критическим событиям:** reboot, panic, **GNSS fix lost** (и при необходимости GNSS fix acquired).
- Цель: снизить количество обращений к flash при высокой частоте событий; при этом критические события сохраняются сразу после flush-триггера.

---

## 4. Log retrieval

- **Seed minimum:** логи можно **выгрузить по serial (UART)** при подключении (команда или режим «dump»; протокол TBD). Устройство отдаёт содержимое кольца от старой к новой записи (или в обратном порядке — определить в реализации).
- **Future:** опционально **BLE LogSnapshot** с постраничным чтением по аналогии с NodeTableSnapshot (TBD; не критично для Sprint 1, если не запрошено отдельно).

См. контракт [ILog](../firmware/hal_contracts_v0.md#4-ilog): getRingBuffer(), exportToUart / exportToBle (TBD).
