# Research: Meshtastic naming / identity propagation patterns

**Purpose:** Extract patterns applicable to Naviga’s future **session/group identity exchange** ([#194](https://github.com/AlexanderTsarkov/naviga-app/issues/194)). Focus on naming mechanics, payload sizing, cadence/rate limiting, caching and UX reconciliation. No protocol copy; no security review.

**Related:** Umbrella [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147); Naviga v0 constraint [NodeTable index §4](../areas/nodetable/index.md) (no names over radio in v0); [traffic_model_v0](../areas/radio/policy/traffic_model_v0.md).

---

## 1) Message types and fields that carry names/identity

| Layer | Message / port | Fields for naming / owner | Notes |
|-------|----------------|---------------------------|--------|
| **Protobuf** | `User` (inside NodeInfo payload) | `id` (string, globally unique e.g. !&lt;8 hex bytes&gt;), `long_name` (string), `short_name` (string) | Carried in **NODEINFO_APP** (port 4). Also: `hw_model`, `role`, `public_key`, `is_licensed`, `is_unmessagable`. |
| **Firmware** | NODEINFO_APP (port 4) | Full `User` serialized in Data.payload | NodeInfoModule sends/receives; NodeDB updated via `updateUser(from, p, channel)`. |
| **On-the-air** | Same as beacon-like mesh packet | No separate “name packet”; User is a **single** protobuf payload per packet | One packet = one full User (no chunking). |

**Takeaway:** Meshtastic uses a **single-packet User** (long_name + short_name + id + role + hw_model, etc.) on a dedicated app port. Names are **not** in the beacon; they are in a separate port (NODEINFO_APP). So “identity exchange” is already a **separate packet class** from position/telemetry—aligns with Naviga’s choice to keep names out of Beacon Core/Tail and define session-only identity exchange.

---

## 2) Size limits and encoding

| Aspect | Meshtastic | Source / caveat |
|--------|------------|------------------|
| **Encoding** | UTF-8 strings in protobuf (`string` type) | [mesh.proto](https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto) — no explicit max length in User. |
| **short_name** | Proto comment: “VERY short name, ideally two characters” for tiny OLED | No enforced byte/code-point limit in proto. |
| **long_name** | No defined max in User message | Known issue: [very long long_name / control chars](https://github.com/meshtastic/firmware/issues/4182) caused display and stability problems; validation was weak. |
| **Other messages** | Waypoint: `name` max 30 chars, `description` max 100 chars | Shows Meshtastic does use explicit limits elsewhere when needed. |

**Takeaway:** **Unbounded strings for names** in the main User message led to real-world bugs (control characters, oversized names). For Naviga session identity exchange: **define explicit max length (bytes/code points) and character/encoding rules** (e.g. UTF-8 with allowed subset or table-coded alphabet). Consider **chunking** (e.g. NameChunk in [#194](https://github.com/AlexanderTsarkov/naviga-app/issues/194)) so long names don’t require a single oversized packet.

---

## 3) Cadence and rate: when names are sent / requested / repeated

| Trigger | Behavior | Source |
|---------|----------|--------|
| **Node startup** | Node **broadcasts** its User; “normal flow is for all other nodes to **reply with their User**” so the new node builds its NodeDB | mesh.proto User message comment. |
| **Collision (same node number)** | If a node receives a User where sender node number **equals our** node number: MAC comparison; lower MAC keeps nodenum and rebroadcasts own User; higher MAC picks new nodenum and rebroadcasts | mesh.proto User comment (collision algorithm). |
| **Periodic** | “Node Info Broadcast Seconds” — configurable device setting (admin message Config.Device); controls how often node info is broadcast | Device config; Neighbor Info has 4–6 h default; Node Info interval is separate. |
| **On change** | Not clearly documented; apps report name updates not reflecting without restart ([#6050](https://github.com/meshtastic/firmware/issues/6050)) | Suggests no robust “on name change” broadcast or cache invalidation. |

**Takeaway:** **Bootstrap + reply storm** (new node broadcasts, everyone replies) is a **traffic spike** and can be abusive on public/OOTB if names are large or many nodes reply. For Naviga: **session-only** identity exchange avoids OOTB; **rate limits** and **join burst strategy** (e.g. one short burst for late joiners, then only on change) are essential. **Epoch or version** to invalidate stale caches is not clearly present in Meshtastic and would help (e.g. NameChunk epoch in [#194](https://github.com/AlexanderTsarkov/naviga-app/issues/194)).

---

## 4) App-side: caching, collisions, unknown nodes, stale names

| Topic | Meshtastic behavior / issues | Naviga relevance |
|-------|------------------------------|-------------------|
| **Caching** | NodeDB (firmware) and app DB cache User; name/role per node. [Cache node names on app](https://github.com/meshtastic/Meshtastic-Android/issues/1115); [name updates don’t reflect without restart](https://github.com/meshtastic/firmware/issues/6050) | Need **explicit invalidation** (epoch, version, or “name changed” signal) and **UI refresh path**. |
| **Collision (same display name)** | No protocol-level disambiguation; node identity is node number + User. [Bug: wrong name/device info shown for node](https://github.com/meshtastic/firmware/issues/2754) (mapping mix-up) | Define **display rule**: e.g. ShortId suffix when two nodes have same name, or “unverified name” UX. |
| **Unknown nodes** | New nodes appear when they send (any packet updates last_heard; NODEINFO_APP updates User). No explicit “request User” in standard flow except reply-to-broadcast | Late joiners: Meshtastic relies on “everyone reply with User” to new node; **request/response** or **targeted burst** could reduce spam. |
| **Stale names** | No epoch/version on User; stale name can persist until next broadcast or restart | **Epoch or sequence** in Naviga NameChunk (or equivalent) to reset receiver buffer and mark “new version”. |
| **Control characters / long names** | [Bug: very long long_name, newlines](https://github.com/meshtastic/firmware/issues/4182); [label formatting](https://github.com/meshtastic/Meshtastic-Android/issues/1652) | **Validate and bound** names (max length, allowed charset); sanitize for display. |

**Takeaway:** **Reusable patterns:** (1) Cache by (nodeId, epoch/version); (2) clear “stale” rule (epoch reset); (3) display rule for duplicate names (e.g. ShortId suffix); (4) “unverified name” UX when name came over radio only. **Avoid:** relying on “everyone reply” without rate limits; unbounded strings; no cache invalidation.

---

## 5) Patterns that map to Naviga (session/group vs public)

| Pattern | Meshtastic | Map to Naviga [#194](https://github.com/AlexanderTsarkov/naviga-app/issues/194) |
|---------|------------|-------------------------------------------------------------------------------|
| **Separate packet class for identity** | NODEINFO_APP (port 4), not in beacon | ✅ Session-only identity packet (NameChunk or similar); no names in Beacon Core/Tail or OOTB. |
| **Bounded name length** | Lacking in User; Waypoint uses 30/100 chars | ✅ Define max bytes/code points and encoding (UTF-8 or table-coded); document in session_identity_exchange_v0_1. |
| **Chunking for long names** | Single User packet; no chunking | ✅ NameChunk (nodeId, epoch, totalLen, chunkIndex, chunkPayload) for loss-tolerant reassembly; epoch resets buffer. |
| **Join / late-joiner strategy** | Broadcast + all reply (high traffic) | ✅ Rate limits + **burst for late joiners** (e.g. one full name send after join, then on change only); avoid “everyone reply” on open channel. |
| **Cache invalidation** | Weak (no version/epoch) | ✅ Epoch (or version) in payload so receiver can reset and avoid stale names. |
| **Display / collision rule** | No standard; bugs on duplicate/wrong mapping | ✅ Document: collision handling (e.g. ShortId suffix) and “unverified name” when name is radio-sourced only. |

---

## 6) Anti-patterns and recommended constraints for Naviga

| Anti-pattern (Meshtastic) | Naviga recommendation |
|---------------------------|------------------------|
| **Names on same channel as beacon / OOTB** | Keep names **session-only**; never in Beacon or OOTB public (already in v0 constraint). |
| **Unbounded name strings** | **Strict max length** (bytes and/or code points); consider table-coded 1 byte/char for RU/EN to save airtime. |
| **Single large packet for long name** | **Chunked** name (NameChunk) with reassembly; loss-tolerant; epoch to reset. |
| **“Everyone reply with User” on join** | **Rate limits**; targeted burst for late joiners; no open flood. |
| **No cache invalidation** | **Epoch or version** in identity payload; receiver resets buffer on new epoch. |
| **No display/collision policy** | Define **collision handling** (e.g. ShortId suffix) and **unverified name** UX in contract. |
| **Control characters / unvalidated input** | **Allowed character set** and sanitization; reject or strip control chars. |

---

## 7) Summary: 3–5 reusable patterns for Naviga

1. **Session-only identity packet** — Do not carry names in beacon or OOTB; use a dedicated session-only packet type (e.g. NameChunk) so traffic model stays clear and public channel is not polluted.
2. **Bounded encoding and length** — Max bytes/code points per name; UTF-8 or lang_ref + table-coded 1 byte/char; supported alphabet list; no unbounded strings.
3. **Chunking + epoch** — Long names as chunks (nodeId, epoch, totalLen, chunkIndex, chunkPayload); receiver caches by (nodeId, epoch); **epoch resets buffer** so stale or changed name is replaced.
4. **Rate limits and join burst** — Limit chunks per node per time window; for late joiners, allow a **single burst** (e.g. full name once) then only on change; avoid “everyone reply” storms.
5. **Cache and UX policy** — Explicit **cache invalidation** (epoch/version); **collision handling** (e.g. display ShortId when duplicate names); **“unverified name”** when name is from radio only (no secure claim).

---

## 8) Sources

| Source | What |
|--------|------|
| [Meshtastic mesh.proto (User, Data, MeshPacket)](https://github.com/meshtastic/protobufs/blob/master/meshtastic/mesh.proto) | User fields (id, long_name, short_name), collision algorithm comment, Waypoint name/description limits. |
| [Meshtastic firmware #4182](https://github.com/meshtastic/firmware/issues/4182) | Very long long_name / control chars bug. |
| [Meshtastic firmware #6050](https://github.com/meshtastic/firmware/issues/6050) | Name updates don’t reflect without app restart. |
| [Meshtastic firmware #2754](https://github.com/meshtastic/firmware/issues/2754) | Wrong node name/device info shown (mapping bug). |
| [Meshtastic Android #1115](https://github.com/meshtastic/Meshtastic-Android/issues/1115) | Cache node names on app. |
| [Meshtastic device config](https://meshtastic.org/docs/configuration/radio/device/) | Node Info Broadcast Seconds. |
| Local research [meshastic-nodetable.md](../areas/nodetable/research/meshastic-nodetable.md) | NodeDB, NODEINFO_APP, update sources, transferability table. |

---

## 9) Follow-up

- **Contract:** [#194](https://github.com/AlexanderTsarkov/naviga-app/issues/194) — `docs/product/wip/areas/session/contract/session_identity_exchange_v0_1.md` to capture NameChunk design, encoding choices (UTF-8 vs table-coded), rate limits, burst strategy, and open questions (size limits, mixed-language, collision, unverified name).
- **Spec map:** Optional — add this research note to WIP index or spec_map “Research / references” so it’s discoverable.
