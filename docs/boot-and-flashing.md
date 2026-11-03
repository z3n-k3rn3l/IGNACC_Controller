# Boot and flashing

How the ESP32 starts, how `idf.py flash` puts new firmware on it, and **what
this project switched off** compared with a normal ESP32 project, and why. UART0
is shared between flashing and the relay protocol.

**Reading order for newcomers:**
1. [ESP32 primer](esp32-primer.md)
2. [ESP-IDF primer](esp-idf-primer.md)
3. Boot and flashing (this page)
4. [Code tour](code-tour.md)

---

## 1. In plain words

When the ESP32 is switched on, a tiny program built into the chip (the **ROM
bootloader**) runs first. It checks one pin, GPIO0:

- **GPIO0 HIGH (normal):** it starts the bootloader stored in flash, which
  starts our firmware.
- **GPIO0 LOW (BOOT button held, or set by the PC):** it waits for the PC to send
  new firmware over the USB cable.

Flashing never needs our firmware's help. The PC resets the chip into the second
mode, writes the flash, and resets it back into the first.

---

## 2. What is in the flash

From this build (`partition-table.bin` and `flash_args`):

| Address | Size | Content | Written by |
|---|---|---|---|
| `0x1000` | ≈ 18 KB | **Second-stage bootloader** | `idf.py flash` |
| `0x8000` | 3 KB | **Partition table**: the map below | `idf.py flash` |
| `0x9000` | 24 KB | `nvs`: key-value storage (unused for now; the natural home for future runtime settings) | the firmware, at runtime |
| `0xF000` | 4 KB | `phy_init`: radio calibration (unused, no Wi-Fi) | — |
| `0x10000` | 1 MB | `factory`: **the app** (`IGNACC_Controller.bin`, 154 KB) | `idf.py flash` |

Flash mode DIO at 40 MHz; 2 MB configured (the module has 4 MB).

---

## 3. The boot chain

| # | Stage | Where it lives | Open source? | What it does |
|---|---|---|---|---|
| 1 | **ROM bootloader** (first stage) | chip ROM, can't be changed | no | Reads the **strapping pins**. Prints `ets Jul 29 2019 …`, `rst:0x1 (POWERON_RESET) …` on UART0. Normal boot: loads stage 2 from `0x1000`. GPIO0 low: **download mode** |
| 2 | **Second-stage bootloader** | flash `0x1000` | yes (ESP-IDF) | Reads the partition table, picks the app, loads its RAM parts and maps its flash parts, jumps to it. **Its messages are switched off in this project** |
| 3 | **App startup** (ESP-IDF) | in the app | yes | Clocks, memory, heap, both CPU cores, starts **FreeRTOS**, creates the `main` task |
| 4 | **`app_main()`** | `main/main.c` | yes (this project) | Relays OFF, UART link, state machine, tasks. See the [code tour](code-tour.md) |

The whole chain takes well under a second. The relay pins stay inputs or OFF
throughout, so no relay clicks during boot
([activity-boot diagram](diagrams/activity-boot.svg)).

Docs:
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/startup.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/bootloader.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/partition-tables.html>

---

## 4. How `idf.py flash` works

`idf.py flash` builds if needed, then runs **esptool**:

1. **Reset into download mode.** The devkit has an **auto-reset circuit**: two
   transistors connect the USB chip's **DTR** and **RTS** lines to **GPIO0** and
   **EN** (reset). esptool toggles DTR/RTS so GPIO0 is LOW while EN pulses. The
   ROM bootloader then starts in download mode.
2. **Upload a "stub".** esptool loads a small helper program into RAM. You see
   `Uploading stub flasher…`. It speeds up writing and supports compression.
3. **Write and verify.** It writes the bootloader, the partition table and the
   app at their addresses, then checks a hash of each (`Hash of data verified`).
4. **Reset back.** It pulses EN via RTS with GPIO0 HIGH (`Hard resetting via RTS
   pin…`). The board boots normally into the new firmware.

**If automatic reset fails:** hold **BOOT**, press and release **EN**, release
**BOOT**, then run `idf.py -p COMx flash` again.

**Only one program can use the COM port.** Close the IGNACC Tester, PuTTY,
Serial Studio or CANoe before flashing.

Docs:
- <https://docs.espressif.com/projects/esptool/en/latest/esp32/>
- <https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/boot-mode-selection.html>

---

## 5. UART0 is shared: what this project changed

UART0 (GPIO1/GPIO3 → CP210x → COM15) normally carries three things:
the ROM bootloader's messages, flashing, and the **console** (`printf` and
ESP-IDF log messages). This project also uses it for the **relay protocol**.
The host must see only protocol replies, so the console and logs are switched
off in [sdkconfig.defaults](../sdkconfig.defaults).

### What is different from a default ESP-IDF project

| Setting (menuconfig name) | ESP-IDF default | This project | Why | Consequence |
|---|---|---|---|---|
| Console output channel (`ESP_CONSOLE_NONE`) | UART0 | **None** | nothing else may write to UART0 | `printf()` and `ESP_LOGx()` print nothing |
| App log level (`LOG_DEFAULT_LEVEL_NONE`) | Info | **None** | no log lines between protocol replies | no logs at all; `idf.py monitor` shows only ROM text and replies |
| Bootloader log level (`BOOTLOADER_LOG_LEVEL_NONE`) | Info | **None** | no boot text from stage 2 | at reset, only the ROM's own text appears |
| FreeRTOS tick (`FREERTOS_HZ`) | 100 Hz | **1000 Hz** | the 15 ms settle isn't rounded to 10/20 ms | negligible extra CPU load |
| Who drives UART0 | console driver | **this firmware's `uart_link`** | installs the IDF UART driver for the protocol | the protocol owns UART0 |
| Crash (panic) messages | printed, then reboot | **not printed**, still reboots | console is off | a crash shows only as a reboot (the host sees ROM text and must repeat `HeyU`) |

### What is **not** changed (and can't be from software)

- **ROM bootloader messages still appear at every reset.** On the original
  ESP32 there is no software setting for them; the only switch is in hardware:
  the GPIO15 (MTDO) strapping pin held LOW at reset. This project doesn't do that. The firmware's parser simply ignores that
  text, and the host should send `HeyU` and wait for `Hello` before commands.
- **Flashing still works exactly as normal.** It runs in the ROM, before our
  firmware starts.
- **Opening the COM port can reset the board** through DTR/RTS. The IGNACC Tester
  opens it with DTR/RTS off; in other programs, turn them off.

### Getting logs back for debugging

For a debug build only. Logs and protocol replies would mix, so the IGNACC
Tester and CAPL will fail while it's on.

1. `idf.py menuconfig`, press `/` and search:
   - `ESP_CONSOLE_UART_DEFAULT`: set console output back to UART0
   - `LOG_DEFAULT_LEVEL_INFO`: set the log level back to Info
2. `idf.py -p COM15 flash monitor`. You'll see logs and crash messages.
3. Undo afterwards: delete `sdkconfig` and run `idf.py reconfigure`, which
   restores `sdkconfig.defaults`. Then build and flash again.

A cleaner option for longer debugging: send the console to **UART1 on other
pins** (`ESP_CONSOLE_UART_CUSTOM`) through an external USB-serial adapter. The
protocol on UART0 then stays clean.
