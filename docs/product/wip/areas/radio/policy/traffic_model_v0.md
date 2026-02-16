# Radio — Traffic model v0 (Policy)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy defines the **v0 traffic model**: which packet classes are allowed on OOTB public vs Session/Group channels, delivery expectations, and frame limits. Semantic truth is this policy; channel modes are defined here, not by UI or product copy.

---

## 1) Scope guardrails

- **In scope:** Packet class allow/disallow per channel mode; delivery expectations (periodic vs non-periodic, loss tolerance); product-level frame limit and ban on fragmentation.
- **Out of scope:** Encryption, access control, addressability; JOIN/session master protocol; backend/statistics; implementation.

---

## 2) Packet classes (v0)

| Class | Description | Periodicity |
|-------|-------------|-------------|
| **BeaconCore** | Core beacon (identity + minimal telemetry); see [beacon_payload_encoding_v0](../../nodetable/contract/beacon_payload_encoding_v0.md). | Periodic; delivery expectation below. |
| **BeaconTail** | Optional extra beacon content (e.g. infrequent telemetry); same payload constraints as encoding contract. | Periodic but rare. |
| **UserMessage** | User-originated message (content format out of scope here). | Non-periodic. |
| **GeoObject** | Geo object / waypoint (format out of scope). | Non-periodic. |
| **GeoObjectActivation** | Activation/ack for geo object. | Non-periodic. |

---

## 3) OOTB public channel

**Definition:** “Out of the box” public channel — default channel mode before the user selects a non-public or session/group context. Policy rule: tracker + “who is around” only; no user comms on public by default.

### 3.1 Allowed packet classes

| Class | Delivery expectation |
|------|----------------------|
| **BeaconCore** | Periodic; **MUST** be delivered (required for presence and NodeTable updates). |
| **BeaconTail** | Periodic but rare; **MAY** be lost (best-effort). |

### 3.2 Disallowed packet classes

- **UserMessage**
- **GeoObject**
- **GeoObjectActivation**

**Rationale:** OOTB public is for tracking and presence only. If the user wants messaging or geo/comms, they **MUST** leave the public channel by configuration (e.g. select Session/Group or a non-public channel).

---

## 4) Session/Group channel

**Minimal condition (v0):** User has selected a **non-public** channel **AND** there is at least a **presumed recipient** (e.g. a node marked in NodeTable as “my device” / “friend” / group context). Exact definition of “presumed recipient” is implementation-defined; this policy only requires the condition to be satisfied for the following rules to apply.

### 4.1 Allowed packet classes

- **BeaconCore** / **BeaconTail** (same delivery as above).
- **UserMessage** / **GeoObject** / **GeoObjectActivation** as **non-periodic**, **best-effort**.

### 4.2 Delivery expectations

- **Non-periodic** packets (UserMessage, GeoObject, GeoObjectActivation) are **loss-tolerant**.
- They are **NOT** required to be repeated by default (no mandatory retransmission in v0).
- Addressability and privacy are **future work**; random receivers may hear packets and **MUST** ignore them if not addressee (behavior TBD in a future policy).

---

## 5) Frame limits (v0, no fragmentation)

### 5.1 ProductMaxFrameBytes

- **ProductMaxFrameBytes v0 = 96** (canonical internal max for a single packet/frame at the product layer).
- Any payload that would exceed **96 bytes** (after any product-level framing) **MUST** be:
  - **rejected** (preferred), OR
  - **compressed or trimmed** to fit within the limit.
- **Fragmentation is NOT supported in v0.** A single logical payload **MUST** fit in one frame; there is no multi-frame reassembly.

### 5.2 RadioMaxFrameBytes

- **RadioMaxFrameBytes** is a separate **HW/profile** parameter (e.g. E220 sub-packet options 32 / 64 / 128 / 240).
- **Invariant:** **ProductMaxFrameBytes MUST NOT exceed RadioMaxFrameBytes** for the current profile. This **MUST** be validated at build-time or runtime (configuration error if violated).

### 5.3 Summary

| Concept | Value / rule |
|--------|------------------|
| ProductMaxFrameBytes (v0) | 96 bytes |
| Fragmentation (v0) | **Banned** |
| Payload > 96 B | Reject or compress; no fragment. |
| ProductMaxFrameBytes vs RadioMaxFrameBytes | ProductMaxFrameBytes ≤ RadioMaxFrameBytes (enforced). |

---

## 6) Related

- **Beacon payload encoding:** [../../nodetable/contract/beacon_payload_encoding_v0.md](../../nodetable/contract/beacon_payload_encoding_v0.md) — payload layout only; not traffic policy.
- **NodeTable hub:** [../../nodetable/index.md](../../nodetable/index.md) — “who is around” semantics.
- **RadioProfiles registry:** [../registry_radio_profiles_v0.md](../registry_radio_profiles_v0.md).
