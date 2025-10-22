
# Meshtastic — чтение **геопозиции (Position)** и **девайс‑метрик (Telemetry)** из Flutter по BLE + Protobuf

> Этот документ для Cursor AI: пошаговый план, ссылки на **официальные источники**, и готовые шаблоны кода, чтобы получать **Position log** и **Device metrics log** от Meshtastic‑нод по Bluetooth LE (Android/iOS).

---

## 0) Ключевые ссылки (официальные и первоисточники)

- **Client API (Serial/TCP/BLE)** — BLE‑сервис, характеристики, MTU, хэндшейк `startConfig/want_config_id`, чтение `FromRadio` «до пустого», подписка на `FromNum`  
  https://meshtastic.org/docs/development/device/client-api/

- **Protobuf схемы (BSR/GitHub)**  
  Buf Schema Registry (обзор): https://buf.build/meshtastic/protobufs  
  `mesh.proto` (RAW на BSR): https://buf.build/meshtastic/protobufs/raw/master/-/meshtastic/mesh.proto  
  `telemetry.proto` (описание типов в Python API, как «источник правды»): https://python.meshtastic.org/protobuf/telemetry_pb2.html  
  Репозиторий схем (GitHub): https://github.com/meshtastic/protobufs

- **Порты (PortNum)** — номера приложений внутри Mesh (Position/Telemetry и др.)  
  Официальная справка: https://meshtastic.org/docs/development/firmware/portnum/  
  Полная таблица (docs.rs, со значениями): https://docs.rs/meshtastic_protobufs/latest/meshtastic_protobufs/meshtastic/enum.PortNum.html

- **CLI и Telemetry** (для верификации поведения «request/response»)  
  Python CLI (гид): https://meshtastic.org/docs/software/python/cli/  
  Issue с пояснением про `want_response`: https://github.com/meshtastic/firmware/issues/4120  
  Пример `request-telemetry` в python‑экосистеме: https://github.com/pdxlocations/Meshtastic-Python-Examples

- **Настройки позиционирования/телеметрии** (интервалы и типы метрик)  
  Position Config: https://meshtastic.org/docs/configuration/radio/position/  
  Telemetry Module: https://meshtastic.org/docs/configuration/module/telemetry/

---

## 1) Что такое *Position log* и *Device metrics log* в приложении

В официальных клиентах это **журналы на стороне приложения**. Устройство **не** отдаёт «исторический лог» по запросу; вы **подписываетесь** на входящие пакеты и складываете их в локальную БД.  
Важно: BLE‑очередь к телефону хранит не все типы сообщений — **только последние** для `Position`/`User`, все `Data` сохраняются, остальное может фильтроваться. См. секцию *Queue management* в Client API.  
— Client API: https://meshtastic.org/docs/development/device/client-api/

**Вывод:** «Position log» и «Device metrics log» строятся из **стриминга** + (опционально) **активных запросов**.

---

## 2) Транспорт BLE (что слушать и куда писать)

**GATT сервис:** `6ba1b218-15a8-461f-9fa8-5dcae273eafd`  
**Характеристики:**
- **FromRadio** (READ): `2c55e69e-4993-11ed-b878-0242ac120002` — читать «пока не пусто»
- **ToRadio** (WRITE): `f75c76d2-129e-4dad-a1dd-7866124401e7` — писать `ToRadio` protobuf
- **FromNum** (READ/NOTIFY/WRITE): `ed9da18c-a800-4f66-a670-aa7547e34453` — уведомляет, что в `FromRadio` появились данные
- Рекомендуемый **MTU = 512** (Android вызвать `setMtu(512)` сразу после коннекта)

Источник и подробности (включая логи/OTA‑сервис):  
https://meshtastic.org/docs/development/device/client-api/

**Инициализация (хэндшейк):**
1. Коннект → MTU 512.  
2. Записать в **ToRadio** protobuf с `startConfig`/`want_config_id` (см. «Protobuf API» раздел).  
3. Читать **FromRadio** **повторно** до пустого ответа — получить NodeDB, конфигурацию, и `endConfig`.  
4. Подписаться на **FromNum** и при каждом notify дочитывать **FromRadio** до пусто.  
https://meshtastic.org/docs/development/device/client-api/

---

## 3) Структуры данных (Protobuf)

### 3.1 Position
Сообщение `Position` лежит в `mesh.proto`. Ключевые поля:  
- `latitude_i`, `longitude_i` — **целые** градусы × `1e‑7` (делим на 1e7 для float)  
- `altitude` (м над MSL), `time` (unix‑сек), `ground_speed` (м/с), `ground_track` (1/100 градуса), `precision_bits` и др.  
RAW (BSR): https://buf.build/meshtastic/protobufs/raw/master/-/meshtastic/mesh.proto

### 3.2 Telemetry
Верхнеуровневое `Telemetry` включает варианты (`variant`):  
`DeviceMetrics`, `EnvironmentMetrics`, `AirQualityMetrics`, `PowerMetrics`, `LocalStats`, `HealthMetrics`.  
Подтверждение имён и полей (генерированный python API):  
https://python.meshtastic.org/protobuf/telemetry_pb2.html

### 3.3 MeshPacket/Data и «request/response»
Полезная нагрузка в эфире:  
`MeshPacket { decoded = Data { portnum, payload, want_response } }`  
Поле `want_response` (в `Data`) — **флаг запроса ответа**. Важно: если поставить на **broadcast**, получите много ответов (каждый узел). Лучше указывать конкретного адресата.  
RAW (`Data.want_response` и комментарии): https://buf.build/meshtastic/protobufs/raw/master/-/meshtastic/mesh.proto

### 3.4 Порт‑номера (PortNum)
- `PositionApp = 3` — полезная нагрузка `Position`  
- `TelemetryApp = 67` — полезная нагрузка `Telemetry`  
Полная таблица с числами:  
https://docs.rs/meshtastic_protobufs/latest/meshtastic_protobufs/meshtastic/enum.PortNum.html  
Обзор: https://meshtastic.org/docs/development/firmware/portnum/

---

## 4) Два способа получать данные

### 4.1 Пассивно (стрим)
- Подпишитесь на **FromNum** → при notify читайте **FromRadio** **до пусто**.  
- Для каждого `FromRadio.packet.decoded = Data` смотрите `portnum`:
  - `PositionApp (3)` → декодируйте `Position` → сохраните точку в БД.  
  - `TelemetryApp (67)` → декодируйте `Telemetry.variant` → сохраняйте в нужный журнал (DeviceMetrics/…).
Client API (алгоритм чтения/очередь): https://meshtastic.org/docs/development/device/client-api/

### 4.2 Активно (запрос‑ответ)
Отправьте **unicast** пакет с `want_response = true`:
- **Запрос позиции:** `Data{ portnum=PositionApp, position=Position{} (пустой), want_response=true }` → ответом придёт `Position`.  
- **Запрос девайс‑метрик:** `Data{ portnum=TelemetryApp, telemetry=Telemetry{ variant=DeviceMetrics{} }, want_response=true }` → ответом `Telemetry.DeviceMetrics`.  
Поведение подтверждено в прошивке/обсуждениях:  
https://github.com/meshtastic/firmware/issues/4120

> Примечание: если вместо unicast послать **broadcast** с `want_response=true`, вы получите множественные ответы от всех узлов — это прямо указано в комментарии к `Data.want_response`.  
RAW комментарий: https://buf.build/meshtastic/protobufs/raw/master/-/meshtastic/mesh.proto

---


## 5) Мини‑пример (Dart, схема вызовов)

> Имена классов соответствуют вашим сгенерированным `*.pb.dart` (из `.proto`). Псевдокод — адаптируйте под ваш BLE‑слой (`flutter_reactive_ble`/`flutter_blue_plus`). Главное — класть сериализованный Protobuf **в `Data.payload`** и правильно указывать `portnum`.

**(а) Запрос девайс‑метрик у конкретной ноды**

```dart
// Отправка запроса DeviceMetrics к конкретной ноде (destNodeNum)
final telemetryReq = Telemetry()
  ..deviceMetrics = DeviceMetrics(); // пустой вариант — «дай метрики сейчас»

final data = Data()
  ..portnum = PortNum.TELEMETRY_APP
  ..payload = telemetryReq.writeToBuffer()
  ..wantResponse = true;

final pkt = MeshPacket()
  ..to = destNodeNum
  ..channel = 0 // primary
  ..decoded = data;

final out = ToRadio()..packet = pkt;
await ble.writeCharacteristicWithResponse(toRadioChar, value: out.writeToBuffer());
```

**(б) Запрос позиции у конкретной ноды**

```dart
final posReq = Data()
  ..portnum = PortNum.POSITION_APP
  ..payload = Position().writeToBuffer()  // пустой/минимальный
  ..wantResponse = true;

final posPkt = MeshPacket()
  ..to = destNodeNum
  ..channel = 0
  ..decoded = posReq;

await ble.writeCharacteristicWithResponse(toRadioChar, value: (ToRadio()..packet = posPkt).writeToBuffer());
```

**(в) Обработка входящих (читать FromRadio «до пусто» по notify FromNum)**

```dart
await ble.subscribeToCharacteristic(fromNumChar).listen((_) async {
  while (true) {
    final bytes = await ble.readCharacteristic(fromRadioChar);
    if (bytes.isEmpty) break;

    final fr = FromRadio.fromBuffer(bytes);
    if (!fr.hasPacket()) continue;

    final mesh = fr.packet;
    final d = mesh.decoded;

    switch (d.portnum) {
      case PortNum.POSITION_APP:
        final pos = Position.fromBuffer(d.payload);
        final lat = pos.latitudeI / 1e7;  // sfixed32 → градусы
        final lon = pos.longitudeI / 1e7;
        savePosition(mesh.from, lat, lon, pos.altitude, pos.time);
        break;

      case PortNum.TELEMETRY_APP:
        final tel = Telemetry.fromBuffer(d.payload);
        if (tel.hasDeviceMetrics()) {
          saveDeviceMetrics(mesh.from, tel.deviceMetrics, tel.time);
        }
        // при желании: tel.hasLocalStats(), tel.hasEnvironmentMetrics(), ...
        break;

      default:
        // другие порты/типы по мере необходимости
        break;
    }
  }
});
```


## 6) Схема локальной БД (пример) Схема локальной БД (пример)

```sql
-- Позиции
CREATE TABLE position_log (
  node_num INTEGER,
  t        INTEGER,      -- unix seconds (из Position.time или rx_time)
  lat      REAL,
  lon      REAL,
  alt_m    INTEGER,
  speed_ms REAL,
  track_deg REAL,
  precision_bits INTEGER,
  raw       BLOB,
  PRIMARY KEY (node_num, t)
);

-- Девайс‑метрики
CREATE TABLE device_metrics_log (
  node_num INTEGER,
  t        INTEGER,
  battery_level REAL,
  voltage       REAL,
  channel_util  REAL,
  air_util_tx   REAL,
  uptime_s      INTEGER,
  raw           BLOB,
  PRIMARY KEY (node_num, t)
);
```

Интервалы авто‑публикаций регулируются настройками модулей:  
Position: https://meshtastic.org/docs/configuration/radio/position/  
Telemetry: https://meshtastic.org/docs/configuration/module/telemetry/

---

## 7) Краевые случаи и отладка

- **NoResponse**: если запрос дошёл, но сервис на удалённой ноде не ответил (спящий узел, права канала и т. п.), вы получите ошибку маршрутизации `NO_RESPONSE` (см. `Routing.Error` в `mesh.proto`).  
  RAW: https://buf.build/meshtastic/protobufs/raw/master/-/meshtastic/mesh.proto

- **Очередь к телефону**: «только последний» `Position`/`User` пакет хранится в очереди BLE к телефону; вычитывайте без задержек и логируйте локально.  
  Client API → *Queue management*: https://meshtastic.org/docs/development/device/client-api/

- **MTU**: устройство работает с любым MTU, но **очень рекомендуется** запросить 512 байт сразу после коннекта (особенно на Android) — быстрее обмен.  
  https://meshtastic.org/docs/development/device/client-api/

- **Совместимость портов**: используйте **официальные** порт‑номера; для экспериментов — `PRIVATE_APP (256+)`.  
  https://meshtastic.org/docs/development/firmware/portnum/  
  https://docs.rs/meshtastic_protobufs/latest/meshtastic_protobufs/meshtastic/enum.PortNum.html

---

## 8) Генерация Dart‑классов из .proto (официальный путь)

Рекомендуем генерировать прямо из Buf Schema Registry, чтобы всегда быть синхронизированными с апстримом.

- Установить **Buf CLI**: https://buf.build/docs/cli/installation/  
- В проекте создать `buf.yaml`:
  ```yaml
  version: v1
  deps:
    - buf.build/meshtastic/protobufs
  ```
- Создать `buf.gen.yaml` с плагином Dart (например, `protocolbuffers/dart`):
  ```yaml
  version: v2
  plugins:
    - plugin: protocolbuffers/dart
      out: lib/meshtastic/proto
  ```
- Сгенерировать:
  ```bash
  buf dep update
  buf generate
  ```

Схемы: https://buf.build/meshtastic/protobufs  
`mesh.proto`: https://buf.build/meshtastic/protobufs/raw/master/-/meshtastic/mesh.proto  
Python API (для сравнения типов Telemetry): https://python.meshtastic.org/protobuf/telemetry_pb2.html

---

## 9) Чек‑лист для реализации в Flutter

- [ ] Подключение к MeshBluetoothService, MTU=512, `startConfig/want_config_id`.  
  https://meshtastic.org/docs/development/device/client-api/
- [ ] Подписка на `FromNum`, цикл «читать `FromRadio` до пусто».  
  https://meshtastic.org/docs/development/device/client-api/
- [ ] Парсер `FromRadio.packet.decoded = Data` → роутинг по `portnum`.  
  Таблица портов: https://docs.rs/meshtastic_protobufs/latest/meshtastic_protobufs/meshtastic/enum.PortNum.html
- [ ] Passive stream: сохранять `Position` и `Telemetry` в БД.  
- [ ] Active request: формировать unicast‑запросы с `want_response=true`.  
  Обсуждение/подтверждение: https://github.com/meshtastic/firmware/issues/4120
- [ ] Обработка ошибок маршрутизации (`NO_RESPONSE` и т.д.).  
  RAW `mesh.proto`: https://buf.build/meshtastic/protobufs/raw/master/-/meshtastic/mesh.proto

---

## 10) Быстрый FAQ

**В: Можно ли «запросить весь исторический Position log» с устройства?**  
О: Нет, история хранится в приложении. Устройство публикует периодические точки/метрики; BLE‑очередь к телефону хранит только последние `Position`/`User` (см. Client API).  
https://meshtastic.org/docs/development/device/client-api/

**В: Как получить «прямо сейчас» позицию/метрики удалённой ноды?**  
О: Отправьте **unicast** `Data` с соответствующим `portnum` и `want_response=true` — устройство вернёт ответ.  
https://github.com/meshtastic/firmware/issues/4120

**В: Какие именно поля есть у `DeviceMetrics`?**  
О: См. сгенерированную доку: `battery_level`, `voltage`, `channel_utilization`, `air_util_tx`, `uptime_seconds` и др.  
https://python.meshtastic.org/protobuf/telemetry_pb2.html

---

## Приложение A. Короткие сниппеты

**Dart: константы UUID** (чтобы Cursor их везде переиспользовал):

```dart
const meshService = Uuid.parse("6ba1b218-15a8-461f-9fa8-5dcae273eafd");
const fromRadio   = Uuid.parse("2c55e69e-4993-11ed-b878-0242ac120002");
const toRadio     = Uuid.parse("f75c76d2-129e-4dad-a1dd-7866124401e7");
const fromNum     = Uuid.parse("ed9da18c-a800-4f66-a670-aa7547e34453");
```

**Dart: преобразование координат**

```dart
double toDegrees(int sfixed32_1e7) => sfixed32_1e7 / 1e7;
```

Источники:  
Client API (BLE, очереди, MTU): https://meshtastic.org/docs/development/device/client-api/  
`Position` формат (×1e‑7) RAW: https://buf.build/meshtastic/protobufs/raw/master/-/meshtastic/mesh.proto  
Telemetry типы (python): https://python.meshtastic.org/protobuf/telemetry_pb2.html  
PortNum значения: https://docs.rs/meshtastic_protobufs/latest/meshtastic_protobufs/meshtastic/enum.PortNum.html

---

**Готово к использованию в Cursor**: этот файл можно класть в корень репозитория и ссылаться на него из задач/промптов для автогенерации кода.
