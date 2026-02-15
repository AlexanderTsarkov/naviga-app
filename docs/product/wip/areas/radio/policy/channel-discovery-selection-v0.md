# Radio — Channel discovery & selection v0 (Policy WIP)

**Work Area:** Product Specs WIP · **Issue:** [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175)

This doc defines policy and options for **channel discovery and selection**: local-only discovery (no backend) and a future backend-assisted heatmap concept. No code or backend implementation; no normative OOTB/UI coupling.

---

## 1) Scope / Non-goals

- **In scope:** User need to choose channels meaningfully; local-only discovery options (limits); future backend heatmap concept; privacy/consent/aggregation notes.
- **Out of scope:** Backend implementation. Code changes. Normative OOTB/UI. This is a policy/stub for v0; details to be filled per [#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175).

---

## 2) User motivations (privacy vs finding community)

*(Placeholder: balance between privacy (minimal data, opt-in) and finding community / busy channels. To be populated.)*

---

## 3) Local-only discovery options (limits)

*(Placeholder: what the device can do without a backend — e.g. local scan/listen, activity per channel, RSSI; single-device view limits. To be populated.)*

---

## 4) Backend-assisted heatmap concept (future)

*(Placeholder: optional later phase — aggregated, anonymized channel-usage data; opt-in; no implementation here. To be populated.)*

---

## 5) Privacy / consent / aggregation notes

*(Placeholder: aggregated stats only; opt-in for backend contribution; avoid deanonymization; consent and aggregation rules. To be populated.)*

---

## 6) Open questions / follow-ups

*(Placeholder: scan duration, channel list source, UX for “no activity” vs “busy”, backend API shape (future). To be populated.)*

---

## 7) Related

- **Beacon minset & encoding:** [beacon-minset-encoding-v0.md](beacon-minset-encoding-v0.md) — [#173](https://github.com/AlexanderTsarkov/naviga-app/issues/173) (payload/airtime).
- **NodeTable / contract:** [../nodetable/contract/link-telemetry-minset-v0.md](../nodetable/contract/link-telemetry-minset-v0.md) — [#158](https://github.com/AlexanderTsarkov/naviga-app/issues/158).
