# Naviga — Start here

**Naviga** is an autonomous outdoor navigation and communication system: embedded firmware (ESP32 + LoRa) + BLE bridge + mobile app. Offline-first; mesh and JOIN are future.

## What works now (OOTB v1)

- **Firmware:** GEO_BEACON TX/RX, NodeTable, BLE NodeTableSnapshot/DeviceInfo. E220 UART LoRa as modem; M1Runtime wires domain ↔ radio ↔ BLE.
- **Mobile (Flutter):** Connect, My Node, Nodes, Map (online tiles), Settings. BLE read-only; NodeTable cache.
- **OOTB v1** completed 2026-02-12; issues #2–#35 closed. Evidence: [OOTB progress inventory](project/ootb_progress_inventory.md), [OOTB workmap](project/ootb_workmap.md).

## How to navigate docs

| Where | What |
|-------|------|
| **[State after OOTB v1](project/state_after_ootb_v1.md)** | Snapshot: what’s in, invariants, next steps. |
| **[Docs map: canonical vs working](project/docs_map.md)** | Which docs are canonical, reference, or to be archived (PR3). |
| [Architecture index](architecture/index.md) | Layer map, source-of-truth table, repo layout. |
| [OOTB progress inventory](project/ootb_progress_inventory.md) | Per-issue PR evidence and Mobile v1 tracking. |
| [OOTB workmap](project/ootb_workmap.md) | Plan, status table, rules. |

For development workflow and PR discipline, see repo root **CLAUDE.md** and `docs/dev/` (reference only; open explicitly when needed).
