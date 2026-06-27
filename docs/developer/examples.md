# Example Applications

MeshCore ships with six reference implementations in the `examples/` directory. Each demonstrates a different use case and can be built and run on supported hardware.

---

## companion_radio

**Path:** `examples/companion_radio/`

The most feature-rich example. Implements a full companion radio that bridges the LoRa mesh to a connected device (phone, PC) over BLE, USB serial, or Wi-Fi.

**What it does:**
- Sends and receives direct messages, group messages, and adverts on the mesh
- Exposes the [Companion Protocol](../companion_protocol.md) over BLE/USB/Wi-Fi for client apps
- Supports the MeshCore web app, Android app, and iOS app

**When to use it:**
- This is the standard firmware for "user-facing" devices — the radio you carry and connect your phone to

**Hardware:**
- Any supported device with BLE (NRF52, ESP32) or Wi-Fi (ESP32)
- Optional display (OLED/e-ink) for UI variants

**Flashing:** Available directly from [meshcore.io/flasher](https://meshcore.io/flasher) as **Companion**

---

## simple_repeater

**Path:** `examples/simple_repeater/`

A relay node. Listens for packets and re-transmits them to extend network coverage.

**What it does:**
- Forwards flood (broadcast) packets to extend their reach
- Optionally displays nearby nodes and signal info on a connected display
- Supports Access Control Lists (ACLs) to restrict which nodes it relays
- Supports Remote Management over LoRa

**When to use it:**
- Place at a high point (rooftop, hilltop) to bridge distant parts of the mesh
- No user interaction required after setup

**Flashing:** Available from [meshcore.io/flasher](https://meshcore.io/flasher) as **Repeater**

---

## simple_room_server

**Path:** `examples/simple_room_server/`

A shared bulletin board. Any node on the mesh can post messages to a room server and read posts from it.

**What it does:**
- Stores incoming posts in flash memory
- Delivers posts to connecting clients
- Supports a join password to restrict membership
- Supports Remote Management over LoRa

**When to use it:**
- When a group wants a shared channel without a peer-to-peer relationship

**Flashing:** Available from [meshcore.io/flasher](https://meshcore.io/flasher) as **Room Server**

---

## simple_secure_chat

**Path:** `examples/simple_secure_chat/`

Encrypted terminal chat. Two devices running this firmware can exchange encrypted direct messages over serial/USB — no app required.

**What it does:**
- Establishes an encrypted channel between two paired nodes
- Interacts via the Arduino Serial Monitor or any serial terminal (115200 baud)
- Uses Ed25519 for identity and AES-128 for message encryption

**When to use it:**
- Development and testing of the direct messaging path
- Minimal hardware builds with no display or app

**To interact:**
```
pio device monitor  # opens serial terminal at 115200 baud
```

---

## kiss_modem

**Path:** `examples/kiss_modem/`

A KISS protocol bridge. Allows a host computer or application to send and receive raw packets over the LoRa radio via the standard KISS protocol over serial/USB.

**What it does:**
- Exposes a KISS TNCP endpoint over USB serial
- Lets host applications (e.g. amateur radio software, custom scripts) control the radio directly

**When to use it:**
- Integration with APRS or AX.25 applications
- Building custom host-side software that needs raw radio access

**Protocol details:** [KISS Modem Protocol](../kiss_modem_protocol.md)

---

## simple_sensor

**Path:** `examples/simple_sensor/`

A remote telemetry node. Reads sensor data and transmits it over the mesh.

**What it does:**
- Reads environmental sensor data (temperature, humidity, etc.)
- Sends periodic telemetry payloads using the [CayenneLPP](https://developers.mydevices.com/cayenne/docs/lora/) encoding
- Supports alert thresholds — sends an immediate alert packet when a value crosses a threshold
- Supports ACLs to restrict which nodes can receive its data

**When to use it:**
- Remote monitoring: weather stations, soil sensors, asset trackers
- Any application that needs to relay sensor readings over LoRa

---

## Building an example

```bash
# clone the repo
git clone https://github.com/meshcore-dev/MeshCore.git
cd MeshCore

# build and upload the companion_radio example for a Heltec V3
pio run -e heltec_v3 --project-dir examples/companion_radio --target upload
```

See [Getting Started for Developers](../getting-started/developers.md) for the full setup walkthrough.
