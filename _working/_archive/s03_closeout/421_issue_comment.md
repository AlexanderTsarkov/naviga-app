## #421 implementation complete

Implementation is packaged in **PR #433** (https://github.com/AlexanderTsarkov/naviga-app/pull/433).

**Scope:** Current v0.1 RX/apply explicit canon alignment only. No packetization or v0.2 change.

**Changes:**
- **node_table.cpp** — Comment blocks state that RX/apply semantics mirror rx_semantics_v0 and packet_to_nodetable_mapping_v0; **legacy ref fields** (last_core_seq16, last_applied_tail_ref_core_seq16) are **runtime-local decoder state only** (not BLE, not persisted).
- **rx_semantics_v0.md** — §6 Implementation reference to node_table.cpp; ref fields per master table §7.
- **No behavior change** — code already matched canon; documentation/truth-lock only.

**Validation:**
- `pio run -e devkit_e220_oled` — passed.
- Existing tests relied upon (beacon_logic apply_tail1, node_table_domain seq/ooo and restore); no new tests added. No overclaim of new coverage.

**Boundary:** #422 = packetization / v0.2 only (unchanged).

After merge of the PR, this issue can be closed.
