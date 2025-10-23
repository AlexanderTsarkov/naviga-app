# Meshtastic Flutter API Guide

## Обзор

Этот документ описывает полную интеграцию с устройствами Meshtastic из Flutter приложений, включая BLE подключение, protobuf коммуникацию, и управление каналами.

## Архитектура

### Основные компоненты

1. **MeshtasticBluetoothService** - основной сервис для BLE коммуникации
2. **DatabaseService** - локальное хранение данных узлов
3. **Protobuf Classes** - официальные классы для сериализации/десериализации
4. **Channel Management** - управление каналами передачи данных

## BLE Подключение

### UUID сервисов и характеристик

```dart
// Meshtastic BLE Service
static const String meshServiceUuid = '6ba1b218-15a8-461f-9fa8-5dcae273eafd';

// Характеристики
static const String fromRadioUuid = '2c55e69e-4993-11ed-b878-0242ac120002';
static const String toRadioUuid = 'f75c76d2-129e-4dad-a1dd-7866124401e7';
static const String fromNumUuid = 'ed9da18c-a800-4f66-a670-aa7547e34453';
```

### Процесс подключения

1. **Подключение к устройству**
2. **Установка MTU (512 байт)**
3. **Обнаружение сервисов**
4. **Подписка на FromNum уведомления**
5. **Handshake с want_config_id**

```dart
// Handshake последовательность
final nonce = DateTime.now().millisecondsSinceEpoch;
final toRadio = ToRadio()..wantConfigId = nonce;
await _toRadio!.write(toRadio.writeToBuffer());

// Чтение ответа до пустого
await _drainFromRadioUntilEmpty();
```

## Protobuf Сообщения

### Основные типы сообщений

- **ToRadio** - отправка команд устройству
- **FromRadio** - получение данных от устройства
- **MeshPacket** - пакеты mesh сети
- **Data** - полезная нагрузка пакетов
- **Position** - GPS координаты
- **Telemetry** - метрики устройства
- **User** - информация о узле

### Port Numbers

```dart
enum PortNum {
  UNKNOWN_APP = 0,
  TEXT_MESSAGE_APP = 1,
  REMOTE_HARDWARE_APP = 2,
  POSITION_APP = 3,
  NODEINFO_APP = 4,
  ROUTING_APP = 5,
  ADMIN_APP = 2,
  TELEMETRY_APP = 67,
}
```

## Управление каналами

### Принцип работы каналов

**КРИТИЧЕСКИ ВАЖНО:** Геоданные передаются по первому каналу, на котором их передача разрешена, начиная с канала 0.

### Возможные конфигурации

1. **Канал 0 разрешен** → геоданные по каналу 0
2. **Канал 0 запрещен, канал 1 разрешен** → геоданные по каналу 1
3. **Несколько каналов разрешены** → используется первый разрешенный
4. **Все каналы запрещены** → геоданные не передаются

### Адаптивная стратегия

```dart
// Определение активного канала для геоданных
int findActivePositionChannel(ChannelConfig config) {
  for (int channel = 0; channel <= 8; channel++) {
    if (config.isPositionEnabled(channel)) {
      return channel;
    }
  }
  return 0; // fallback на primary канал
}
```

## Получение данных

### Типы данных

1. **Последняя известная позиция** - из NodeInfo при handshake
2. **Обновления позиции** - через POSITION_APP сообщения
3. **Телеметрия** - через TELEMETRY_APP сообщения
4. **Информация о узлах** - через NODEINFO_APP сообщения

### Обработка данных

```dart
void _handleMeshPacket(MeshPacket packet) {
  switch (packet.decoded?.portnum) {
    case PortNum.POSITION_APP:
      _handlePositionData(packet.decoded!.payload, packet.from);
      break;
    case PortNum.TELEMETRY_APP:
      _handleTelemetryData(packet.decoded!.payload, packet.from);
      break;
    case PortNum.NODEINFO_APP:
      _handleNodeInfoData(packet.decoded!.payload, packet.from);
      break;
  }
}
```

## Локальное хранение

### SQLite схема

```sql
-- Информация о узлах
CREATE TABLE node_info (
  node_num INTEGER PRIMARY KEY,
  long_name TEXT,
  short_name TEXT,
  macaddr TEXT,
  hw_model TEXT,
  role TEXT,
  last_seen INTEGER
);

-- Лог позиций
CREATE TABLE position_log (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  node_num INTEGER,
  timestamp INTEGER,
  latitude REAL,
  longitude REAL,
  altitude INTEGER,
  speed_ms REAL,
  track_deg REAL,
  precision_bits INTEGER,
  raw_data BLOB,
  FOREIGN KEY (node_num) REFERENCES node_info (node_num)
);

-- Лог метрик
CREATE TABLE device_metrics_log (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  node_num INTEGER,
  timestamp INTEGER,
  battery_level REAL,
  voltage REAL,
  channel_util REAL,
  air_util_tx REAL,
  uptime_seconds INTEGER,
  raw_data BLOB,
  FOREIGN KEY (node_num) REFERENCES node_info (node_num)
);
```

## Запросы данных

### Активные запросы

```dart
// Запрос позиции конкретного узла
Future<void> requestPosition(int nodeNum, int channel) async {
  final queryPos = Position(); // Пустой Position как запрос
  final data = Data()
    ..portnum = PortNum.POSITION_APP
    ..payload = queryPos.writeToBuffer()
    ..wantResponse = true;
  
  final packet = MeshPacket()
    ..to = nodeNum
    ..channel = channel
    ..decoded = data
    ..hopLimit = 3;
  
  final toRadio = ToRadio()..packet = packet;
  await _toRadio!.write(toRadio.writeToBuffer());
}
```

## Обработка ошибок

### Типичные проблемы

1. **Неправильный канал** - данные не приходят
2. **Неправильный handshake** - устройство не отвечает
3. **MTU проблемы** - большие пакеты не передаются
4. **Отключение устройства** - null pointer exceptions

### Решения

```dart
// Проверка подключения перед операциями
if (_fromRadio == null) {
  print('❌ Устройство не подключено');
  return;
}

// Обработка отключения
void _handleDisconnection() {
  _fromRadio = null;
  _toRadio = null;
  _fromNum = null;
  // Очистка подписок и таймеров
}
```

## Best Practices

1. **Всегда используйте официальные protobuf классы**
2. **Реализуйте адаптивное определение каналов**
3. **Сохраняйте все данные локально**
4. **Обрабатывайте отключения gracefully**
5. **Используйте правильный handshake sequence**
6. **Логируйте все операции для отладки**

## Troubleshooting

### GPS данные не приходят

1. Проверьте настройки каналов на устройстве
2. Убедитесь что GPS включен
3. Проверьте что "Share phone location" включен
4. Попробуйте разные каналы (0-8)

### Устройство не отвечает

1. Проверьте правильность handshake
2. Убедитесь что MTU установлен правильно
3. Проверьте подписку на FromNum
4. Убедитесь что читаете FromRadio до пустого

### Производительность

1. Используйте периодическое чтение FromRadio
2. Кэшируйте данные в локальной БД
3. Ограничивайте частоту запросов
4. Очищайте старые данные

## Заключение

Этот API позволяет полностью интегрироваться с устройствами Meshtastic из Flutter приложений. Ключевые моменты:

- Правильный BLE handshake
- Адаптивное управление каналами
- Локальное хранение данных
- Обработка всех типов сообщений
- Graceful error handling

Следуя этому руководству, можно создать надежное приложение для работы с mesh сетями Meshtastic.
