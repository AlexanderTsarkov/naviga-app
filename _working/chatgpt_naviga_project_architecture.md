# Naviga — ChatGPT Project Files: architecture guardrails

Stable invariants to keep in working memory. Do not treat as full product spec; details live in repo (`docs/product/areas/`, `docs/architecture/`).

---

## Embedded-first

- Firmware → radio → domain → BLE bridge → mobile. Mobile consumes the delivered BLE surface; do not back-drive architecture from speculative mobile ideas.

---

## Wiring and composition

- **M1Runtime** is the **single** wiring/composition point (radio ↔ domain ↔ BLE transport). No second composition root.
- **Domain** (NodeTable, BeaconLogic) stays **radio-agnostic**. No modem/radio types in domain APIs.

---

## Radio

- **E220/E22** is used as a **UART modem**. Adapter implements `IRadio`; it is not a chip-level radio stack. SPI driver out of scope unless explicitly planned. Real CAD/LBT out of scope unless explicitly planned.

---

## BLE

- **Legacy BLE service** remains **disabled** unless explicitly reintroduced by plan. New BLE contract and implementation are canonical.

---

## Product ordering

- **OOTB / base product first.** JOIN, Mesh, and higher scenario layers only when explicitly in scope. Avoid premature deep work on JOIN/Mesh.

---

## Mobile

- Mobile consumes the **delivered** BLE surface (contract in repo). No architecture changes to satisfy speculative mobile features; extend contract and implementation when needed, in plan.
