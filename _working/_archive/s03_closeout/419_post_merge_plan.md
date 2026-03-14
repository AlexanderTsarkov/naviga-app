# #419 post-merge plan

## After PR #431 is merged

1. **Close #419**  
   Comment (optional): *Closed by #431. Canonical field map, BLE alignment, snapshot v4, and master table docs are in place. Remaining: #420 / #421 / #422.*

2. **Progress note for #416 (umbrella)**  
   Add a short comment to issue #416, for example:

   ---
   **#419 done.** NodeTable canonical field map alignment merged via PR #431: master field table doc, `node_name` canonical field, BLE removal of `last_seq` (snr_last at offset 24), snapshot v4 with v3/v4 restore compatibility, legacy ref fields runtime-local only. Next execution: #420 (TX rules) / #421 (RX semantics) / #422 (packetization).
   ---

No code or doc changes required at merge time; only close #419 and update #416 as above.
