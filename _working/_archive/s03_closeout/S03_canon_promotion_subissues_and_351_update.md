# S03 Canon Promotion — Sub-issues and #351 update

Use these to create sub-issues (optional; only if it reduces risk) and to update the master umbrella #351.

---

## 1) Create umbrella first

Create the umbrella issue from `_working/S03_canon_promotion_umbrella_issue_body.md`, then create sub-issues and link them. Add umbrella + sub-issues to **Project #3** with **Status = Ready**.

---

## 2) Sub-issues (create only if useful)

Each sub-issue: **Goal**, **Outputs**, **DoD**, **Links** (umbrella + related). Add to Project #3, Status = Ready.

### P0 — Canon promotion: NodeTable + packet contracts + RX semantics

**Title:** `S03 promotion (P0): NodeTable field map, packet contracts, RX semantics → canon`

**Body:**

- **Goal:** Promote the S03 NodeTable slice to canon so that field map, packet→NodeTable mapping, packet sets, presence/age, identity/naming/persistence/eviction, seq/ref/version/link metrics, TX priority/arbitration, and RX semantics (where not yet canon) are the single source of truth. Largest risk area; do first.
- **Outputs:** Canon docs under `docs/product/areas/nodetable/` (policy + contract as per layout); S03 slice index updated to point to canon paths; spec_map updated; no canonical doc references WIP for these definitions.
- **DoD:** (1) Inventory of NodeTable WIP docs done; each either promoted (move/merge + redirect stub) or marked WIP with "WIP (not canon)" banner. (2) Canon readable without WIP context. (3) Reference-impl disclaimer present where appropriate.
- **Links:** Umbrella (S03 WIP→Canon Promotion), #351, #352, #407, #358; layout policy, promotion policy.

---

### P0 — Canon promotion: Provisioning / boot policy

**Title:** `S03 promotion (P0): Provisioning baseline + OOTB autonomous start → canon`

**Body:**

- **Goal:** Promote provisioning baseline and OOTB autonomous start to canon; ensure boot/provisioning policy is the source of truth and canon links to canon (registries, role profiles, etc.), not WIP.
- **Outputs:** Canon docs for provisioning baseline and OOTB autonomous start under `docs/product/areas/firmware/policy/`; cross-links from boot_pipeline, provisioning_interface, module_boot_config to promoted paths; spec_map updated.
- **DoD:** (1) Provisioning baseline and OOTB autonomous start either in canon or explicitly WIP with banner. (2) No canon doc depends on WIP for provisioning/boot semantics.
- **Links:** Umbrella (S03 WIP→Canon Promotion), #351, #385, #354; boot_pipeline_v0, provisioning_interface_v0.

---

### P1 — Canon promotion: Registries (fw/hw ids) and their links

**Title:** `S03 promotion (P1): fwVersionId / hwProfileId semantics + canon links`

**Body:**

- **Goal:** Registries (fw_version_id_registry_v0, hw_profile_id_registry_v0) are already in canon. Promote fwVersionId and hwProfileId **semantics** docs to canon (or add "WIP (not canon)" banner); ensure all canon policy docs link to canon registry paths only.
- **Outputs:** Semantics docs either promoted to `docs/product/areas/firmware/` or explicitly WIP with banner; provisioning_interface_v0 and any other canon docs reference only canon paths for registries and semantics; no WIP-only links for normative semantics.
- **DoD:** (1) Canon semantics for fwVersionId/hwProfileId clear (either in canon or explicitly deferred). (2) All canon→canon links for registries/semantics.
- **Links:** Umbrella (S03 WIP→Canon Promotion), #351, #405, #406, #409, #410; fw_version_id_registry_v0, hw_profile_id_registry_v0.

---

### P2 — Canon promotion: Radio policy (tx rules / packetization) or WIP banner

**Title:** `S03 promotion (P2): Radio policy (packet context, TX rules, traffic model) → canon or WIP banner`

**Body:**

- **Goal:** Promote packet context/TX rules and traffic model to canon if stable; otherwise keep in WIP with explicit "WIP (not canon)" banner and one-line reason. Optionally promote tx power contract, radio profiles model, E220 mapping, channel-utilization only if agreed stable.
- **Outputs:** Either canon docs under `docs/product/areas/radio/policy/` for packet context/TX rules and traffic model, or WIP docs with clear banner; spec_map and cross-links updated accordingly.
- **DoD:** (1) Each radio policy doc either promoted or explicitly WIP with reason. (2) No canon doc defines semantics by reference to unstable WIP without banner.
- **Links:** Umbrella (S03 WIP→Canon Promotion), #351, #407, #360; packet_context_tx_rules_v0_1, traffic_model_s03_v0_1.

---

### P2 — BLE gate doc: remain WIP gate + link from canon

**Title:** `S03 promotion (P2): BLE snapshot gate — WIP gate only, link from canon`

**Body:**

- **Goal:** Confirm BLE snapshot gate doc remains a **WIP gate** (S04); do not specify BLE. Add or update a link from canon (e.g. current_state or mobile/index) so that the gate is discoverable without pretending BLE is specified.
- **Outputs:** Gate doc has explicit "WIP (not canon)" banner; canon has a short pointer (e.g. "BLE snapshot design deferred to S04; see [gate doc] for conditions and checklist"); no BLE semantics in canon.
- **DoD:** (1) Gate doc clearly non-normative and deferral reason stated. (2) Canon links to gate for discovery only; no BLE spec in canon.
- **Links:** Umbrella (S03 WIP→Canon Promotion), #351, #361; ble_snapshot_s04_gate.md.

---

## 3) Update issue #351

Add the following to the **body** of #351 (e.g. new section "Next phase" or "Promotion to Canon"):

**Section to add:**

```markdown
---

### Next: S03 WIP → Canon Promotion

- **S03 planning:** Complete. All planned S03 planning slices (NodeTable, packet/radio, provisioning, registries, BLE gate) have been addressed in WIP.
- **Next phase:** [S03: WIP → Canon Promotion (planning slice to canon)](link-to-new-umbrella-issue) — promote S03 WIP to canonical docs; ensure canon is self-contained and is the source of truth; treat current code as reference-only.
- **After promotion:** Implementation planning and execution start after promotion completes. Do not enforce backward compatibility or protocol version bumps in this phase; no release yet.
```

- Replace `(link-to-new-umbrella-issue)` with the actual issue URL after creating the umbrella (e.g. `https://github.com/AlexanderTsarkov/naviga-app/issues/NNN`).
- Optionally add a checkbox: `- [ ] S03 WIP → Canon promotion umbrella created and in progress.`

**Project board:** Add the new promotion umbrella (and any sub-issues) to **Project #3** with **Status = Ready**.
