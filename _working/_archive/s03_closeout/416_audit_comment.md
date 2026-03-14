**Post-merge hygiene:** #435 and #438 checklists reconciled against merged PRs and repo state.

- **#435** — All 7 DoD items confirmed done via PR [#436](https://github.com/AlexanderTsarkov/naviga-app/pull/436) (design) and [#437](https://github.com/AlexanderTsarkov/naviga-app/pull/437) (implementation). Checkboxes updated; no items moved or left undone.
- **#438** — All 7 DoD items confirmed done via PR [#440](https://github.com/AlexanderTsarkov/naviga-app/pull/440) (final v0.2-only cutover). Checkboxes updated; no remaining undone items.

**Final protocol state:** TX = v0.2 only; RX = v0.2 only. Packet family: Node_Pos_Full (0x06), Node_Status (0x07), Alive (0x02). v0.1 packet types (0x01, 0x03, 0x04, 0x05) are rejected on RX. Migration docs and packet_sets state cutover complete.

**Remaining follow-ups:** None for #435/#438 scope. BLE linker (test_ble_transport_core) remains pre-existing and out of scope.
