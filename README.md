# MeshCore-Dev — Connecticut Mesh fork

> **This is a fork of [MeshCore](https://github.com/meshcore-dev/MeshCore) v1.18.0 with one addition: a forwarding ACL (blocklist) for repeaters, covering both direct messages and group/channel messages.**
>
> For general MeshCore documentation, hardware support, flashing and clients, see the [upstream project](https://github.com/meshcore-dev/MeshCore) and [docs.meshcore.io](https://docs.meshcore.io).

## Why this fork exists

The Connecticut Mesh has been taking floods of junk traffic coming in from nodes in the Monroe area. That traffic eats airtime on every repeater that relays it, and the sheer volume of these floods has been draining the batteries of solar- and battery-powered repeaters: every junk packet a repeater re-transmits costs TX power, and at flood volumes that adds up fast. Stock MeshCore doesn't give a repeater operator a way to stop forwarding a specific sender's traffic.

This fork adds a small, opt-in **forwarding ACL** to the Simple Repeater. Operators can stop their repeater from **re-transmitting** packets that match a source, a destination, and/or the first repeater the packets entered the mesh through. Nothing else changes:

- Packets addressed to the repeater itself are still received and processed normally. Only forwarding is suppressed.
- The ACL is **off by default**. A repeater flashed with this firmware behaves exactly like stock v1.18.0 until an admin adds rules and runs `acl on`, unless the firmware is built with a default rule list (`FWD_ACL_DEFAULTS`, see below).
- Direct-message rules check `REQ`, `RESPONSE`, `TXT_MSG` and `PATH` packets, which carry cleartext source and destination hashes. Group rules (`acl add grp <hop> <chan>`) check channel/group messages by channel hash and first hop, since their sender is encrypted. Adverts, ACKs, trace and similar packets are never blocked.

## Quick start (repeater admin CLI)

```
acl add * 08 *        # stop forwarding anything from source hash 08
acl add c8 * *        # stop forwarding DM-type traffic first relayed by repeater c8
acl add grp c8 11     # stop forwarding channel 11 group messages first relayed by c8
acl on                # enable filtering (rules are saved to flash)
acl list              # show rules with hit counters
acl status            # e.g. "acl on, 2/32 entries, 42 hits"
```

## Group message filter

Much of the flood traffic is group/channel messages. A group message's sender is encrypted, so a repeater can't filter it by source the way it can a DM. Instead, **group rules** match on what *is* visible in the clear: the **channel hash** and the **first repeater** the message entered the mesh through.

```
acl add grp c8 11      # drop channel 11 group messages first relayed by repeater c8
acl add grp c8f2a1 *   # drop all group messages first relayed by repeater c8f2a1
acl add grp 0 11       # drop channel 11 group messages heard straight from a direct neighbour
```

- Group rules apply to both group text and group data packets (`GRP_TXT`, `GRP_DATA`).
- `hop` can be a 1–3 byte repeater hash, `0` (not yet relayed, i.e. sent by a direct neighbour) or `*`. A short hop rule also matches longer path hashes with the same prefix, and a long rule still matches when the sender switches to smaller path hashes, so it can't be dodged that way.
- `chan` is the 1-byte channel hash, or `*` for every channel. Unrelated channels can share a hash, so a channel rule hits all of them.
- `grp * *` (block every group message) is rejected.
- In `acl list` group rules show as `{n}:grp {hop}/#{chan} h={hits}`, e.g. `2:grp c8f2a1/#11 h=40`. DM and group rules share the 32-rule limit.

The full reference is in [docs/cli_commands.md → Forwarding ACL](./docs/cli_commands.md#forwarding-acl-repeater-only). It covers rule syntax, scope, persistence and error messages.

## Building with default rules

To ship repeaters that start filtering straight away, set a rule list in the repeater env's `build_flags`:

```ini
  -D FWD_ACL_DEFAULTS='"c8/08/7d,c8/#11"'
```

DM rules are `hop/src/dst`, group rules are `hop/#chan`. On first boot, and again whenever a firmware with a *different* list is flashed, the list replaces the saved rules and turns the ACL on (or off, if the list is empty). CLI changes made after that persist across reboots. Details, including error handling and downgrades, are in [docs/cli_commands.md → Build-time default rules](./docs/cli_commands.md#build-time-default-rules-fwd_acl_defaults).

### Connecticut Mesh rule set

These are the rules that block the current flood on the Connecticut Mesh. The junk traffic is a back-and-forth between node `08` and nodes `7d` and `11`, entering the mesh through repeater `c8`. Blocking both directions of each conversation at `c8` stops it from being re-flooded across the state and burning airtime and battery on every repeater along the way, while all other traffic through `c8` keeps flowing.

`acl list` on a live repeater running these rules:

```
1:c8/08/7d h=1014
2:c8/08/11 h=106
3:c8/7d/08 h=316
4:c8/11/08 h=0
```

| Rule | Blocks (first relayed by `c8`) |
|---|---|
| `c8/08/7d` | `08` → `7d` |
| `c8/08/11` | `08` → `11` |
| `c8/7d/08` | `7d` → `08` |
| `c8/11/08` | `11` → `08` (covers the return direction) |

To build them in as defaults:

```ini
  -D FWD_ACL_DEFAULTS='"c8/08/7d,c8/08/11,c8/7d/08,c8/11/08"'
```

Or enter them by hand on a running repeater:

```
acl add c8 08 7d
acl add c8 08 11
acl add c8 7d 08
acl add c8 11 08
acl on
```

#### Example offending packet

A captured copy of the flood traffic:

```
0906c8bcd30d12ab7d0886466484caabf4a8007d365ba61a1a9101c45247638baaca6d4b2b7c4e502dcd6919
```

| Bytes | Field | Value |
|---|---|---|
| `09` | Header | Route type `01` = **flood**; payload type `2` = **TXT_MSG** (direct text message); version 1 |
| `06` | Path length | 1-byte hashes, **6 hops** so far |
| `c8 bc d3 0d 12 ab` | Path | First relayed by **`c8`**, then `bc` → `d3` → `0d` → `12` → `ab` |
| `7d` | Destination hash | **`7d`** |
| `08` | Source hash | **`08`** |
| `8646` | MAC | 2-byte authentication code |
| `6484…6919` | Ciphertext | 32 bytes (2 AES blocks): timestamp, flags and a short text |

It is a private message from `08` to `7d` sent as a **flood** instead of along a known route, so every repeater that hears it re-broadcasts it across the whole mesh. This copy had already been relayed by 6 repeaters. Rule `c8/08/7d` matches it: the first hop is `c8`, the source is `08` and the destination is `7d`. The hop rule only looks at the first path entry, so the message is caught however far it has already spread. The message body is encrypted; a repeater only sees the cleartext `08` → `7d` header, which is exactly what DM rules match on.

## What changed vs. upstream v1.18.0

| File | Change |
|---|---|
| `examples/simple_repeater/ForwardACL.{h,cpp}` | Rule matching, CLI parsing, save/load to `/fwd_acl`, `FWD_ACL_DEFAULTS` parsing |
| `examples/simple_repeater/MyMesh.{h,cpp}` | Hooks the ACL into `allowPacketForward()` and the CLI |
| `test/test_forward_acl/` | Native unit tests (`pio test -e native -f test_forward_acl`) |
| `docs/cli_commands.md` | Forwarding ACL documentation |

Companion, room server and sensor firmware are unchanged from upstream.

