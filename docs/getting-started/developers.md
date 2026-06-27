# Quick Start for Developers

Build and run a MeshCore example application on real hardware.

## Prerequisites

- [Visual Studio Code](https://code.visualstudio.com)
- [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) for VS Code
- A supported LoRa device (see [Hardware & Devices](hardware.md))
- A USB cable

## Step 1 — Clone the repository

```bash
git clone https://github.com/meshcore-dev/MeshCore.git
cd MeshCore
```

## Step 2 — Open in VS Code

```bash
code .
```

PlatformIO will detect the `platformio.ini` and offer to set up the project. Accept the prompt.

## Step 3 — Select an example and a target device

The `examples/` directory contains six reference applications:

| Example | Description |
|---------|-------------|
| `companion_radio` | Full-featured radio for use with the mobile/web app over BLE, USB, or Wi-Fi |
| `kiss_modem` | Serial KISS protocol bridge — lets a host application control the radio |
| `simple_repeater` | Relay node that extends network coverage |
| `simple_room_server` | Shared BBS-style message board |
| `simple_secure_chat` | Encrypted terminal chat over serial/USB |
| `simple_sensor` | Remote telemetry node with alerting |

See [Example Applications](../developer/examples.md) for full details on each.

Open the `platformio.ini` inside the example you want to build. PlatformIO will list all supported environments (device variants) in the left sidebar.

## Step 4 — Configure your build

Most examples require you to set at least the LoRa frequency. Open the example's `platformio.ini` and look for `build_flags`:

```ini
build_flags =
  -D LORA_FREQ=869.525
  -D LORA_BW=250.0
  -D LORA_SF=9
```

Adjust frequency to match your region. See [Number Allocations](../number_allocations.md) for frequency guidance.

## Step 5 — Build and flash

1. Connect your device via USB
2. In the PlatformIO sidebar, select the environment matching your hardware
3. Click **Upload** (the right-arrow icon) or run:

```bash
pio run -e <your_environment> --target upload
```

## Step 6 — Open the Serial Monitor

For examples with a serial interface (e.g., `simple_secure_chat`):

```bash
pio device monitor
```

Default baud rate is 115200.

## Running unit tests

```bash
pio test --environment native --verbose
```

## Building the library standalone

To use MeshCore as a library in another project (without the examples):

```bash
python build_as_lib.py
```

## Next steps

- Read [Architecture Overview](../developer/architecture.md) to understand how MeshCore is structured
- Read [Contributing](../developer/contributing.md) for coding standards and PR guidelines
- Browse `src/Mesh.h` and `src/Dispatcher.h` for the main API
