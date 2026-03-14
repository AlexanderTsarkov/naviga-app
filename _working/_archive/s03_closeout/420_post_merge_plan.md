# #420 post-merge plan

## After PR #432 is merged

1. **Close #420**  
   Optional comment: *Closed by #432. TX formation v0.1 canon-aligned; code/comments mirror packet_context_tx_rules_v0 §1; cadence-gate test added. Remaining: #421 (RX semantics), #422 (packetization / v0.2).*

2. **Progress note for #416 (umbrella)**  
   Add a short comment to issue #416, for example:

   ---
   **#420 done.** TX formation v0.1 canon alignment merged via PR #432: formation rules explicit in beacon_logic (trigger/earliest/deadline/coalesce per §1 table), one new cadence-gate test, policy doc implementation reference. hw_profile_id/fw_version_id unchanged. Next: #421 (RX semantics) / #422 (packetization).
   ---

No code or doc changes at merge time; only close #420 and update #416 as above.
