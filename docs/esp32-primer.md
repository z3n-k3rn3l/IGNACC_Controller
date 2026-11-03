# ESP32 primer

What the ESP32 is, what's on this project's board, and which parts of the ESP32
world are open source. Read this first if you are new to the ESP32.

**Reading order for newcomers:**
1. ESP32 primer (this page)
2. [ESP-IDF primer](esp-idf-primer.md)
3. [Boot and flashing](boot-and-flashing.md)
4. [Code tour](code-tour.md)

---

## 1. What is an ESP32?

The ESP32 is a low-cost **microcontroller with Wi-Fi and Bluetooth**, made by
Espressif Systems (Shanghai). One chip holds two processor cores, memory, radio
and many peripherals (UART, SPI, I²C, ADC, PWM, CAN, …). You program it in C or
C++ with Espressif's framework, **ESP-IDF**, or with Arduino or MicroPython,
which are built on top of ESP-IDF.

This project uses only a small part of it: one UART and a few GPIO pins.
Wi-Fi and Bluetooth are not used.

---

## 2. The chip on this board

Read from the board with esptool, and from this project's build:

| Item | Value |
|---|---|
| Board | ESP32-WROOM-32 devkit, 38 pins, micro-USB |
| Module | ESP32-WROOM-32 (chip + 40 MHz crystal + SPI flash + antenna, under a metal can) |
| Chip | **ESP32-D0WD-V3**, silicon revision **v3.1** |
| CPU | **2 × Xtensa LX6** 32-bit cores (Cadence/Tensilica architecture, not ARM) |
| Clock | up to **240 MHz**. This build runs at **160 MHz**, the ESP-IDF default (menuconfig → ESP System Settings → CPU frequency) |
| Low-power core | ULP co-processor, for very small programs while the main cores sleep. Not used here |
| Crystal | 40 MHz |
| Flash | **4 MB** external SPI flash on the module. The build is configured for 2 MB, the ESP-IDF default, which is enough for this 154 KB app |
| USB | **No native USB.** The devkit's **CP210x** chip turns micro-USB into UART0 (this is COM15) |
| Radio | Wi-Fi 802.11 b/g/n (2.4 GHz), Bluetooth 4.2 Classic + BLE (unused here) |

### Memory

| Memory | Size | What it holds |
|---|---|---|
| ROM | 448 KB | Espressif's **first-stage bootloader** and helper functions. Fixed in silicon |
| Internal SRAM | 520 KB | Split by ESP-IDF into **IRAM** (fast code, interrupt handlers) and **DRAM** (data, stacks, heap) |
| RTC SRAM | 16 KB | Survives deep sleep (not used here) |
| External flash | 4 MB | Bootloader, partition table and the app. Code runs from flash through a cache ("execute in place") |

This firmware's usage, from `idf.py size`:

| Region | Used | Available |
|---|---|---|
| Flash (code + data) | ≈ 99 KB (154 KB image) | 1 MB app partition |
| IRAM | 45 KB | 128 KB |
| DRAM | 12 KB | 176 KB |

### Peripherals (summary)

34 GPIOs (6 of them input-only), 3 UARTs, 4 SPI, 2 I²C, 2 I²S, 18 ADC
channels, 2 DAC channels, 10 touch inputs, LED PWM (16 channels), motor PWM,
RMT (IR / timing), TWAI (CAN 2.0), Ethernet MAC, SD/MMC, hardware crypto
(AES, SHA, RSA), secure boot and flash encryption. The datasheet has the full list.

### Pins you must know about

| GPIO | Why it matters |
|---|---|
| **0** | Strapping pin: **LOW at reset = download (flashing) mode**. The BOOT button pulls it low |
| **2, 5, 12, 15** | Strapping pins, read at reset. GPIO12 selects flash voltage; **GPIO15 LOW silences the ROM boot messages** |
| **1, 3** | UART0 TX/RX, wired to the CP210x. Used for flashing **and** this project's protocol |
| **6–11** | Connected to the SPI flash inside the module. Never use them |
| **14** | Outputs a PWM signal during boot |
| **34–39** | Input only |
| **26, 27** | This project's IN1 / IN2 relays (default) |

That's why the firmware only accepts 14 "safe" relay pins; see
[esp32_relay_pins.h](../components/relay/include/esp32_relay_pins.h).

---

## 3. What is open source, and what isn't

| Part | Open? | Details |
|---|---|---|
| **ESP-IDF** (drivers, build system, startup code, system services) | **Yes**, Apache 2.0 | <https://github.com/espressif/esp-idf> |
| **Second-stage bootloader** | **Yes**, part of ESP-IDF | `components/bootloader`. You can configure or change it |
| **FreeRTOS** kernel (ESP-IDF's port) | **Yes**, MIT | Espressif's dual-core version of FreeRTOS V10.5.1, plus optional upstream SMP V11.1.0 |
| **First-stage ROM bootloader** and ROM functions | **No** | Burned into the chip's ROM at manufacture; it can't be changed or replaced. Its source isn't published. ESP-IDF ships linker scripts and headers so programs can call ROM functions |
| **Wi-Fi, Bluetooth, PHY (radio) drivers** | **No**, precompiled libraries | <https://github.com/espressif/esp32-wifi-lib> (binaries). **Not linked into this firmware**: the linker map shows none of their code is used |
| **Compiler toolchain** (GCC, binutils, GDB) | **Yes**, GPL | Espressif builds them with crosstool-NG: <https://github.com/espressif/crosstool-NG> |
| **esptool** (the flasher) | **Yes**, GPL-2.0 | <https://github.com/espressif/esptool> |
| Chip design | No | Datasheet and Technical Reference Manual are public |

**For this project:** everything that runs on the board is open source except
the ROM bootloader, which every ESP32 has and nobody can change.

---

## 4. Official documents

| Document | Link |
|---|---|
| ESP32 datasheet (pins, electrical, memory) | <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf> |
| ESP32-WROOM-32 module datasheet | <https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf> |
| ESP32 Technical Reference Manual (every register) | <https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf> |
| ESP-IDF documentation (ESP32, stable) | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html> |
