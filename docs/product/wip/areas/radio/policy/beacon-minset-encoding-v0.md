# Radio — Beacon minset & encoding v0 (Policy WIP)

**Work Area:** Product Specs WIP · **Issue:** [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173)

This doc defines **beacon payload minset**, **message types**, **encoding rules**, and **airtime budget** for the frequent beacon (and related extended messages). **Encoding and byte budget are the focus.** Mesh routing is out of scope. RadioProfile / HW registries mapping deferred to [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159). OOTB/UI are non-normative.

---

## 1) Scope guardrails

- **In scope:** Field-by-field bytes, message types, cadence; beacon minset mapping from NodeTable contract (#158); encoding rules (versioning, optional fields, packing); airtime budget table (bytes→ms). LoRa settings inputs TBD.
- **Out of scope:** Mesh routing. Registries/mappings (#159). Full protocol design. This stub defines structure only; encoding details to be filled in per [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173).

---

## 2) Field-by-field bytes table & message types

*(Placeholder: field-by-field bytes table, message types, cadence. To be populated.)*

---

## 3) Beacon minset mapping (from NodeTable contract)

*(Placeholder: mapping from [NodeTable Link/Telemetry minset](../nodetable/contract/link-telemetry-minset-v0.md) (#158) — what is in the frequent beacon vs extended/less-frequent messages. To be populated.)*

---

## 4) Encoding rules v0

*(Placeholder: versioning, optional fields, packing guidelines. To be populated.)*

---

## 5) Airtime budget table (bytes → ms)

*(Placeholder: airtime budget table with TODO for LoRa settings inputs. To be populated.)*

---

## 6) Open questions / TODO

- LoRa settings (SF, BW, CR) inputs for airtime calculation.
- Exact payload cap and cadence parameters (see Issue #173 targets).

---

## 7) Related

- **NodeTable contract (Link/Telemetry minset):** [../nodetable/contract/link-telemetry-minset-v0.md](../nodetable/contract/link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158); product field set; encoding/beacon subset defined here.
- **Source precedence:** [../nodetable/policy/source-precedence-v0.md](../nodetable/policy/source-precedence-v0.md) — [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)
- **Persistence / snapshot / restore:** [../nodetable/policy/snapshot-semantics-v0.md](../nodetable/policy/snapshot-semantics-v0.md), [../nodetable/policy/restore-merge-rules-v0.md](../nodetable/policy/restore-merge-rules-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157); derived vs persisted separation.
- **Registries (#159):** Deferred; not in scope for this doc.
