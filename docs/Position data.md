## **Где смотреть в исходниках (Android и Flutter)**

### **1\) Официальное Android‑приложение**

* **RadioInterfaceService.kt** — сервис, который держит BLE‑линк и гоняет `ToRadio/FromRadio` по протоколу. Служит «транспортом», а не местом бизнес‑логики. Ссылка на файл и автосгенерированную KDoc:  
   • `RadioInterfaceService.kt` (GitHub) [GitHub](https://github.com/meshtastic/Meshtastic-Android/blob/master/app/src/main/java/com/geeksville/mesh/repository/radio/RadioInterfaceService.kt)  
   • Док страница пакета `com.geeksville.mesh.repository.radio` (KDoc) [Meshtastic](https://meshtastic.github.io/Meshtastic-Android/-meshtastic%20-app/com.geeksville.mesh.repository.radio/index.html)

* Поведение обмена — **одинаковое для BLE, USB и TCP** и описано в **Client API**:

  1. после соединения приложение шлёт `ToRadio{ want_config_id = N }`;

  2. далее **подписывается на BLE‑нотификацию `FromNum`**, и **на каждую нотификацию** читает **все** пакеты из `FromRadio` **пока не вернётся пусто**;

  3. для отправки — пишет один `ToRadio` в характеристику `ToRadio`.  
      Описание на сайте и в старом dev‑доке к BLE: Client API и «Bluetooth API» (тот же паттерн чтения/записи/нотификатора). [meshtastic.org+2GitHub+2](https://meshtastic.org/docs/development/device/client-api/?utm_source=chatgpt.com)

**Важно:** геопозиция приходит обычным `MeshPacket.decoded = Data{ portnum = POSITION_APP, payload = <serialized Position> }` — то есть **позиция лежит в `Data.payload`** (байты protobuf), а не в отдельных полях `Data`. Схемы сообщений тут: `mesh.proto` и таблица портов (`PortNum.PositionApp = 3`). [GitHub+1](https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto?utm_source=chatgpt.com)

### **2\) MultiMesh (Flutter‑клиент)**

* Репозиторий: `paulocode/multimesh`. Там логика разделена на **читателя** (подписка на BLE, разбор `FromRadio`) и **писателя** (очередь пакетов `ToRadio`). [GitHub](https://github.com/paulocode/multimesh)

* По отправке видно из issue, что есть сервис‑пакет **`lib/services/queued_radio_writer.dart`**, где собирают `MeshPacket` (hopLimit/priority и т. п.) и шлют в радио. Это ровно то место, куда добавляется отправка запроса позиции. [GitHub](https://github.com/paulocode/multimesh/issues/17)

---

## **Минимальный «Dart‑эквивалент» (как в Android и MultiMesh)**

Ниже — **самодостаточные фрагменты**. Они опираются на **сгенерённые Dart‑protobuf классы** из `meshtastic/protobufs` (тот же набор, что на BSR/Buf). Поля и enum‑ы соответствуют `mesh.proto`. [GitHub+1](https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto?utm_source=chatgpt.com)

Примечание по BLE‑транспорту: в примерах используются абстракции `ble.write(ToRadioUUID, bytes)`, `ble.read(FromRadioUUID)` и подписка на `FromNum`. У вас они уже есть — вы писали, что «общая информация о нодах» приходит.

### **A. Подписка на FromNum и «чтение до пусто» из FromRadio**

`// Псевдо‑интерфейс к BLE-харам`  
`abstract class MeshBle {`  
  `Stream<List<int>> onFromNumNotify();              // notify`  
  `Future<List<int>?> readFromRadio();               // read; вернёт null или [] когда "пусто"`  
  `Future<void> writeToRadio(List<int> bytes);       // write ToRadio`  
`}`

`// Протобуф-классы (сгенерированы из mesh.proto)`  
`import 'meshtastic/mesh.pb.dart';   // MeshPacket, Data, Position, ToRadio, FromRadio, PortNum`

`void startMeshReader(MeshBle ble, void Function(int nodeId, Position pos) onPosition) {`  
  `// 1) Подписываемся на FromNum (сигнал "в буфере есть пакеты")`  
  `ble.onFromNumNotify().listen((_) async {`  
    `// 2) Читаем FromRadio до пустого ответа`  
    `while (true) {`  
      `final bytes = await ble.readFromRadio();`  
      `if (bytes == null || bytes.isEmpty) break;`

      `final fr = FromRadio()..mergeFromBuffer(bytes);`

      `// Вариант 1: пришёл пакет`  
      `if (fr.hasPacket()) {`  
        `final pkt = fr.packet;`

        `if (pkt.hasDecoded()) {`  
          `final d = pkt.decoded;`

          `// --- КЛЮЧЕВОЕ МЕСТО: геопозиция приходит как payload для POSITION_APP ---`  
          `if (d.portnum == PortNum.PositionApp && d.hasPayload()) {`  
            `final pos = Position()..mergeFromBuffer(d.payload);`  
            `onPosition(pkt.from, pos); // 'from' — numeric ID отправителя`  
          `}`  
        `}`  
      `}`

      `// Вариант 2: ответы на конфиг/ноды и т.п. — обрабатывайте по необходимости`  
      `// (MyNodeInfo, node_infos, config_complete и т.д.)`  
    `}`  
  `});`  
`}`

**Почему именно так:** это ровно тот же «паттерн» чтения, что описан в Client API/«Bluetooth API» и который реализован в `RadioInterfaceService` в Android. [meshtastic.org+1](https://meshtastic.org/docs/development/device/client-api/?utm_source=chatgpt.com)

### **B. Явный запрос позиции у конкретного узла (DM с want\_response)**

Android‑приложение при долгом тапе по ноде умеет «Request position». Технически это **однонаправленный DM** на `dest` с `Data{ portnum = POSITION_APP, want_response = true }` и пустым `Position` в `payload`. Узел отвечает своей текущей позицией тем же портом. (Поведение подтверждается обсуждениями и практикой клиентов.) [GitHub+1](https://github.com/meshtastic/Meshtastic-Android/issues/813?utm_source=chatgpt.com)

`Future<void> requestNodePosition(MeshBle ble, int destNodeNum) async {`  
  `// Пустой Position как «запрос»`  
  `final queryPos = Position(); // без полей`

  `final data = Data()`  
    `..portnum = PortNum.PositionApp`  
    `..wantResponse = true`  
    `..payload = queryPos.writeToBuffer();`

  `final mesh = MeshPacket()`  
    `..from = 0 // заполняется прошивкой, можно опустить`  
    `..to = destNodeNum`  
    `..decoded = data`  
    `..hopLimit = 3; // или из вашей конфигурации сети`

  `final tr = ToRadio()..packet = mesh;`

  `await ble.writeToRadio(tr.writeToBuffer());`  
`}`

Если узел отвечает, вы поймаете **входящий** `MeshPacket.decoded{ portnum = POSITION_APP, payload = <Position> }` в обработчике из блока **A** (см. `onPosition`).

**Замечание:** на старте сессии **обязательно** отправляйте «хочу полный конфиг/ноды» через `ToRadio{ want_config_id = ... }`, чтобы получить `node_infos`, `MyNodeInfo` и т. п. — так вы подхватите «последнюю известную позицию» из нодовой БД, если она есть. Это канонический первый пакет клиента. [Docs.rs](https://docs.rs/meshtastic_protobufs/latest/meshtastic_protobufs/meshtastic/to_radio/enum.PayloadVariant.html?utm_source=chatgpt.com)

---

## **Частые причины «позиции нет», хотя «должна быть»**

1. **Нода физически не умеет GPS** (напр. Heltec V3 без GPS) и не включено «Share phone location». В таком случае **никакой «последней позиции» в ноде может не быть**. Проверьте **Position Config** и «Provide phone location to mesh». [meshtastic.org+1](https://meshtastic.org/docs/configuration/radio/position/?utm_source=chatgpt.com)

2. **Вы читаете не то поле.** Позиция **всегда** приходит как `Data.payload` (protobuf `Position`), а не как `Data.position`. Это самая частая ошибка при первой интеграции. Схема `mesh.proto`/`PortNum` подтверждает. [GitHub+1](https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto?utm_source=chatgpt.com)

3. **Нет «снимка» при подключении.** Без `want_config_id` прошивка не пришлёт вам node‑db; BLE‑буфер содержит только то, что накопилось **с момента разрыва**. Шлите `want_config_id` на старте. [Docs.rs](https://docs.rs/meshtastic_protobufs/latest/meshtastic_protobufs/meshtastic/to_radio/enum.PayloadVariant.html?utm_source=chatgpt.com)

4. **Тайминги и точность.** Позиции публикуются по настройкам Position Config (интервалы, «умные» обновления). Если устройство стоит на месте и включена «умная» подача, логи/обновления могут «молчать». [meshtastic.org](https://meshtastic.org/docs/configuration/radio/position/?utm_source=chatgpt.com)

5. **Логи в приложении ≠ история в устройстве.** «Position Log»/«Device Metrics Log» в приложении — это **журнал на стороне клиента** (и он может показывать только последние обновления, если фильтры/условия не выполняются). На iOS, например, часто видят **только последнюю позицию** именно поэтому. Не ожидайте, что прошивка вернёт «историю» по запросу. [GitHub](https://github.com/orgs/meshtastic/discussions/217?utm_source=chatgpt.com)

---

## **«Куда смотреть» в MultiMesh, если хочется найти один‑в‑один**

* Основной репозиторий: `paulocode/multimesh`. [GitHub](https://github.com/paulocode/multimesh)

* Отправитель пакетов: **`lib/services/queued_radio_writer.dart`** — видно из issue, где обсуждали приоритет/хоплимит. Добавляйте туда сборку `MeshPacket` для `POSITION_APP`. [GitHub](https://github.com/paulocode/multimesh/issues/17)

* Чтение/декодинг — файлы сервиса/ридера BLE в `lib/services/...` (структура может меняться, но паттерн тот же: notify `FromNum` → «read‑until‑empty» `FromRadio` → `FromRadio.packet.decoded`).

* Если нужен ориентир по транспорту и последовательности действий — сверяйтесь с официальным **Client API** и «Bluetooth API» (паттерн идентичен). [meshtastic.org+1](https://meshtastic.org/docs/development/device/client-api/?utm_source=chatgpt.com)

---

## **Быстрый чек‑лист для Cursor (AI)**

1. **На старте BLE‑сессии**: отправь `ToRadio{ want_config_id = <rand> }`. Жди `config_complete_id` и поток `node_infos`. [Docs.rs](https://docs.rs/meshtastic_protobufs/latest/meshtastic_protobufs/meshtastic/to_radio/enum.PayloadVariant.html?utm_source=chatgpt.com)

2. **Подпишись на FromNum**; на каждую нотификацию делай цикл чтения `FromRadio` «до пусто». [GitHub](https://github.com/zombodotcom/Meshtastic-device/blob/master/docs/software/bluetooth-api.md?utm_source=chatgpt.com)

3. **Декодируй входящие**:

   * если `FromRadio.packet.decoded.portnum == POSITION_APP` → `Position.mergeFromBuffer(decoded.payload)`. [GitHub+1](https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto?utm_source=chatgpt.com)

   * сохраняй **последнюю позицию** и при желании — **историю** в локальную БД приложения (это и есть «Position log» в UI). [GitHub](https://github.com/orgs/meshtastic/discussions/217?utm_source=chatgpt.com)

4. **Запрос позиции у ноды**: шли `ToRadio.packet{ to = nodeNum, decoded{ portnum = POSITION_APP, want_response = true, payload = <empty Position> } }`. Лови ответ как в п.3. [GitHub+1](https://github.com/meshtastic/Meshtastic-Android/issues/813?utm_source=chatgpt.com)

5. **Если пусто**: проверь Position Config/телеметрию/включён ли «Provide phone location to mesh». [meshtastic.org+1](https://meshtastic.org/docs/configuration/radio/position/?utm_source=chatgpt.com)

