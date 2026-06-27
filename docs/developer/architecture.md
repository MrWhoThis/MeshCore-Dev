# Architecture Overview

MeshCore is organized as a layered class hierarchy. Each layer handles one concern, and application code sits on top.

```
┌─────────────────────────────────────┐
│         Application Layer           │  (your code / examples)
├─────────────────────────────────────┤
│              Mesh                   │  multi-hop routing, packet forwarding
├─────────────────────────────────────┤
│           Dispatcher                │  radio I/O, timing, packet queuing
├─────────────────────────────────────┤
│       Radio / Board HAL             │  hardware abstraction (RadioLib wrappers)
└─────────────────────────────────────┘
```

---

## Radio / Board HAL

**Files:** `src/helpers/radiolib/`, `src/helpers/ESP32Board.h`, `src/helpers/NRF52Board.h`

The `Radio` interface (`src/Dispatcher.h`) abstracts the physical radio. Implementations wrap [RadioLib](https://github.com/jgromes/RadioLib) and handle:

- Starting and checking send/receive operations (`startSendRaw`, `recvRaw`, `isSendComplete`)
- Airtime estimation (`getEstAirtimeFor`)
- RSSI/SNR reporting
- Channel Activity Detection (CAD) to avoid collisions

Board-level helpers provide clock, flash storage, display, and GPIO abstractions that keep the core code hardware-agnostic.

---

## Dispatcher

**Files:** `src/Dispatcher.h`, `src/Dispatcher.cpp`

`Dispatcher` drives the main radio loop. It:

- Polls the `Radio` for incoming packets
- Manages a send queue with timing constraints (airtime delays, CAD)
- Calls virtual `onRecvPacket()` when a packet arrives
- Enforces duty-cycle limits

Key virtual methods your application can override:

| Method | Purpose |
|--------|---------|
| `onRecvPacket(Packet*)` | Called for every received packet before routing |
| `getCADFailRetryDelay()` | How long to wait before retrying a blocked transmission |

---

## Mesh

**Files:** `src/Mesh.h`, `src/Mesh.cpp`

`Mesh` extends `Dispatcher` and implements multi-hop routing:

- **Packet deduplication** via `MeshTables` — tracks seen packet hashes to prevent loops
- **Flood forwarding** — packets without a specific destination are re-broadcast with decremented hop count
- **Path-directed forwarding** — packets with a path hash are forwarded only toward the destination
- **ACK routing** — delivery acknowledgements are routed back along the reverse path
- **Multipart reassembly** — large messages split across multiple packets are reassembled

Key virtual methods to override in your application:

| Method | Purpose |
|--------|---------|
| `filterRecvFloodPacket(Packet*)` | Return `true` to drop a flood packet before processing |
| `allowPacketForward(Packet*)` | Return `false` to suppress forwarding of a packet |
| `getFloodRetxDelay(Packet*)` | Random delay before re-transmitting a flood packet (reduces collisions) |
| `onRecvFlood(Packet*)` | Handle an incoming broadcast/flood packet |
| `onRecvDirect(Packet*)` | Handle an incoming directed packet addressed to this node |
| `onAckRecv(Packet*)` | Handle a delivery acknowledgement |

---

## Packet

**Files:** `src/Packet.h`, `src/Packet.cpp`

`Packet` is a fixed-size struct (no heap allocation) that holds the raw bytes of one over-the-air packet plus metadata (RSSI, SNR, timestamps). Key fields:

- `payload_type` — discriminates between flood, direct, ACK, advert, and sensor payloads
- `path_hash` — 1, 2, or 3-byte routing hash identifying the destination or path
- `hop_count` — decremented at each hop; packet is dropped when it reaches 0
- Encrypted payload body (AES-128 CTR for direct packets, unauthenticated for flood)

Packet format details: [Packet Format](../packet_format.md)

---

## Identity

**Files:** `src/Identity.h`, `src/Identity.cpp`

Each node has an Ed25519 keypair as its identity:

- **`Identity`** — holds only the public key; used to verify signatures and derive shared secrets
- **`LocalIdentity`** — holds both public and private keys; used to sign packets and perform ECDH

Key derivation for direct messages uses X25519 ECDH to produce a per-link symmetric key. Flood packets are not individually encrypted but carry an HMAC-style signature to authenticate the sender.

---

## GroupChannel

**File:** `src/Mesh.h`

A `GroupChannel` represents a shared channel key (derived from a passphrase). Nodes that know the channel secret can send and receive group messages encrypted under that key.

---

## Application layer

Your application subclasses `Mesh` and overrides the virtual methods above to implement behaviour. The six `examples/` demonstrate this pattern — see [Example Applications](examples.md).

**Minimal application skeleton:**

```cpp
#include <Mesh.h>

class MyApp : public mesh::Mesh {
protected:
  bool onRecvFlood(mesh::Packet* pkt) override {
    // handle incoming broadcast
    return false; // false = don't suppress further processing
  }

  bool onRecvDirect(mesh::Packet* pkt) override {
    // handle incoming direct message addressed to us
    return false;
  }
};
```

---

## Memory model

MeshCore avoids heap allocation after startup (`new`/`delete` are not used in the routing path). Packets are managed via `StaticPoolPacketManager` (`src/helpers/StaticPoolPacketManager.h`), which keeps a fixed-size pool of `Packet` objects. This makes behaviour deterministic and avoids fragmentation on microcontrollers.
