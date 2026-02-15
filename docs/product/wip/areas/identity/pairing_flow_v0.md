# Identity — Pairing flow v0 (Policy WIP)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue Ref:** [#182](https://github.com/AlexanderTsarkov/naviga-app/issues/182)

This doc defines **Identity & pairing flow v0**: node identity (MCU-anchored), first-time connect (sticker + BLE list), NFC/RFID wake to target a specific node, UX rules for “many nodes nearby” and “switch while connected,” and optional human label. No cryptographic secure claim/provisioning in v0; no BLE/NFC implementation; no code changes. Pairing UX only; JOIN/Mesh protocol is out of scope.

---

## 1) Purpose / scope

- **Purpose:** Define how the **app** discovers and connects to a **node** for the first time (no NFC, or with NFC wake), how the user picks one node among many, and when the app must ask for confirmation (e.g. switch from node A to node B). Identity is anchored to the MCU; display uses a human-friendly short id and optional human label.
- **Scope (v0):** Definitions (node_id_long, node_id_short, hw_profile_id, human label); minimal BLE discovery payload requirements for **matching**; UX decision trees and rules; collision handling. Out of scope: secure claim protocol, BLE/NFC implementation detail, Mesh/JOIN.

---

## 2) Definitions and “who vs what” boundary

| Term | Meaning |
|------|--------|
| **node_id_long** | MCU-anchored identity (e.g. ESP32 MAC-derived uint64). **Who** the node is; immutable; source of truth for pairing and NodeTable. In NodeTable contract this is **DeviceId**. |
| **node_id_short** | Human-friendly display id derived from node_id_long (e.g. CRC16, 4 hex digits). **Who** in the UI when long id is not shown. May collide; disambiguation via suffix (see §6). In NodeTable contract this is **ShortId**. |
| **hw_profile_id** | **What** hardware the node is (board + radio/sensors, etc.). From [HW registry](../hardware/registry_hw_capabilities_v0.md) ([#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159)). Used for capability lookup and UX (e.g. “Collar”, “Dongle”); not identity for pairing. |
| **Human label** | Optional display name: **networkName** (broadcast by node) or **localAlias** (user-set on phone). Shown when available; precedence per NodeTable: networkName > localAlias > node_id_short. |

**Boundary:** **Who** = node_id_long / node_id_short (identity for connection and NodeTable). **What** = hw_profile_id (capabilities, device type). **Label** = human-facing name when present; does not replace identity.

---

## 3) Minimal BLE discovery payload (for matching)

To **match** a BLE advertiser to a known node or to a sticker/NFC target, the app needs at least one of the following in the discovery payload (exact format and encoding are implementation-defined; this is the **minimal requirement** for UX):

- **node_id_long** (or a compact representation that maps 1:1 to it), so the app can:
  - Show “this is node X” in the list, and
  - Resolve “connect to node_id_long L” (e.g. from NFC) to the correct BLE device when several are in range.
- **node_id_short** alone is **not** sufficient for matching when collisions exist; if only short is in the payload, matching is best-effort and the app should disambiguate (e.g. show long id or ask user to choose) when multiple devices share the same short id.

No commitment in this doc to specific BLE advertising structure or NFC payload; only that **matching** requires at least node_id_long (or equivalent) for reliable “connect to this node.”

---

## 4) UX decision trees

### 4.1) First-time connect (no NFC)

- User has a **sticker** (or printed) with node_id_short (and optionally node_id_long or QR).
- User opens “Connect” in the app; app shows a **BLE list** of nearby devices.
- **Rule:** List entries should show node_id_short (and human label when available) so the user can match the list to the sticker. If multiple devices share the same node_id_short, show disambiguation (e.g. suffix or long id).
- User selects a device from the list → app connects to that node. No NFC involved.

### 4.2) NFC/RFID wake connect

- User taps phone (NFC) or brings phone near the node (RFID wake). The **NFC payload** (or wake response) provides the **target node_id** (long or short; long preferred for uniqueness).
- **Rule:** App learns target node_id → filters or highlights the corresponding BLE device when several are in range → user can “Connect to this node” with one action. If only short id is available and there is a collision, app must disambiguate (e.g. show list with long id or ask user to choose).
- **Rule:** After NFC, app should connect to the **exact** node indicated (when matching is unambiguous); no requirement to show a list when target is unique.

### 4.3) Many nodes nearby (e.g. hunting)

- Many nodes in BLE range; list is long.
- **Rule:** App shows a manageable list (e.g. by RSSI, or “recent,” or user search). Each entry: node_id_short + human label when available; collision disambiguation as above.
- **Rule:** NFC wake is the preferred shortcut to “this node” when the user can tap the specific device; otherwise user picks from the list using sticker/label.

### 4.4) Active connection: switch to another node (NFC targets B while connected to A)

- User is **already connected** to node **A**. User performs NFC wake (or similar) that targets node **B**.
- **Rule:** App **must not** silently switch from A to B. App must **require user confirmation** (e.g. “You are connected to [A]. Connect to [B] instead?”). On confirm → disconnect A (or hand off) and connect to B. On cancel → stay connected to A.

---

## 5) Optional human label

- When the node broadcasts **networkName** or the user has set **localAlias** for that node_id_long, the app should **display** that label (with precedence networkName > localAlias > node_id_short) in lists and connection screens.
- Label is **optional**; if absent, show node_id_short (and disambiguation suffix if collision). Label does not replace node_id_long for matching or NodeTable identity.

---

## 6) Collision handling (short id)

- **node_id_short** can collide across nodes (same CRC16 or equivalent).
- **Rule:** In BLE list and connection UX, when two or more devices share the same node_id_short, the app must **disambiguate**: e.g. show “6DA2:1”, “6DA2:2”, or expand to show **node_id_long** (full or truncated) so the user can choose the correct node. DeviceId / node_id_long remains the source of truth; short id is for convenience only.

---

## 7) Sequence diagrams (ASCII)

### 7.1) First-time connect (no NFC): sticker + BLE list

```
User                App                     BLE / Node
  |                   |                         |
  |  Open "Connect"    |                         |
  |------------------->|                         |
  |                   |  Start scan              |
  |                   |------------------------->|
  |                   |  Adverts (node_id_long/short, optional label)
  |                   |<-------------------------|
  |                   |  Build list (short + label, disambiguate if collision)
  |  Show list        |                         |
  |<------------------|                         |
  |  Select device (match sticker)
  |------------------->|                         |
  |                   |  Connect to node         |
  |                   |------------------------->|
  |                   |  Connected              |
  |<------------------|                         |
```

### 7.2) NFC wake → connect to exact node

```
User                App                     NFC / Node
  |                   |                         |
  |  Tap NFC (target node_id from sticker/tag)
  |------------------->|                         |
  |                   |  Read target node_id (long or short)
  |                   |  Start BLE scan         |
  |                   |------------------------->|
  |                   |  Match advertiser with target node_id
  |                   |  If unique match: offer "Connect to [label/short]"
  |  Show "Connect to [node]?"                   |
  |<------------------|                         |
  |  Confirm          |                         |
  |------------------->|  Connect                |
  |                   |------------------------->|
  |  Connected        |                         |
  |<------------------|                         |
```

### 7.3) Switch while connected: NFC targets B, currently connected to A

```
User                App                     Node A    Node B
  |                   |                         |          |
  |  (Connected to A) |                         |          |
  |                   |<========================|          |
  |  Tap NFC → target B                        |          |
  |------------------->|                         |          |
  |                   |  Resolve target = B     |          |
  |                   |  "Currently connected to A. Switch to B?"
  |  Dialog: Switch?  |                         |          |
  |<------------------|                         |          |
  |  Confirm          |                         |          |
  |------------------->|  Disconnect A           |          |
  |                   |------------------------>|          |
  |                   |  Connect to B            |          |
  |                   |------------------------------------>|
  |  Connected to B   |                         |          |
  |<------------------|                         |          |
```

---

## 8) Non-goals (v0)

- No cryptographic secure claim or provisioning protocol; no anti-spoofing design.
- No implementation of BLE advertising payload format or NFC encoding in this doc.
- No JOIN/Mesh protocol; pairing UX only.
- No code changes; doc is product/UX policy only.

---

## 9) Open questions / follow-ups

- **Secure claim / provisioning:** Future protocol so that only the “owner” can bind a node to an account or enforce ownership; out of scope for v0.
- **Privacy hardening:** What identity (long/short) is exposed in BLE/NFC in which contexts; reduce leakage in public settings.
- **Spoofing:** Risk that a nearby device advertises the same node_id_long; v0 does not define mitigation; follow-up for secure binding or attestation.

---

## 10) Related

- **NodeTable contract (Identity):** [../nodetable/index.md](../nodetable/index.md) — [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) (DeviceId, ShortId, networkName, localAlias).
- **HW Capabilities registry:** [../hardware/registry_hw_capabilities_v0.md](../hardware/registry_hw_capabilities_v0.md) — [#159](https://github.com/AlexanderTsarkov/naviga-app/issues/159) (hw_profile_id, hw_type).
