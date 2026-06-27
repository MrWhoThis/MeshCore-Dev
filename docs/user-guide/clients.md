# Connecting a Client

After flashing Companion firmware, connect to your device using one of the MeshCore client apps.

## Connection methods

Companion firmware supports three connection methods depending on the device and firmware variant you flashed:

| Method | Range | Best for |
|--------|-------|----------|
| **BLE** (Bluetooth Low Energy) | ~10 m | Mobile apps on Android/iOS |
| **USB** | Direct connection | Web app, desktop, always-available connection |
| **Wi-Fi** | Same network | Web app, headless operation |

---

## Web App

**URL:** [app.meshcore.nz](https://app.meshcore.nz)

- Runs in the browser — no installation
- Supports BLE, USB, and Wi-Fi connections
- Requires Chrome, Edge, or Brave (for BLE/USB support)

**To connect:**

1. Open [app.meshcore.nz](https://app.meshcore.nz)
2. Click **Connect**
3. Choose your connection type (BLE / USB / Wi-Fi)
4. Select your device from the list

---

## Android

**[Download on Google Play](https://play.google.com/store/apps/details?id=com.liamcottle.meshcore.android)**

- Supports BLE, USB (OTG), and Wi-Fi connections
- Full messaging, map, and contact management

**To connect:**

1. Install the app
2. Grant Bluetooth and Location permissions when prompted
3. Tap **Add Device** and choose your connection type
4. Select your device from the scan results

---

## iOS

**[Download on the App Store](https://apps.apple.com/us/app/meshcore/id6742354151)**

- Supports BLE connections
- Grant Bluetooth permission when prompted

---

## Node.js (meshcore.js)

**[github.com/liamcottle/meshcore.js](https://github.com/liamcottle/meshcore.js)**

A Node.js library and CLI for communicating with MeshCore companion radios. Suitable for automation, scripting, and building custom integrations.

```bash
npm install meshcore.js
```

---

## Python CLI

**[github.com/fdlamotte/meshcore-cli](https://github.com/fdlamotte/meshcore-cli)**

A command-line client for interacting with MeshCore over serial/USB.

```bash
pip install meshcore-cli
```

---

## Repeater & Room Server configuration

Repeaters and Room Servers do not have a companion app — they are managed via:

- **[config.meshcore.io](https://config.meshcore.io)** — browser-based configuration tool (USB)
- The **Remote Management** feature in the mobile app (over LoRa)

See [Repeaters & Room Servers](servers.md) for setup details.
