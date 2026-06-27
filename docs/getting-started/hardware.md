# Hardware & Supported Devices

MeshCore supports 75+ LoRa devices across four chipset families. The easiest way to flash a device is via the **[MeshCore Flasher](https://meshcore.io/flasher)** — select your device and firmware type, and it handles the rest.

If your device is not in the flasher, it may still be supported via manual compilation. Find your device in the tables below and use the corresponding PlatformIO environment name.

---

## Heltec

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| Heltec V2 | ESP32 | `heltec_v2` |
| Heltec V3 | ESP32-S3 | `heltec_v3` |
| Heltec V4 | ESP32-S3 | `heltec_v4` |
| Heltec T96 | ESP32 | `heltec_t096` |
| Heltec T1 | ESP32 | `heltec_t1` |
| Heltec T114 | NRF52840 | `heltec_t114` |
| Heltec T190 | ESP32-S3 | `heltec_t190` |
| Heltec Tracker | ESP32-S3 | `heltec_tracker` |
| Heltec Tracker V2 | ESP32-S3 | `heltec_tracker_v2` |
| Heltec Wireless Paper | ESP32-S3 | `heltec_wireless_paper` |
| Heltec Mesh Solar | ESP32-S3 | `heltec_mesh_solar` |
| Heltec CT62 | ESP32-C3 | `heltec_ct62` |
| Heltec E213 | ESP32 | `heltec_e213` |
| Heltec E290 | ESP32 | `heltec_e290` |

---

## LilyGo

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| LilyGo T-Beam (SX1262) | ESP32 | `lilygo_tbeam_SX1262` |
| LilyGo T-Beam (SX1276) | ESP32 | `lilygo_tbeam_SX1276` |
| LilyGo T-Beam Supreme (SX1262) | ESP32-S3 | `lilygo_tbeam_supreme_SX1262` |
| LilyGo T-Beam 1W | ESP32 | `lilygo_tbeam_1w` |
| LilyGo T-Deck | ESP32-S3 | `lilygo_tdeck` |
| LilyGo T-Echo | NRF52840 | `lilygo_techo` |
| LilyGo T-Echo Card | NRF52840 | `lilygo_techo_card` |
| LilyGo T-Echo Lite | NRF52840 | `lilygo_techo_lite` |
| LilyGo T-ETH Elite | ESP32-S3 | `lilygo_teth_elite` |
| LilyGo T-Impulse Plus | ESP32-S3 | `lilygo_t_impulse_plus` |
| LilyGo T3S3 (SX1262) | ESP32-S3 | `lilygo_t3s3` |
| LilyGo T3S3 (SX1276) | ESP32-S3 | `lilygo_t3s3_sx1276` |
| LilyGo T-LoRa C6 | ESP32-C6 | `lilygo_tlora_c6` |
| LilyGo T-LoRa V2.1 | ESP32 | `lilygo_tlora_v2_1` |

---

## RAK Wireless

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| RAK4631 | NRF52840 | `rak4631` |
| RAK3112 | NRF52840 | `rak3112` |
| RAK3401 | NRF52840 | `rak3401` |
| RAK3x72 | NRF52840 | `rak3x72` |
| RAK11310 | RP2040 | `rak11310` |
| RAK WisMesh Tag | NRF52840 | `rak_wismesh_tag` |

---

## Seeed XIAO

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| XIAO ESP32-C3 | ESP32-C3 | `xiao_c3` |
| XIAO ESP32-C6 | ESP32-C6 | `xiao_c6` |
| XIAO ESP32-S3 | ESP32-S3 | `xiao_s3` |
| XIAO ESP32-S3 WIO | ESP32-S3 | `xiao_s3_wio` |
| XIAO NRF52840 | NRF52840 | `xiao_nrf52` |
| XIAO RP2040 | RP2040 | `xiao_rp2040` |

---

## Ikoka

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| Ikoka Handheld NRF | NRF52840 | `ikoka_handheld_nrf` |
| Ikoka Nano NRF | NRF52840 | `ikoka_nano_nrf` |
| Ikoka Stick NRF | NRF52840 | `ikoka_stick_nrf` |

---

## ThinkNode

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| ThinkNode M1 | ESP32-S3 | `thinknode_m1` |
| ThinkNode M2 | ESP32-S3 | `thinknode_m2` |
| ThinkNode M3 | ESP32-S3 | `thinknode_m3` |
| ThinkNode M5 | ESP32-S3 | `thinknode_m5` |
| ThinkNode M6 | ESP32-S3 | `thinknode_m6` |

---

## GAT562

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| GAT562 30S Mesh Kit | ESP32-S3 | `gat562_30s_mesh_kit` |
| GAT562 Mesh EVB Pro | ESP32-S3 | `gat562_mesh_evb_pro` |
| GAT562 Mesh Tracker Pro | ESP32-S3 | `gat562_mesh_tracker_pro` |
| GAT562 Mesh Watch 13 | ESP32-S3 | `gat562_mesh_watch13` |

---

## Wio / SenseCAP

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| Wio E5 Dev | STM32WL | `wio-e5-dev` |
| Wio E5 Mini | STM32WL | `wio-e5-mini` |
| Wio Tracker L1 | STM32WL | `wio-tracker-l1` |
| Wio Tracker L1 E-ink | STM32WL | `wio-tracker-l1-eink` |
| Wio WM1110 | NRF52840 | `wio_wm1110` |
| SenseCAP Indicator (ESPNow) | ESP32-S3 | `sensecap_indicator-espnow` |
| SenseCAP Solar | ESP32-S3 | `sensecap_solar` |

---

## Waveshare / Raspberry Pi

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| Waveshare RP2040 LoRa | RP2040 | `waveshare_rp2040_lora` |
| Raspberry Pi Pico W | RP2040 | `rpi_picow` |

---

## Other / Miscellaneous

| Device | Chipset | PlatformIO Environment |
|--------|---------|----------------------|
| Ebyte EoRa S3 | ESP32-S3 | `ebyte_eora_s3` |
| Generic E22 | ESP32 | `generic-e22` |
| Generic ESPNow | ESP32 | `generic_espnow` |
| Keepteen LT1 | ESP32-S3 | `keepteen_lt1` |
| M5Stack Unit C6L | ESP32-C6 | `m5stack_unit_c6l` |
| Mesh Pocket | ESP32-S3 | `mesh_pocket` |
| MeshAdventurer | ESP32-S3 | `meshadventurer` |
| MeshTiny | ESP32-C3 | `meshtiny` |
| Minewsemi ME25LS01 | ESP32-S3 | `minewsemi_me25ls01` |
| Muziworks R1 Neo | ESP32-S3 | `muziworks_r1_neo` |
| Nano G2 Ultra | ESP32-S3 | `nano_g2_ultra` |
| Nibble Screen Connect | ESP32-S3 | `nibble_screen_connect` |
| ProMicro | ESP32-C3 | `promicro` |
| Station G2 | ESP32 | `station_g2` |
| Station G3 | ESP32-S3 | `station_g3_esp32` |
| T1000-E | NRF52840 | `t1000-e` |
| Tenstar C3 | ESP32-C3 | `tenstar_c3` |
| Tiny Relay | ESP32-C3 | `tiny_relay` |

---

## Adding a new device

If your device isn't listed, you can add support by:

1. Creating a new directory under `variants/` with your device name
2. Adding a `platformio.ini` that sets the board, framework, and required build flags
3. Implementing any board-specific pin definitions in `src/helpers/`
4. Submitting a pull request — see [Contributing](../developer/contributing.md)
