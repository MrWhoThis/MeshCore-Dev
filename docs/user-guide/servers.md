# Repeaters & Room Servers

Repeaters and Room Servers are infrastructure nodes — they run headlessly and extend or enrich your mesh network.

## Repeaters

A repeater listens for packets and re-transmits them, extending the effective range of the mesh. Place a repeater at a high point (rooftop, hilltop) to bridge areas that would otherwise be out of range of each other.

### Flashing a repeater

1. Go to [meshcore.io/flasher](https://meshcore.io/flasher)
2. Select your device
3. Select **Repeater** as the firmware type
4. Flash and reboot

### Configuring a repeater

Use the browser-based config tool at **[config.meshcore.io](https://config.meshcore.io)**:

1. Connect the repeater via USB
2. Open [config.meshcore.io](https://config.meshcore.io)
3. Click **Connect** and select the serial port
4. Set:
   - **Name** — a display name for the repeater
   - **Location** — GPS coordinates (optional, used for map display)
   - **LoRa frequency and bandwidth** — must match the rest of your mesh
   - **Admin password** — used for remote management

### Remote management

Once a repeater is deployed, you can manage it over LoRa using the mobile app's **Remote Management** feature — no physical access required.

### ACLs (Access Control Lists)

Repeaters support ACLs to control which nodes are allowed to use them. Configure via the config tool or remote management.

---

## Room Servers

A Room Server hosts a shared message channel — think of it as a bulletin board. Any node on the mesh can post to a room server and read its messages.

### Flashing a room server

1. Go to [meshcore.io/flasher](https://meshcore.io/flasher)
2. Select your device
3. Select **Room Server** as the firmware type
4. Flash and reboot

### Configuring a room server

Use [config.meshcore.io](https://config.meshcore.io) — same process as a repeater:

1. Connect via USB
2. Set a **name**, **location**, **LoRa settings**
3. Optionally set a **join password** to restrict who can post

### Joining a room server

In the mobile or web app:

1. Open **Channels** or **Rooms**
2. Tap **Add Room Server**
3. Select the server from the discovered nodes list (it will appear as an advertisement on the mesh)
4. Enter the join password if one is set

---

## Network design tips

- **Coverage**: one well-placed repeater at elevation can cover several kilometers; use the app's **Nodes** map to identify coverage gaps
- **Frequency**: all devices on the same mesh must use the same LoRa frequency, bandwidth, and spreading factor — see [Number Allocations](../number_allocations.md) for regional defaults
- **Hop count**: the default maximum hop count prevents packets from circulating indefinitely; see the [FAQ](../faq.md) for guidance on tuning this
- **Multiple meshes**: Transport Codes allow filtering by zone/region so multiple independent meshes can share the same frequency band without interfering
