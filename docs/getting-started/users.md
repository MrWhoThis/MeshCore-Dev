# Quick Start for Users

Get MeshCore running on a device in a few minutes. No coding or compilation needed.

## What you need

- A supported LoRa device (see [Hardware & Devices](hardware.md))
- A USB cable
- A modern web browser (Chrome or Edge recommended for the flasher)

## Step 1 — Flash the firmware

1. Go to **[meshcore.io/flasher](https://meshcore.io/flasher)**
2. Connect your device via USB
3. Select your device model from the list
4. Choose a firmware type:
   - **Companion Radio** — for use with a mobile or web app over BLE, USB, or Wi-Fi
   - **Repeater** — extends network coverage by relaying messages (no user interface needed)
   - **Room Server** — shared message board that other nodes can post to
5. Click **Flash** and wait for the process to complete

> If your device is not listed in the flasher, check the [Hardware & Devices](hardware.md) page — it may require manual compilation.

## Step 2 — Connect a client app

Once flashing is complete, connect to the device with one of the supported client apps:

| Client | Connection | Link |
|--------|-----------|------|
| Web App | BLE / USB / Wi-Fi | [app.meshcore.nz](https://app.meshcore.nz) |
| Android | BLE / USB / Wi-Fi | [Google Play](https://play.google.com/store/apps/details?id=com.liamcottle.meshcore.android) |
| iOS | BLE | [App Store](https://apps.apple.com/us/app/meshcore/id6742354151) |

Open the app, scan for nearby devices, and select yours.

## Step 3 — Send a message

Once connected, you can:

- Send direct messages to other MeshCore users
- Post to room servers
- View nearby nodes on the map (if GPS is available)

## Next steps

- **Add a repeater** — place a second device running Repeater firmware at a high point to extend range
- **Set up a room server** — gives your group a shared message board
- Read the [FAQ](../faq.md) for common setup questions
- Join the [MeshCore Discord](https://meshcore.gg) for community help

## Video guides

- [MeshCore QuickStart Playlist](https://www.youtube.com/watch?v=iaFltojJrAc&list=PLshzThxhw4O4WU_iZo3NmNZOv6KMrUuF9) by The Comms Channel
- [MeshCore Technical Presentation](https://www.youtube.com/watch?v=OwmkVkZQTf4) by Liam Cottle
