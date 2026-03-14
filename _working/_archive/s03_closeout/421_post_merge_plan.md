# #421 post-merge plan

## After PR #433 is merged

1. **Close #421**  
   Optional comment: *Closed by #433. RX/apply semantics v0.1 canon-aligned in code and docs; ref fields documented as runtime-local only (not BLE, not persisted). Remaining: #422 (packetization).*

2. **Progress note for #416 (umbrella)**  
   Add a short comment to issue #416, for example:

   ---
   **#421 done.** RX semantics v0.1 canon alignment merged via PR #433: node_table comments and rx_semantics_v0 §6 implementation reference; legacy ref fields explicit as runtime-local only. No behavior change. Next: #422 (packetization).
   ---

No code or doc changes at merge time; only close #421 and update #416 as above.
