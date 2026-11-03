# ESP-IDF primer

What ESP-IDF is, how it turns this project's C code into firmware, how
**Kconfig** settings reach the code, and why everything runs on **FreeRTOS**.
It also covers where to install ESP-IDF on Windows and Linux.

**Reading order for newcomers:**
1. [ESP32 primer](esp32-primer.md)
2. ESP-IDF primer (this page)
3. [Boot and flashing](boot-and-flashing.md)
4. [Code tour](code-tour.md)

---

## 1. What is ESP-IDF?

**ESP-IDF** (Espressif IoT Development Framework) is Espressif's official SDK
for ESP32 chips. It is open source (Apache 2.0) and contains:

- the **build system** (CMake + Ninja, driven by the `idf.py` command),
- **drivers** for every peripheral (UART, GPIO, SPI, …),
- **FreeRTOS**, plus startup code, memory management and watchdogs,
- the **second-stage bootloader** and partition-table tools,
- optional networking stacks (Wi-Fi, Bluetooth, TCP/IP, MQTT, …),
- **Kconfig**, the configuration system (`idf.py menuconfig`).

This project uses ESP-IDF **v6.1**.

---

## 2. Install it

Espressif's guides cover every OS step by step:

| OS | Guide |
|---|---|
| All (start here) | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html> |
| Windows | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/windows-setup.html> |
| Linux / macOS | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-macos-setup.html> |
| ESP-IDF Installation Manager (EIM) | <https://docs.espressif.com/projects/idf-im-ui/en/latest/> |
| What each tool is | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-tools.html> |

**On this PC (Windows 11):** v6.1 was installed with the Installation Manager.
- ESP-IDF itself is in `C:\esp\v6.1\esp-idf`.
- The tools are in `C:\Espressif\tools`.
- Each PowerShell session starts with
  `C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`, which sets
  `IDF_PATH` and the PATH for `idf.py`.

**On Linux:** install the prerequisites from the guide (git, cmake, ninja,
python3, …), then use the Installation Manager or `git clone` ESP-IDF and run
`./install.sh esp32` followed by `. ./export.sh`. The project builds the same way.

---

## 3. From source to firmware

```
main/, components/  ──►  CMake configure  ──►  Ninja + xtensa GCC  ──►  linker  ──►  ELF  ──►  esptool  ──►  .bin files  ──►  flash
                         (find components,       (compile each .c)       (memory map,               (elf2image)
                          run Kconfig)                                     ROM symbols)
```

`idf.py build` runs all of it:

| Step | What happens | Tool on this PC |
|---|---|---|
| Configure | Finds every component (ESP-IDF's and this project's `components/` + `main/`), reads all Kconfig files and the `sdkconfig`, and generates `sdkconfig.h` | CMake 4.0.3 |
| Compile | Compiles every `.c` file for the Xtensa CPU | **xtensa-esp32-elf-gcc 15.2.0** (crosstool-NG `esp-15.2.0_20251204`), `-std=gnu23` |
| C library | `printf`, `memcpy`, `snprintf`, … | **picolibc**, the ESP-IDF 6.1 default (older versions used newlib; a newlib-compatibility option is on) |
| Link | Places code in IRAM/flash and data in DRAM, using ESP-IDF's linker scripts, and resolves ROM functions | GNU ld (binutils) |
| Image | Converts the ELF into flashable `.bin` files | esptool 5.4.0 |
| Build driver | Runs the steps above in parallel | Ninja 1.12.1 |

Outputs in `build/`: `bootloader/bootloader.bin`,
`partition_table/partition-table.bin` and `IGNACC_Controller.bin`. `idf.py flash`
writes them to the board; see [boot-and-flashing.md](boot-and-flashing.md).

Each folder under `components/` is a **component**: a mini-library with its
own `CMakeLists.txt`. The `idf_component_register(... REQUIRES ...)` line
declares what it depends on. `main/` is the special component holding
`app_main()`.

---

## 4. Kconfig: how settings reach the code

Kconfig is ESP-IDF's settings system, taken from the Linux kernel. Every
setting is decided at **build time** and becomes a C macro.

```
Kconfig files ──► menuconfig ──► sdkconfig ──► build/config/sdkconfig.h ──► #include "sdkconfig.h" in C
(define menus)    (you choose)   (all values,   (#define CONFIG_… values)
                                  git-ignored)
                   ▲
sdkconfig.defaults ┘  (the project's chosen defaults, committed)
```

| File | Role |
|---|---|
| `Kconfig` / `Kconfig.projbuild` in a component | defines settings, defaults, ranges and help. This project's menu is in [main/Kconfig.projbuild](../main/Kconfig.projbuild) |
| `sdkconfig.defaults` (committed) | starting values when `sdkconfig` doesn't exist yet. This project uses it to switch the console and logs off and set a 1 kHz tick |
| `sdkconfig` (generated, git-ignored) | the complete list of every setting's current value. `menuconfig` edits it |
| `build/config/sdkconfig.h` | the same values as C macros, e.g. `#define CONFIG_IGNACC_RELAY1_GPIO 26` |

**Important:** changing a setting means **rebuild and reflash**. The board can't
change its settings at runtime; that is the planned "dynamic configuration"
feature, not yet built.

Useful commands:

```bash
idf.py menuconfig
```
```bash
idf.py reconfigure
```

- `menuconfig` opens the menus. Press `/` to search by name, e.g. `IGNACC_RELAY_COUNT`.
- `reconfigure` re-reads `sdkconfig.defaults` and the Kconfig files.

To return to this project's defaults, delete `sdkconfig` and run
`idf.py reconfigure`.

How this project's own settings (relay count, pins, UART, tasks) are designed
is explained in [configuration-and-architecture.md](configuration-and-architecture.md),
with the [Kconfig channel model diagram](diagrams/kconfig-channel-model.svg).

---

## 5. FreeRTOS: why it's there, and how this project uses it

### What it is

**FreeRTOS** is a small real-time operating system kernel (MIT license). It lets
one program run as several **tasks**, each with its own stack and priority. A
**scheduler** switches between them, thousands of times per second if needed. It
also provides **queues** to pass data between tasks and **delays** that let a
task sleep without wasting CPU.

### Why ESP-IDF is built on it

- ESP-IDF's own parts are tasks and queues: the UART driver delivers events
  through a queue, and Wi-Fi, Bluetooth, TCP/IP and timers run as tasks.
  Without an RTOS they couldn't run side by side.
- The ESP32 has **two cores**. Espressif extended FreeRTOS to schedule tasks on
  both cores (SMP) and to pin a task to one core.
- FreeRTOS is small, widely used and MIT-licensed, so it fits a
  microcontroller and suits commercial products.

### What Espressif changed

- **"ESP-IDF FreeRTOS":** based on FreeRTOS **V10.5.1**, made dual-core. It adds
  core affinity (`xTaskCreatePinnedToCore`) and spinlock-based critical sections
  that work across both cores. This is the version this project uses.
- An alternative is the upstream **SMP FreeRTOS V11.1.0**, selectable in menuconfig.
- ESP-IDF **starts the scheduler for you**. By the time `app_main()` runs, it is
  already running inside a task called "main" (stack 3584 bytes, core 0).
  There is no `main()` and no `vTaskStartScheduler()` to write.

Docs:
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html>

### Why this project needs it

The UART must keep receiving while a relay command waits its 15 ms settle
delay. So the firmware uses two tasks and a queue:

| Task | Priority | Job |
|---|---|---|
| `main` | (ESP-IDF) | runs `app_main()`: creates everything, then returns, which ends the task |
| `uart_link` | 10 | waits for UART driver events, turns bytes into lines, puts lines in the queue |
| `app_fsm` | 9 | takes lines from the queue, switches relays (`vTaskDelay` for the settle), replies |
| ESP-IDF's own | various | idle task per core, timers, inter-core calls, watchdog |

The FreeRTOS **tick** is set to **1000 Hz** (default 100 Hz), so a 15 ms delay
is 15 ms, not rounded to 10 or 20. The task and interrupt **watchdogs** are on
(ESP-IDF defaults), so a stuck task would reset the board.

The state machine itself (`app_fsm.c`) contains no FreeRTOS calls. The task
wrapper (`app_fsm_task.c`) is the only bridge, and that is why the logic can be
unit-tested on a PC; see [unit-testing.md](unit-testing.md).

---

## 6. More from Espressif

| Topic | Link |
|---|---|
| Build system | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html> |
| Every Kconfig option | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/kconfig-reference.html> |
| Startup flow | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/startup.html> |
| Source code | <https://github.com/espressif/esp-idf> |

Every tool this project uses, with versions, is listed in
[toolchain.md](toolchain.md). That page also covers how the Python tester app
is compiled (PyInstaller).
