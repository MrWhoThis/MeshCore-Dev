# Flashing Firmware

The MeshCore Flasher at **[meshcore.io/flasher](https://meshcore.io/flasher)** is the easiest way to put firmware on a supported device. No software installation required — it runs entirely in the browser.

## Prerequisites

- A Chromium-based browser: **Chrome**, **Edge**, or **Brave** (Firefox does not support Web Serial)
- A USB cable (data-capable, not charge-only)
- A [supported device](../getting-started/hardware.md)

## Choosing a firmware type

| Firmware | What it does | Who needs it |
|----------|-------------|--------------|
| **Companion Radio** | Connects to a mobile or web app via BLE, USB, or Wi-Fi. Sends and receives messages on your behalf. | Most users — this is the standard choice |
| **Repeater** | Relays packets to extend network range. No user interface. Runs headless. | Network operators placing coverage nodes at high points |
| **Room Server** | Hosts a shared message board that anyone on the mesh can post to. | Groups wanting a common channel |

## Flashing steps

1. Go to [meshcore.io/flasher](https://meshcore.io/flasher)
2. Connect your device via USB
3. Click **Connect** — the browser will show a list of serial ports
4. Select the port that corresponds to your device (typically `USB Serial` or `CP210x`)
5. Select your **Device** from the dropdown
6. Select the **Firmware Type** (Companion, Repeater, or Room Server)
7. Click **Flash**
8. Wait for the progress bar to complete — do not disconnect the device

## Troubleshooting

**Device not listed in the port picker**

- Try a different USB cable (many cables are charge-only)
- Install the CP210x or CH340 driver for your OS if prompted
- On Linux, add your user to the `dialout` group: `sudo usermod -a -G dialout $USER`, then log out and back in

**Flash fails partway through**

- Hold the BOOT button on the device while clicking Flash to put it in bootloader mode
- Try a shorter USB cable or a direct port on the computer (not a hub)

**Device unresponsive after flashing**

- Press the RST or RESET button to reboot the device
- If still unresponsive, try flashing again

## Manual flashing (advanced)

If your device is not in the flasher, you can compile and flash manually:

```bash
git clone https://github.com/meshcore-dev/MeshCore.git
cd MeshCore
pio run -e <your_environment> --target upload
```

See [Getting Started for Developers](../getting-started/developers.md) and [Hardware & Devices](../getting-started/hardware.md) for environment names.
