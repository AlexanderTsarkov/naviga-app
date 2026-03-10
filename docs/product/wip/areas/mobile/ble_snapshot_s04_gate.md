# Mobile — BLE snapshot for S04: gate and deferral note

**Status:** WIP (gating note). **Issue:** [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361) · **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351)

This document is a **docs-only gating note**. It states that **detailed BLE snapshot design is deferred** until S03 canon and code convergence are complete. It does **not** define BLE snapshot fields, format, cadence, GATT, transport, or event model. It lists the required S03 inputs and a checklist for starting real BLE snapshot design in S04.

---

## 1) Purpose of this note

- **Purpose:** Make explicit that the “BLE snapshot outline for S04” planning task ([#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361)) is **reframed** as a **gate/deferral note**: we record *what must be true before* S04 designs the actual BLE snapshot, and we **do not** produce a detailed BLE snapshot spec in S03.
- **Scope:** Embedded-first architecture; NodeTable as product heart. The BLE snapshot will expose a subset of NodeTable (and related state) to the mobile app; the exact subset, encoding, and triggers are an **S04 design task** once S03 semantics and code are stable.

---

## 2) Why detailed BLE snapshot design is deferred

- **S03 canon and code convergence are not yet complete.** NodeTable field map, packet → NodeTable mapping, packet context/TX rules, and versioning/identity semantics (fwVersionId, hwProfileId) have been stabilized in **WIP** and partially in canon; promotion of WIP to canon and **code parity** with accepted semantics are still in progress or planned under the S03 Execution umbrella.
- **Designing BLE snapshot fields/format/cadence in detail now** would risk rework when S03 canon and implementation align (e.g. Node_Pos_Full / Node_Status v0.2 direction, final field set, persistence and export rules). This note defers that detailed design to **post–S03 stabilization**, i.e. when starting S04.

---

## 3) What must stabilize in S03 first

- **Accepted NodeTable canon:** Field semantics, storage, persistence, and on-air mapping agreed and reflected in canon (not only WIP).
- **Accepted field map:** NodeTable Field Map (or equivalent master table) accepted as the source of truth for which fields exist, how they are stored, and which are exported (e.g. `exported_over_ble`).
- **Accepted packet → NodeTable mapping:** Clear and stable mapping from on-air packets to NodeTable field updates; see [packet_to_nodetable_mapping_s03.md](../../nodetable/policy/packet_to_nodetable_mapping_s03.md) ([#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358)).
- **Accepted packet context / TX rules:** Packet purpose, contents, creation, and send rules stabilized; see [packet_context_tx_rules_v0_1.md](../../radio/policy/packet_context_tx_rules_v0_1.md) ([#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407)).
- **Code parity with accepted semantics:** Firmware (and any existing BLE bridge) aligned with the accepted NodeTable/packet semantics so that “what we document” and “what the device does” are consistent. S03 Execution phase.

---

## 4) What S04 will design later

- **BLE snapshot scope:** Which NodeTable (and related) fields are exposed to the mobile app (map/list/status and any extensions).
- **Format and cadence/trigger:** Encoding of the snapshot (e.g. record layout, paging), when the snapshot is produced or refreshed, and how the app requests or subscribes (GATT/transport/event model — all S04).
- **Contracts and policies:** BLE snapshot contract/spec as an S04 deliverable, building on the S03-stable NodeTable and packet semantics.

---

## 5) Required inputs from S03 (checklist for S04 start)

Before starting **detailed** BLE snapshot design in S04, the following should be in place:

| Input | Description | Reference |
|-------|-------------|-----------|
| **Accepted NodeTable canon** | Field semantics, storage, persistence, on-air mapping in canon (not only WIP). | [NodeTable hub](../../../areas/nodetable/index.md); [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv); S03 promotion. |
| **Accepted field map** | NodeTable Field Map or master table accepted as source of truth; `exported_over_ble` and consumer notes stable. | [master_table README](../../nodetable/master_table/README.md); [nodetable_field_map_v0_1](../../nodetable/policy/nodetable_field_map_v0_1.md) (WIP). |
| **Accepted packet → NodeTable mapping** | Which packets update which fields; gating and RX/self rules. | [packet_to_nodetable_mapping_s03.md](../../nodetable/policy/packet_to_nodetable_mapping_s03.md) ([#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358)). |
| **Accepted packet context / TX rules** | Packet purpose, contents, creation, send rules (including v0.2 direction where adopted). | [packet_context_tx_rules_v0_1.md](../../radio/policy/packet_context_tx_rules_v0_1.md) ([#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407)). |
| **Code parity with accepted semantics** | FW (and BLE bridge) behavior aligned with accepted NodeTable/packet semantics. | S03 Execution; implementation/validation. |

---

## 6) Non-goals and scope guardrails

This document is **not** a BLE snapshot contract or spec. It does **not**:

- Define actual BLE snapshot fields, record layout, or byte format.
- Define GATT services/characteristics, transport, serialization, or event model.
- Introduce new NodeTable semantics or change existing ones.
- Promote unfinished WIP content into canon.
- Require or imply firmware or mobile code changes.

**Scope:** Gate/deferral note and S04 entry checklist only. Detailed BLE snapshot design is a **post–S03 stabilization** task (S04).

---

## 7) S04 entry checklist (when to start detailed design)

- [ ] S03 planning WIP promoted to canon where agreed (per [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) and promotion policy).
- [ ] NodeTable field map / master table accepted and stable for export subset.
- [ ] Packet → NodeTable mapping and packet context/TX rules accepted and reflected in canon/code.
- [ ] Code parity: FW (and existing BLE snapshot path, if any) aligned with accepted semantics.
- [ ] S04 BLE snapshot design task created and scoped (fields, format, cadence/trigger, contract).

---

## 8) Open questions (not answered in this doc)

- Which NodeTable fields are in the **first** BLE snapshot subset for map/list/status (priority order).
- Whether snapshot is full-table dump only or supports incremental/partial refresh.
- Cadence/trigger: on-demand only, periodic, or event-driven; and how the app requests or subscribes.
- Backward compatibility and versioning of the BLE snapshot format once S04 defines it.

These are to be answered in S04 when detailed BLE snapshot design starts.

---

## 9) Related

- **Umbrella:** [#351](https://github.com/AlexanderTsarkov/naviga-app/issues/351) — S03 Planning (WIP): NodeTable as Product Heart.
- **This issue:** [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361) — reframed as gate/deferral; detailed outline = post-S03.
- **Packet → NodeTable mapping:** [packet_to_nodetable_mapping_s03.md](../../nodetable/policy/packet_to_nodetable_mapping_s03.md) ([#358](https://github.com/AlexanderTsarkov/naviga-app/issues/358)).
- **Packet context / TX rules:** [packet_context_tx_rules_v0_1.md](../../radio/policy/packet_context_tx_rules_v0_1.md) ([#407](https://github.com/AlexanderTsarkov/naviga-app/issues/407)).
- **NodeTable hub and contracts:** [NodeTable index](../../../areas/nodetable/index.md); [fields_v0_1.csv](../../nodetable/master_table/fields_v0_1.csv), [master_table README](../../nodetable/master_table/README.md).
- **Mobile integration audit (current BLE snapshot usage):** [s02_230_mobile_nodetable_integration_audit.md](../../../areas/mobile/audit/s02_230_mobile_nodetable_integration_audit.md) ([#230](https://github.com/AlexanderTsarkov/naviga-app/issues/230)) — existing app/FW snapshot layout; S04 design will build on or extend this.
