# MeshCore

**MeshCore** is a lightweight, portable C++ library that enables multi-hop packet routing for embedded projects using LoRa and other packet radios. Build resilient, decentralized communication networks that work without internet infrastructure.

[Get Started](getting-started/users.md){ .md-button .md-button--primary }
[View on GitHub](https://github.com/meshcore-dev/MeshCore){ .md-button }

---

## Choose your path

<div class="grid cards" markdown>

-   **Flash & Use**

    ---

    Buy a supported device, flash the pre-built firmware, and start communicating in minutes.

    [:octicons-arrow-right-24: Quick Start for Users](getting-started/users.md)

-   **Build on MeshCore**

    ---

    Integrate the MeshCore library into your own embedded firmware project using PlatformIO.

    [:octicons-arrow-right-24: Quick Start for Developers](getting-started/developers.md)

-   **Explore the Protocol**

    ---

    Deep-dive into the packet format, cryptographic model, and protocol specifications.

    [:octicons-arrow-right-24: Protocol Reference](packet_format.md)

</div>

---

## What is MeshCore?

MeshCore creates wireless mesh networks using LoRa radios. Messages travel across multiple nodes (multi-hop), extending range far beyond a single radio. There is no central server — the network is self-organizing and self-healing.

**Key features:**

- **Multi-hop routing** — messages relay through intermediate nodes automatically
- **Decentralized** — no internet or central infrastructure required
- **Encrypted** — Ed25519 signatures and AES-128 per-link encryption
- **Low power** — designed for battery and solar-powered devices
- **70+ supported devices** — Heltec, RAK Wireless, LilyGo, Seeed XIAO, and more

**Use cases:** off-grid communication, emergency response, hiking & outdoor activities, tactical networks, IoT sensor telemetry.

---

## Clients & Apps

Connect to your MeshCore companion radio via BLE, USB, or Wi-Fi:

| Client | Platform | Link |
|--------|----------|------|
| Web App | Browser | [app.meshcore.nz](https://app.meshcore.nz) |
| Android | Android | [Google Play](https://play.google.com/store/apps/details?id=com.liamcottle.meshcore.android) |
| iOS | iPhone/iPad | [App Store](https://apps.apple.com/us/app/meshcore/id6742354151) |
| Node.js | Server/Desktop | [meshcore.js](https://github.com/liamcottle/meshcore.js) |
| Python CLI | Terminal | [meshcore-cli](https://github.com/fdlamotte/meshcore-cli) |

---

## Community

- [Discord](https://meshcore.gg) — chat with developers and get help
- [GitHub Issues](https://github.com/meshcore-dev/MeshCore/issues) — bug reports and feature requests
- [YouTube QuickStart](https://www.youtube.com/watch?v=iaFltojJrAc&list=PLshzThxhw4O4WU_iZo3NmNZOv6KMrUuF9) by The Comms Channel
- [Technical Presentation](https://www.youtube.com/watch?v=OwmkVkZQTf4) by Liam Cottle
