# Tail1/Tail2 instrumentation — finding (PR #276)

**Date:** 2026-02-23

## Finding

**В прошивке нет реальных TX/RX путей для Tail1/Tail2.**

- **On-air:** Реализован только один тип пакета — `GeoBeaconFields`, 24 байта, `kGeoBeaconMsgType = 0x01`, один кодек: `encode_geo_beacon` / `decode_geo_beacon` (protocol/geo_beacon_codec.*).
- **BeaconLogic::build_tx** формирует один пакет за тик (CORE или ALIVE по `time_for_silence` и `pos_valid`). Очереди tail, отдельной сборки Tail-1/Tail-2 или поля `core_seq` в payload нет.
- **BeaconLogic::on_rx** парсит только этот же 24-байтный формат; различения Core vs Tail по заголовку/типу нет.
- В репозитории нет файлов *tail*, *alive* (отдельных кодеков); спека [beacon_payload_encoding_v0](docs/product/areas/nodetable/contract/beacon_payload_encoding_v0.md) описывает Core 19 B, Tail-1 (core_seq16), Tail-2, Alive — в FW это не реализовано.

## Следствия

- Подключить instrumentation к «реальным» Tail TX/RX путям **невозможно** без появления в коде сборки/парсинга tail-пакетов.
- Proof-run не может показать `pkt tx type=TAIL1|TAIL2 ... core_seq=<nonzero>` или соответствующие rx, пока tail не будет закодирован и отправлен/принят.
- Причины drop/skip (NEW_CORE_PREEMPT, WINDOW_EXPIRED, QUEUE_FULL) относятся к логике планировщика tail; такой логики в коде нет — добавлять нечего.

## Рекомендации

1. **Оставить PR #276 как есть:** инструментация готова к типу CORE/TAIL1/TAIL2/ALIVE и `core_seq`; реально в логах будут только CORE и ALIVE до появления tail в прошивке.
2. **Реализацию Tail** вынести в отдельную задачу/PR: отдельные кодек(и) и msg_type (или иной способ различения) для Tail-1/Tail-2 по спеке, очередь/планировщик tail, затем подключить к существующим логам (уже готовый формат с `type=TAIL1|TAIL2` и `core_seq`).
3. **Proof-run** для #276 имеет смысл только для проверки CORE/ALIVE и формата логов; папку `_working/hw_tests/2026-02-23_s02_276_tail_proof/` можно завести для такого run и явно зафиксировать в notes.md, что tail в логах не ожидается (нет реализации).

## Exit criteria (обновлённые)

- [x] PR #276 логи различают type= и поддерживают core_seq для tail (формат готов).
- [ ] **Реальные** TAIL с core_seq ≠ 0 появятся только после реализации tail-пакетов в FW (отдельная задача).
- [ ] Proof-run: возможен для проверки CORE/ALIVE и формата; артефакты в `_working/hw_tests/..._s02_276_tail_proof/` с пометкой в notes.md.
