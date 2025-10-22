
# Meshtastic (T‑Beam и др.) из Flutter через BLE + Protobuf — **инструкция для Cursor**

> Цель: дать **чёткий план действий и готовые фрагменты кода**, чтобы приложение на Flutter для Android/iOS напрямую общалось с Meshtastic‑устройством по **BLE** и обменивалось сообщениями через **Protobuf** (без gRPC).

---

## TL;DR (шпаргалка для Cursor)

1) **Сгенерируй Dart‑классы из официальных .proto Meshtastic** (через Buf + `protocolbuffers/dart`) в `lib/meshtastic/proto`.  
2) **BLE:** подключись к сервису `6ba1b218-15a8-461f-9fa8-5dcae273eafd`, пиши в `ToRadio`, читай из `FromRadio`, слушай нотификации `FromNum`.  
3) Сразу после коннекта **увеличь MTU до 512** (Android), затем **отправь `ToRadio.want_config_id`** (nonce произвольный), и **читай `FromRadio` пока буфер пуст** — это скачает NodeDB/конфиг.  
4) Для отправки текста: собери `Data{ portnum=TEXT_MESSAGE_APP, payload=UTF8(text) }` → оберни в `MeshPacket{ to=broadcast }` → вложи в `ToRadio.packet` → **write** в `ToRadio`.  
5) На входящие: по нотификации `FromNum` вызови «помпу» чтения `FromRadio` до пустого ответа, парси `FromRadio` и обрабатывай `packet.decoded` по `portnum`.

— UUID и полный BLE‑протокол описаны в официальной «Client API». citeturn9view0  
— Структуры `ToRadio`/`FromRadio`/`MeshPacket`/`Data` — в `mesh.proto`. citeturn13view1turn15view0  
— Готовая генерация **Dart SDK** через Buf (`protocolbuffers/dart`) есть на BSR. citeturn3search0

---

## 1) Подготовка проекта (Flutter)

### `pubspec.yaml`

```yaml
environment:
  sdk: ">=3.3.0 <4.0.0"

dependencies:
  flutter:
    sdk: flutter
  flutter_reactive_ble: ^5.4.0        # BLE-клиент (Android/iOS)
  protobuf: ^3.1.0                    # рантайм для .pb.dart

dev_dependencies:
  flutter_test:
    sdk: flutter
```

> `flutter_reactive_ble` поддерживает **сканирование, соединение, чтение/запись, нотификации и MTU negotiation**. citeturn17search0

### Права и декларации

**Android (`android/app/src/main/AndroidManifest.xml`):**
```xml
<!-- Android 12+ -->
<uses-permission android:name="android.permission.BLUETOOTH_SCAN"
  android:usesPermissionFlags="neverForLocation"/>
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT"/>
<!-- До Android 12: -->
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" android:maxSdkVersion="30"/>
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" android:maxSdkVersion="30"/>
```

**iOS (`ios/Runner/Info.plist`):**
```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>Нужно для связи с Meshtastic по BLE</string>
<key>NSBluetoothPeripheralUsageDescription</key>
<string>Нужно для связи с Meshtastic по BLE</string>
```

---

## 2) Генерация Dart‑классов из официальных Protobuf

Официальные схемы опубликованы в **Buf Schema Registry** (`meshtastic/protobufs`) и снабжены **готовыми SDK‑плагинами**, включая Dart (`protocolbuffers/dart`). citeturn3search1turn3search0

### Вариант A (рекомендуется): Buf + удалённый плагин `protocolbuffers/dart`

1. Установи [Buf CLI] и добавь конфиг зависимости:
   **`buf.yaml`**
   ```yaml
   version: v1
   deps:
     - buf.build/meshtastic/protobufs
   ```

2. Создай **`buf.gen.yaml`** (v2) с Dart‑плагином:
   ```yaml
   version: v2
   plugins:
     - plugin: protocolbuffers/dart
       out: lib/meshtastic/proto
   ```
   > Buf сгенерирует `.pb.dart` в указанный каталог. citeturn12search1turn12search13

3. Обнови lock‑файл зависимостей модуля и сгенерируй код:
   ```bash
   buf dep update
   buf generate
   ```
   > `buf dep update` фиксирует точные версии зависимостей в `buf.lock`, `buf generate` запускает кодогенерацию по `buf.gen.yaml`. citeturn12search7turn12search1

### Вариант B: локальный `protoc` + `protoc_plugin` (Dart)

1. Установи `protoc` и плагин:
   ```bash
   dart pub global activate protoc_plugin
   ```
   > Плагин `protoc-gen-dart` генерирует `.pb.dart`. citeturn10search0turn10search7

2. Выгрузи `.proto` локально и сгенерируй:
   ```bash
   # Скопировать протосы из BSR в ./third_party/meshtastic
   buf export buf.build/meshtastic/protobufs -o third_party/meshtastic

   # Сгенерировать Dart-код
   protoc \
     -I third_party \
     --dart_out=lib/meshtastic/proto \
     third_party/meshtastic/meshtastic/*.proto
   ```
   > `buf export` копирует исходные `.proto` из BSR. citeturn12search0

После генерации у тебя будут, в частности:  
`lib/meshtastic/proto/mesh.pb.dart`, `portnums.pb.dart`, `config.pb.dart` и т.п.

---

## 3) Что важно знать про BLE‑протокол Meshtastic

**Сервис (GATT) и характеристики:**  
- **Service UUID:** `6ba1b218-15a8-461f-9fa8-5dcae273eafd`  
- **FromRadio (READ):** `2c55e69e-4993-11ed-b878-0242ac120002`  
- **ToRadio (WRITE):** `f75c76d2-129e-4dad-a1dd-7866124401e7`  
- **FromNum (READ/NOTIFY/WRITE):** `ed9da18c-a800-4f66-a670-aa7547e34453`  
- Дополнительно (лог‑нотификации): `5a3d6e49-06e6-4423-9944-e9de8cdf9547`  
- Рекомендуется **MTU=512** (ускоряет обмен; устройство работает и с меньшим MTU). citeturn9view0

**Инициализация соединения (handshake):**  
1) Соединись по BLE и **установи MTU ~512** (на iOS это автоматом, на Android — запросом).  
2) Отправь `ToRadio.want_config_id = <nonce>` в `ToRadio`.  
3) Читай `FromRadio` **повторно** до пустого ответа — устройство выдаст `MyNodeInfo`, `Config`, `NodeInfo*…` и закончит `FromRadio.config_complete_id = <nonce>`.  
4) Далее подпишись на нотификации `FromNum` и при каждой нотификации **вычитывай `FromRadio` до пустого ответа**. citeturn9view0turn13view1

**Модель сообщений (по Protobuf):**  
- В сторону устройства пишем `ToRadio` → либо `want_config_id`, либо `packet` (где `packet` — это `MeshPacket`).  
- От устройства читаем `FromRadio` → может прийти `packet` (обёртка над `MeshPacket`), `my_info`, `config`, `node_info`, `config_complete_id` и др. citeturn13view1turn14view0

**Отправка пользовательских данных:**  
- Полезная нагрузка — это `MeshPacket.decoded = Data{ portnum, payload, want_response? }`.  
- Для **широковещательных** сообщений в поле назначения используют **broadcast‑адрес** (в прошивке зарезервирован `0xFFFFFFFF`). citeturn15view0turn4search1

> Про **диапазоны портов** (`PortNum`) и регистрацию собственных портов см. «Port Numbers» в protobuf/Docs. Используй `PortNum.PRIVATE_APP` (256+) для экспериментов или `TEXT_MESSAGE_APP` для простых текстов. citeturn5search1turn5search10

---

## 4) Код на Flutter (Android/iOS)

### Константы BLE UUID

```dart
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

final serviceId  = Uuid.parse("6ba1b218-15a8-461f-9fa8-5dcae273eafd");
final fromRadio  = Uuid.parse("2c55e69e-4993-11ed-b878-0242ac120002");
final toRadio    = Uuid.parse("f75c76d2-129e-4dad-a1dd-7866124401e7");
final fromNum    = Uuid.parse("ed9da18c-a800-4f66-a670-aa7547e34453");
final logRecord  = Uuid.parse("5a3d6e49-06e6-4423-9944-e9de8cdf9547"); // optional
```

### Сканирование и подключение

```dart
final ble = FlutterReactiveBle();

Stream<DiscoveredDevice> scanForMeshtastic() {
  return ble.scanForDevices(withServices: [serviceId]).where(
    (d) => d.serviceUuids.contains(serviceId) || (d.name.toLowerCase().contains('meshtastic')),
  );
}

Future<String> connect(String deviceId) async {
  // Быстрая схема: предварительно сканируем и коннектимся к конкретному сервису
  await for (final _ in ble.connectToAdvertisingDevice(
    id: deviceId,
    withServices: [serviceId],
    prescanDuration: const Duration(seconds: 3),
    servicesWithCharacteristicsToDiscover: {serviceId: [fromRadio, toRadio, fromNum]},
    connectionTimeout: const Duration(seconds: 10),
  )) {
    // дождёмся состояния connected — затем выходим из цикла
    break;
  }
  return deviceId;
}
```

> `connectToAdvertisingDevice` помогает стабильнее коннектиться к BLE‑устройствам на Android. citeturn17search7

### MTU и подготовка характеристик

```dart
QualifiedCharacteristic qFromRadio(String id) =>
  QualifiedCharacteristic(serviceId: serviceId, characteristicId: fromRadio, deviceId: id);
QualifiedCharacteristic qToRadio(String id) =>
  QualifiedCharacteristic(serviceId: serviceId, characteristicId: toRadio, deviceId: id);
QualifiedCharacteristic qFromNum(String id) =>
  QualifiedCharacteristic(serviceId: serviceId, characteristicId: fromNum, deviceId: id);

Future<void> tuneMtuAndroid(String deviceId) async {
  try {
    // На iOS MTU регулируется автоматически; метод просто вернётся
    await ble.requestMtu(deviceId: deviceId, mtu: 512);
  } catch (_) {/* игнор */}
}
```

### Handshake: `want_config_id` + «помпа» чтения FromRadio

```dart
import 'dart:typed_data';
import 'package:meshtastic_proto/meshtastic/mesh.pb.dart' as mesh;

Future<void> startSession(String deviceId) async {
  await tuneMtuAndroid(deviceId);

  // 1) отправляем want_config_id
  final want = mesh.ToRadio()..wantConfigId = DateTime.now().millisecondsSinceEpoch & 0x7fffffff;
  await ble.writeCharacteristicWithResponse(qToRadio(deviceId), value: want.writeToBuffer());

  // 2) читаем последовательность FromRadio до пустого ответа
  await _drainFromRadio(deviceId);

  // 3) подписываемся на FromNum и при каждом уведомлении дочитываем FromRadio
  ble.subscribeToCharacteristic(qFromNum(deviceId)).listen((_) async {
    await _drainFromRadio(deviceId);
  });
}

Future<void> _drainFromRadio(String deviceId) async {
  while (true) {
    final data = await ble.readCharacteristic(qFromRadio(deviceId));
    if (data.isEmpty) break;
    final fr = mesh.FromRadio.fromBuffer(Uint8List.fromList(data));
    _handleFromRadio(fr);
  }
}

void _handleFromRadio(mesh.FromRadio fr) {
  if (fr.hasPacket()) {
    final pkt = fr.packet;
    if (pkt.hasDecoded()) {
      final d = pkt.decoded;
      // разбор по d.portnum, например текстовые сообщения и т.п.
    }
  }
  if (fr.hasConfigCompleteId()) {
    // проверяй соответствие nonce, если хранишь его
  }
  // ... обработка my_info, config, node_info и пр.
}
```

> Поведение «читать до пустого» и последовательность начальной выгрузки описаны в Client API. Поля `want_config_id` / `config_complete_id` — в `mesh.proto`. citeturn9view0turn13view1

### Отправка широковещательного текстового сообщения

```dart
import 'dart:convert';
import 'dart:typed_data';
import 'package:meshtastic_proto/meshtastic/mesh.pb.dart' as mesh;
import 'package:meshtastic_proto/meshtastic/portnums.pb.dart' as ports;

const int BROADCAST_ADDR = 0xFFFFFFFF; // спец. адрес широковещания на прошивке

Future<void> sendBroadcastText(String deviceId, String text) async {
  final data = mesh.Data()
    ..portnum = ports.PortNum.TEXT_MESSAGE_APP
    ..payload = Uint8List.fromList(utf8.encode(text));

  final packet = mesh.MeshPacket()
    ..to = BROADCAST_ADDR
    ..decoded = data
    ..wantAck = false;

  final toRadio = mesh.ToRadio()..packet = packet;
  await ble.writeCharacteristicWithResponse(qToRadio(deviceId), value: toRadio.writeToBuffer());
}
```

> В protobuf сообщение пользователя — это `MeshPacket{ decoded = Data{ portnum, payload } }`. Для широковещания используется спец‑адрес `0xFFFFFFFF`. citeturn15view0turn4search1

---

## 5) Полезные порт‑номера и расширение протокола

- Диапазоны портов (`PortNum`):  
  `0–63` — ядро Meshtastic; `64–127` — зарегистрированные 3rd‑party; `256–511` — приватные приложения.  
  Для собственных фич на раннем этапе используй `PRIVATE_APP` (256). Позже — PR в `portnums.proto`. citeturn5search1turn5search10

- Для простого чата используй `TEXT_MESSAGE_APP`. ( enum доступен в `portnums.pb.dart`, числа руками не жёстко кодировать.)

---

## 6) Отладка и логи

- При желании подпишись на `logRecord` (см. UUID выше) — устройство шлёт `LogRecord` protobuf в нотификациях. citeturn9view0  
- Для повторяемых тестов есть Python‑CLI/библиотека (`meshtastic/python`), где по умолчанию используется `BROADCAST_ADDR`. Полезно сверять формат пакетов. citeturn7search5turn4search9

---

## 7) Частые вопросы и тонкости

- **MTU на iOS** напрямую не управляется — CoreBluetooth подбирает автоматически; библиотека Flutter просто работает с тем, что есть. Рекомендация Meshtastic «попросить 512» релевантна для Android. citeturn9view0  
- **Фрагментация:** Meshtastic укладывает отдельный protobuf‑пакет в значение характеристики. Держи полезные сообщения компактными (обычно <200 байт). Если сообщение крупное, смотри `XModem`/chunked‑payload в `mesh.proto`. citeturn7search12  
- **Порядок чтения:** всегда «читай до пустого», когда пришла нотификация `FromNum`, иначе можно потерять сообщения из очереди устройства. citeturn9view0

---

## 8) Мини‑чеклист для PR/CI

- [ ] Генерация `.pb.dart` повторяется локально командой `buf generate` и кэшируется в репозитории (или в артефактах CI). citeturn12search1  
- [ ] UUID/характеристики вынесены в константы. citeturn9view0  
- [ ] Есть интеграционный тест «подключение → want_config_id → чтение config_complete_id». citeturn13view1  
- [ ] Линтер запрещает использовать «магические числа» портов; enum из `portnums.pb.dart` обязателен. citeturn5search1

---

## 9) Ссылки и первоисточники

- **Client API (официально, BLE UUID, handshake, MTU)** — Meshtastic Docs. citeturn9view0  
- **`mesh.proto` (ToRadio/FromRadio/Data/MeshPacket и др.)** — Buf raw. citeturn13view1turn15view0  
- **BSR: сгенерированные SDK и плагины (Dart)** — Buf BSR. citeturn3search0  
- **Buf export / generate** — Buf Docs. citeturn12search0turn12search1  
- **Python lib (для сверки поведения, BROADCAST_ADDR)** — Meshtastic Python Docs. citeturn7search5turn4search9

---

### В контексте проекта «Навига»
Эта интеграция вписывается в оффлайн‑связь и обмен данными между устройствами в полевых условиях (см. видение проекта). fileciteturn0file0

