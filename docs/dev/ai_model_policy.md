# AI model policy — invariants and review blockers

Canonical rule for product/domain/spec work:

**OOTB is not product truth.** Domain, policy, and spec docs must not depend on OOTB or UI as normative source. OOTB and UI may be referenced as examples or defaults only when explicitly marked non-normative.

## Review blocker

Reviews **MUST** block merge if product, domain, or spec documentation treats OOTB or UI as the normative source of truth (e.g. defining staleness, retention, or precedence by OOTB/UI thresholds without deferring the boundary to policy or consumer).
