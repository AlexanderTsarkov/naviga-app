# POC Evidence: LoRa E220-400T30D (Radio Smoke Tests)

**Purpose:** Extract engineering evidence from the archived POC for the E220-400T30D radio path. No code migration—documentation only. Used to inform the OOTB Radio v0 spec and firmware HAL.

**Related:** [OOTB v0 analysis and plan](../product/OOTB_v0_analysis_and_plan.md) (Phase 0.5.1), Issue #7.

---

## 1. Hardware Setup (Assumptions)

- **MCU:** ESP32 (e.g. DevKit or similar 3.3V board).
- **Radio module:** Ebyte **E220-400T30D** (LLCC68-based, 410–493 MHz, default 433.125 MHz; up to 30 dBm; UART TTL).
- **Interface:** **UART** (primary; E220-400T30D is UART-based). SPI is supported on the E22-400T30D variant; this POC is documented for the UART variant.
- **Wiring assumptions:**
  - ESP32 UART TX → E220 RX, ESP32 UART RX → E220 TX, GND common, 3.3V.
  - **AUX** pin: if used, 4.7 kΩ pull-up to 3.3V recommended to avoid freezes (per vendor notes).
  - M0/M1: as per datasheet for mode (e.g. normal operation vs config).
- **Tooling:** Dual-ESP32 serial capture via `archive/poc-20260131/tools/dual_serial_log.py` (two boards, timestamped logs, optional RTS reset).

### 1.1. Library Used in POC (Message Exchange Between 2 Boards)

В тестовом коде обмена сообщениями между двумя платами использовалась **внешняя библиотека** для работы с E220 (не собственный низкоуровневый драйвер). В репозитории Naviga тестовый firmware и `platformio.ini` не хранятся; точное имя и версия библиотеки восстанавливаются по проекту тестов (например, из `lib_deps` в meshtastic-firmware или локального стенда).

Типичные варианты для E220-400T30D (LLCC68) под Arduino/ESP32 и PlatformIO:

| Библиотека | Описание / источник |
|------------|----------------------|
| **EByte LoRa E220 Series Library** | [xreef/EByte_LoRa_E220_Series_Library](https://github.com/xreef/EByte_LoRa_E220_Series_Library) — поддержка UART/SPI, конфиг, TX/RX, примеры для двух устройств. |
| **Renzo Mischianti — Ebyte LoRa E220 LLCC68** | Документация и примеры: [mischianti.org](https://mischianti.org/ebyte-lora-e220-llcc68-device-for-arduino-esp32-or-esp8266-library-2/); часто используется вместе с библиотекой выше или аналогом. |

Рекомендация для будущей реализации OOTB Radio v0: зафиксировать выбранную библиотеку и версию в `firmware/platformio.ini` (или аналоге) и не копировать её код в репо — подтягивать как зависимость (см. также `archive/poc-20260131/docs/CLEAN_SLATE.md`, раздел про E220/LoRa как зависимости).

*Если при восстановлении тестового проекта будет найдено точное имя пакета (например, `lib_deps = ...`), его стоит добавить сюда для полноты evidence.*

---

## 2. What Was Proven Working (Checklist)

Derived from the OOTB plan and archive contents (no E220 firmware or raw test logs are stored in this repo):

- [x] **Radio link (ESP32 + E220):** Basic TX/RX between two nodes assumed validated in POC (plan: “радио-обмен (ESP32+E220)”).
- [x] **Concurrent send / contention:** “Конкурентная отправка” (competing send) was in scope of smoke tests.
- [x] **ACK / sequencing:** “ack” (acknowledgement and/or sequence handling) was in scope.
- [x] **Power control:** “Управление мощностью” (TX power change) was in scope.

**Note:** The archive in this repo (`archive/poc-20260131/`) contains **docs, naviga_app (Flutter/Meshtastic protos), and tools**—not the actual E220 test firmware or session logs. Any concrete test runs or logs live in the **meshtastic-firmware** clone (branch `naviga-custom`) or local captures; see MANIFEST below for reference.

---

## 3. Test Parameters Used (Reference Table)

| Parameter        | Value / assumption        | Notes                          |
|-----------------|---------------------------|--------------------------------|
| Module          | E220-400T30D (UART)       | 433 MHz band, 30 dBm max       |
| Interface       | UART                      | 3.3V TTL                       |
| Frequency       | 433.125 MHz (default)     | Or as configured for region    |
| TX power        | Configurable (datasheet)  | Power-change tests exercised   |
| CCA / LBT       | Module supports LBT       | Listen Before Talk (E220 spec) |
| ACK / sequence  | Protocol-dependent        | POC scope; exact format TBD v0 |
| Baud rate       | Per E220 config           | Typically 9600 for config      |

*(If specific values from past runs are recovered from meshtastic-firmware or logs, add a row or “POC measured” column.)*

---

## 4. Known Issues / Gaps

- **RSSI/SNR:** Not yet standardized in this repo for OOTB v0. E220/LLCC68 can report RSSI; mapping to NodeTable and BLE contract is for Radio v0 / BLE v0 specs.
- **No in-repo logs:** No raw E220 session logs or config dumps are stored in the Naviga repo; only tooling and MANIFEST. Evidence is “plan + tooling + MANIFEST”; detailed traces live in meshtastic-firmware or local files.
- **CCA/LBT usage:** LBT is supported by the module; whether and how it was used in POC (e.g. backoff, retries) is not documented here.
- **GNSS:** Out of scope for this document; see OOTB plan (GNSS stub for v0).
- **BLE + radio in one loop:** Integration of BLE and radio in a single firmware cycle was listed as a gap to address in the full OOTB flow.

---

## 5. Implications for OOTB Radio v0 and Firmware HAL

### 5.1. Required HAL / Radio Interface (from this evidence)

- **Init:** Load default or persisted config (frequency, power, mode).
- **TX:** Send payload (e.g. GEO_BEACON bytes) with optional timeout/retry policy.
- **RX:** Receive path (poll or callback) and pass payload to protocol layer.
- **Config:** Set frequency, TX power, and, if used, CCA/LBT-related parameters.
- **Power control:** Set TX power level (proven in POC scope).
- **Optional:** CCA / “channel busy” before TX (E220 supports LBT).
- **Optional:** RSSI/SNR for received frame (for NodeTable and BLE).

### 5.2. Logging (for testability and field debug)

- TX: payload length, destination/type if any, power level, timestamp.
- RX: payload length, RSSI/SNR if available, timestamp.
- Config changes: frequency, power, mode.
- Decode ok/err (protocol layer).
- NodeTable updates (from protocol/domain).

*(See Phase 2.2 in OOTB plan: ring-buffer + export via BLE or UART.)*

---

## 6. Archive and External References

- **This repo:**  
  - `archive/poc-20260131/` — POC snapshot: docs, `naviga_app`, `tools` (e.g. `dual_serial_log.py`), MANIFEST.  
  - No E220 firmware or radio test logs under `archive/`.

- **MANIFEST (for meshtastic-firmware):**  
  - `archive/poc-20260131/MANIFEST-meshtastic-repos.txt`  
  - meshtastic-firmware: `https://github.com/meshtastic/firmware.git`, branch `naviga-custom`, commit `fb757f0314b3afbca79a23285979b36838aa45af`.  
  - Use this to clone and search for E220/radio-related code or logs if needed.

- **Tooling excerpt:**  
  - `archive/poc-20260131/tools/dual_serial_log.py` — dual-ESP32 serial logger; usage example:
    ```text
    python3 tools/dual_serial_log.py \
      --port-a /dev/cu.wchusbserialAAAA \
      --port-b /dev/cu.wchusbserialBBBB \
      --out-a boardA.log --out-b boardB.log \
      --seconds 90
    ```
  - Optional RTS pulse for ESP32 reset to capture boot in logs.

- **Docs:**  
  - `archive/poc-20260131/docs/CLEAN_SLATE.md` — E220-400T30D (and LoRa stack) described as **dependencies** (e.g. via platformio.ini), not copied into repo; own code in `firmware/`.

---

## 7. Verification

- [ ] Document is present under `docs/firmware/poc_e220_evidence.md`.
- [ ] Hardware setup, checklist, parameters table, gaps, and HAL/logging implications are complete.
- [ ] No code from archive imported; PR adds only this documentation (and possibly references under `docs/`).

This document is sufficient for future Radio v0 implementation and HAL design (e.g. `IRadio` under `docs/firmware/hal_contracts_v0.md` and `docs/protocols/ootb_radio_v0.md`).
