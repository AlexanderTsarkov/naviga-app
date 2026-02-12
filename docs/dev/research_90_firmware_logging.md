# Firmware Logging Research — Ticket #90

**Purpose:** Structured research on firmware logging implementation and BLE log retrieval capability. No implementation changes — documentation only.

---

## 1. Current Logging Implementation

### Files

| Path | Role |
|------|------|
| `firmware/src/domain/logger.h` | Logger class declaration |
| `firmware/src/domain/logger.cpp` | Ring buffer implementation |
| `firmware/src/domain/log_events.h` | Event IDs enum |
| `firmware/lib/NavigaCore/include/naviga/platform/log.h` | Platform ILogger abstraction |
| `firmware/src/platform/arduino_logger.cpp` | Arduino Serial logger (platform) |
| `firmware/src/platform/log_export_uart.cpp` | UART export (drain to Serial) |
| `firmware/src/platform/log_export_uart.h` | UART export API |

### Log Format

**Domain Logger (event log):** Binary records, little-endian:

| Field | Type | Size | Description |
|-------|------|------|--------------|
| t_ms | uint32 | 4 | Uptime in ms at record time |
| event_id | uint16 | 2 | Event type (see `log_events.h`) |
| level | uint8 | 1 | LogLevel (Debug=0, Info=1, Warn=2, Error=3) |
| len | uint8 | 1 | Payload length (0..255) |
| payload | byte[] | len | Optional event data |

**UART export format (text):** `LOG t_ms=<n> event=<id> level=<n> len=<n> payload=<hex>\n`

### Storage

- **Type:** RAM ring buffer only (no NVS, LittleFS, SPIFFS).
- **Buffer:** In-process `uint8_t storage_[kDefaultRingSize]` (or external pointer via constructor).
- **Size:** `kDefaultRingSize = 2048` bytes (configurable via `Logger(uint8_t* storage, size_t capacity)`).
- **Overwrite policy:** `drop_oldest()` when full — oldest records evicted to make space.
- **Persistence:** None; logs lost on power cycle.

### Levels

- `kDebug` (0), `kInfo` (1), `kWarn` (2), `kError` (3)
- Defined in both `domain::LogLevel` and `platform::LogLevel`.

### Event IDs (`log_events.h`)

| ID | Name | Description |
|----|------|-------------|
| RADIO_TX_OK | 0x0101 | TX success |
| RADIO_TX_ERR | 0x0102 | TX failure |
| RADIO_RX_OK | 0x0103 | RX success |
| RADIO_RX_ERR | 0x0104 | RX failure |
| DECODE_OK | 0x0201 | Beacon decode ok |
| DECODE_ERR | 0x0202 | Beacon decode error |
| NODETABLE_UPDATE | 0x0301 | NodeTable updated |
| BLE_CONNECT | 0x0401 | BLE connected |
| BLE_DISCONNECT | 0x0402 | BLE disconnected |
| BLE_READ | 0x0403 | BLE characteristic read |

### Enable/Disable Mechanism

- No compile-time or runtime switch. Logging is always enabled when `event_logger_` is used.
- Platform logger (ArduinoLogger) writes to Serial whenever `log()` is called.

### UART Drain Behavior (`log_export_uart.cpp`)

- Triggered from `app_services.cpp` every **5 seconds** in `tick()`.
- **Condition:** Only drains if `total_len <= kMaxBytesPerDrain` (512 bytes) and `Serial.availableForWrite() >= total_len`.
- **Effect:** `drain()` clears the ring buffer after emitting; records are consumed and removed.
- **Output:** Human-readable lines to Serial (UART).

---

## 2. BLE Exposure

### Service UUID

- **Naviga service:** `6e4f0001-1b9a-4c3a-9a3b-000000000001`

### Characteristic UUIDs (existing)

| UUID | Name | Access | Purpose |
|------|------|--------|---------|
| `6e4f0002-...` | DeviceInfo | READ | Identity, capabilities |
| `6e4f0003-...` | NodeTableSnapshot | READ + WRITE | Paged NodeTable |

### Log Characteristic

**Does not exist.** No BLE service or characteristic exposes logs.

- `ble_service.cpp` (legacy Page0..3) — no log characteristic.
- `ble_esp32_transport.cpp` (current NodeTableSnapshot) — no log characteristic.
- Protocol spec `ootb_ble_v0.md` — no log characteristic defined.

### Protocol / Limitations

- Logs are **not** readable via BLE READ.
- Logs are **not** streamed via BLE NOTIFY.
- There is **no** write command to trigger a log dump.
- No chunking/framing/MTU handling for logs (because no BLE log path exists).

---

## 3. Mobile Support Status

### Existing Support

- **Service/characteristic usage:** Mobile uses `6e4f0001`, `6e4f0002`, `6e4f0003` for Connect/NodeTable.
- **BLE read logic:** `connect_controller.dart` has reusable BLE read flow: discover services → find characteristic by UUID → read.
- **Logging:** `lib/shared/logging.dart` provides `logInfo()` (debugPrint) for app-level logs; no firmware log consumption.

### Missing Pieces

- No firmware log fetching over BLE.
- No log/diagnostics UI; Settings screen has: `'TODO: About + diagnostics (no BLE config in v1).'`
- No UUID or characteristic for logs; nothing to discover.

---

## 4. Gap Analysis

### What Exists

- Domain event logger (ring buffer, binary records).
- UART export (periodic drain to Serial).
- BLE telemetry (DeviceInfo, NodeTableSnapshot).
- HAL/planning docs (ILog, `exportToBle` TBD; `logging_v0.md` describes future flash + BLE).

### What Is Missing

- BLE characteristic for logs.
- Any BLE protocol for log retrieval (READ, NOTIFY, or WRITE-triggered dump).
- Persistent log storage (flash); current is RAM only.
- Mobile diagnostics UI or log viewer.
- Chunking/MTU handling for log transfer.
- End-of-log marker or framing over BLE.

### Risks

- **RAM size:** 2 KB ring buffer fills quickly under high event rate; short retention window.
- **UART-only:** Logs available only via serial cable; no remote diagnostics.
- **Drain limits:** `drain_logs_uart` skips if total > 512 bytes; logs can be dropped without export.
- **Design docs vs code:** `logging_v0.md` and HAL describe flash + BLE; implementation is RAM + UART only.

---

## 5. Proposal (Minimal MVP)

**BLE log does NOT exist.** Therefore a minimal protocol is proposed.

### Option A: Single READ Characteristic

- **Characteristic UUID:** `6e4f0004-1b9a-4c3a-9a3b-000000000001` (or next free in 6e4f000x range).
- **Properties:** READ.
- **Behavior:** Read returns current ring buffer content, up to MTU, in text format. Subsequent reads continue from offset (TBD: cursor/offset in request or fixed “last N bytes”).
- **Limitation:** BLE MTU (typically 20–512 bytes) limits single read; chunking via multiple reads required.

### Option B: Write-Triggered Chunked Response (Recommended)

- **Characteristic UUID:** `6e4f0004-1b9a-4c3a-9a3b-000000000001`.
- **Properties:** READ + WRITE.
- **Command (WRITE):** `GET_LOG` (e.g. 1-byte opcode `0x01`; optional 2-byte offset for pagination).
- **Response (READ):** Chunked text stream.
  - Each chunk: UTF-8 text lines, max size aligned with MTU (e.g. 200 bytes per chunk).
  - End marker: `<EOF>` on last chunk.
  - Format: same as UART (`LOG t_ms=... event=... level=... len=... payload=...`).
- **Flow:** App writes `GET_LOG` → polls READ until `<EOF>` or empty.

### Option C: NOTIFY Stream

- **Characteristic:** READ + NOTIFY.
- **Flow:** App subscribes to NOTIFY; firmware pushes chunks until done, then sends `<EOF>`.
- **Pros:** Lower latency, no polling. **Cons:** More complex; flow control and buffer management.

### Recommended Minimal MVP (Option B)

1. **Command:** WRITE `0x01` (GET_LOG) to Log characteristic.
2. **Response:** Chunked text via READ; firmware formats records as UART text.
3. **Chunk size:** `min(MTU - 3, 200)` bytes (reserve for ATT overhead).
4. **End marker:** `<EOF>` as final chunk (or empty read).
5. **Firmware:** Add `LogSnapshot` characteristic; on WRITE GET_LOG, prepare buffer; on READ, return next chunk.

---

## 6. Recommended Next Implementation Steps

### Firmware

1. Add Log characteristic `6e4f0004-...` with READ + WRITE.
2. Implement GET_LOG handler: on WRITE `0x01`, snapshot ring buffer (copy or iterate) without draining.
3. Format records to text (reuse `log_export_uart` formatting logic).
4. On READ: return next chunk (e.g. 200 bytes) until done; send `<EOF>` or empty as terminator.
5. Document in `ootb_ble_v0.md` (new § 1.x Log characteristic).
6. **Optional:** Increase ring buffer size if RAM allows; consider persistent storage later.

### Mobile

1. Add constant `kNavigaLogUuid = '6e4f0004-1b9a-4c3a-9a3b-000000000001'`.
2. In Settings (or a new Diagnostics screen): "Export logs" → connect → write GET_LOG → read chunks until `<EOF>`.
3. Display in scrollable view or allow share/save to file.
4. Reuse existing BLE connect/read flow from `connect_controller.dart`.

---

## References

- [logging_v0.md](../product/logging_v0.md) — Product vision (flash, BLE future).
- [hal_contracts_v0.md](../firmware/hal_contracts_v0.md) — ILog, exportToBle TBD.
- [ootb_ble_v0.md](../protocols/ootb_ble_v0.md) — Current BLE GATT spec.
- `firmware/src/domain/logger.*`, `log_export_uart.*`, `ble_esp32_transport.cpp`.
