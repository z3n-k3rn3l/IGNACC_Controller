# IGNACC_Controller

ESP32 firmware (ESP-IDF v6.1, C) that switches a 2-channel relay module on
ASCII commands received over the board's USB-UART bridge.

## Hardware

| Relay module | ESP32-WROOM-32 devkit (38-pin) |
|---|---|
| IN1 | GPIO26 (default) |
| IN2 | GPIO27 (default) |
| VCC / GND | 5 V (VIN) / GND, per module rating |

- The module is **active-low**: pin LOW = relay ON, pin HIGH = relay OFF.
- **3.3 V logic:** an ESP32 HIGH is 3.3 V. Many 5 V relay modules (input LED
  wired from the module's 5 V VCC to IN) still see that as ON, so the relay
  never releases. This was confirmed on the bench: with IN tied to the ESP32's
  3V3 pin the relay stays ON. Fixes:
  - Module with a JD-VCC jumper: remove the jumper. Connect JD-VCC to 5 V (coil
    power) and VCC to the ESP32's 3V3 (input side). Keep active level LOW.
  - Otherwise: one NPN transistor or N-MOSFET per channel (GPIO drives the
    base/gate, collector/drain pulls IN to GND). This inverts the logic, so set
    menuconfig → Relay active level → **Active HIGH**.
  - Or use a relay module rated for 3.3 V inputs.
- At boot, and whenever a relay command fails, both relays are driven OFF.
- The OFF level is written to the output register *before* the pin is
  switched to output, so there is no ON glitch at boot. During reset, before
  the firmware runs, the pins are high-impedance inputs, which typical
  opto-isolated modules see as OFF.
- **Channels:** 2 by default, up to 14 (menuconfig → Relays → Number of relay
  channels). Each channel gets its own GPIO setting. The safe pins are 4, 13,
  16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32 and 33 (`esp32_relay_pins.h`).
- **Build-time pin checks:** the build fails if a channel uses a strapping
  pin (0, 2, 5, 12, 15), a flash pin (6–11), a UART pin, GPIO14 or an
  input-only pin (34–39), or if two channels share a GPIO.

## Protocol

UART0 through the on-board micro-USB bridge, **115200 8N1**.

- Each line ends with `\n`; a trailing `\r` is stripped.
- Lines longer than 32 chars (configurable) are discarded entirely.
- Commands are case-sensitive, and every response ends with `\r\n`.

| Command | Response |
|---|---|
| `IN1_ON`  | `IN1_ON_OK` / `IN1_ON_FAIL` |
| `IN1_OFF` | `IN1_OFF_OK` / `IN1_OFF_FAIL` |
| `IN2_ON`  | `IN2_ON_OK` / `IN2_ON_FAIL` |
| `IN2_OFF` | `IN2_OFF_OK` / `IN2_OFF_FAIL` |
| `HeyU`    | `Hello IGNACC v<FW_VERSION> READY` |
| `IN<n>_ON` / `IN<n>_OFF` | `IN<n>_ON_OK` … for every configured channel n (IN3… only on boards with 3+ channels) |
| `ALL_ON`  | `ALL_ON_OK` / `ALL_ON_FAIL` |
| `ALL_OFF` | `ALL_OFF_OK` / `ALL_OFF_FAIL` |
| `STATUS`  | `STATUS IN1=<ON\|OFF> IN2=<ON\|OFF>`, one field per configured channel |
| anything else (typo, wrong case, a channel the board doesn't have) | `UNK_NACK` |

- **OK** is sent after the pin is driven, the relay settle delay (default 15 ms)
  has passed, and read-back still shows the commanded level.
- **FAIL** means the GPIO API returned an error or the pin read-back did not
  match. Both relays are then driven OFF.

  The read-back checks the **pin**, not the relay contact; no contact feedback
  is wired.
- **STATUS** reports the pin levels as read back from the hardware.
- **ALL_ON / ALL_OFF** drive every channel back to back (microseconds apart),
  wait one settle delay, then check every read-back before `_OK`. If any channel
  fails, all relays are driven OFF and the reply is `_FAIL`. A CAPL script can
  start with `ALL_OFF` to put the bench in a known safe state.
- **UNK_NACK** answers any line that isn't a valid command: wrong case, extra
  spaces, unknown text, a malformed channel (`IN0_ON`, `IN01_ON`) or a channel
  the board doesn't have (`IN3_ON` on a 2-channel board). The host learns about
  the mistake in a few ms instead of waiting for a timeout. No relay is touched.
- Still silent: lines longer than 32 characters (dropped by the UART layer
  before parsing), an incomplete line (no `\n` yet), and anything received
  during the first milliseconds after boot.
- Commands are idempotent: re-sending the current state drives the pin again
  and returns `_OK`.
- If more than 4 lines (configurable) are waiting, extra lines are dropped
  without a response. Wait for each response before sending the next command.

### Host checklist

1. **The ROM bootloader always prints at reset**, e.g. `ets Jul 29 2019 …`,
   `rst:0x1 (POWERON_RESET)…`, `entry 0x…`. On ESP32 the application cannot
   disable this. The firmware itself prints nothing: application logs,
   2nd-stage bootloader logs and the console are all disabled in
   `sdkconfig.defaults`. Discard everything up to the `Hello` line.
2. **Handshake first.** Send `HeyU` and wait for
   `Hello IGNACC v… READY` before sending commands. Retry `HeyU` until it
   answers, since the board may still be booting.
3. **Opening the COM port can reset the board.** The devkit's auto-reset
   circuit ties DTR/RTS to EN/GPIO0. Deassert DTR and RTS before or when
   opening the port. In pyserial:
   ```python
   s = serial.Serial(); s.port = "COM15"; s.baudrate = 115200
   s.dtr = False; s.rts = False; s.open()
   ```
   In terminal programs, turn off DTR/RTS or hardware flow control.

## Configuration

`idf.py menuconfig` → **IGNACC Controller** has these settings:

- number of relay channels (1–14), their pins and the active level
- relay settle delay
- UART port, baud rate, TX/RX pins, RX buffer size and event queue length
- maximum line length
- command queue depth
- task priorities and stack sizes

The firmware version is defined in exactly one place: `PROJECT_VER` in the
top-level [CMakeLists.txt](CMakeLists.txt).

## Build, flash, test

From an ESP-IDF v6.1 shell (EIM: `C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`):

```bash
idf.py set-target esp32
```
```bash
idf.py build
```
```bash
idf.py -p COM15 flash
```

Host unit tests (MinGW gcc + CMake; Unity is taken from `$IDF_PATH`):

```bash
cmake -S test/host -B test/host/build -G "MinGW Makefiles"
```
```bash
cmake --build test/host/build
```
```bash
ctest --test-dir test/host/build --output-on-failure
```

Hardware-in-the-loop test against a flashed board. **The relays will click.**

```bash
python tools/hil_test.py --port COM15
```

## Editor setup (VS Code code navigation)

Code navigation uses **clangd** (extension `llvm-vs-code-extensions.vscode-clangd`)
with the `esp-clangd` that ships with ESP-IDF v6.1. Microsoft C/C++ IntelliSense
is disabled.

- `.clangd` points clangd at `build/compile_commands.json`, which `idf.py build`
  creates. It also strips GCC-only `-m*` / `-f*` flags.
- `test/host/.clangd` points the unit tests at
  `test/host/build/compile_commands.json`, which the host-test CMake configure
  step creates.
- `.vscode/settings.json` (local, machine-specific) sets `clangd.path`,
  `--query-driver` (Xtensa GCC and MinGW) and `--resource-dir`. The esp-clangd
  package has no built-in headers such as `stddef.h`, so they come from the
  matching `esp-clang` package.

After a fresh clone, or after adding source files, run `idf.py build` and the
host-test `cmake -S test/host -B test/host/build ...` configure step, then run
**clangd: Restart language server** in VS Code.

Full step-by-step guide, including the exact settings, how to verify, and
troubleshooting: [docs/vscode-clangd-setup.md](docs/vscode-clangd-setup.md).

## Prebuilt tester

[tools/ignacc_tester/bin/v1.2.0/](tools/ignacc_tester/bin/v1.2.0/) (ALL ON/OFF, UNK_NACK tests) has the
ready-to-run tester `.exe`, its SHA-256 checksum and a short README. The
firmware isn't stored as binaries: build and flash it from source with
`idf.py -p COMx flash`.

## Repositories

This repo is kept in two private GitHub repos, `GeoLCost/IGNACC_Controller`
and `z3n-k3rn3l/IGNACC_Controller`. A plain `git push` updates both. To set up
two accounts on Windows or Linux, see
[docs/github-two-accounts.md](docs/github-two-accounts.md).

## IGNACC Tester (GUI)

A single-file Windows app that runs without Python installed. The committed
copy is `tools/ignacc_tester/bin/v1.2.0/IGNACC_Tester-v1.2.0-windows-x64.exe`.
A local rebuild goes to `tools/ignacc_tester/dist/IGNACC_Tester.exe`, which
git ignores.

- **Connection:** pick the COM port from the dropdown (Refresh rescans) and
  click Connect. The port is opened with DTR/RTS off, so the board is not reset.
- **Tests:** run all 15 tests (including ALL_ON/ALL_OFF), only the ticked ones, or a range (from/to).
  - The delay between steps (default 1 s) lets you watch the relays switch.
  - Each test shows PASS/FAIL, and the log has the reply and round trip for
    every step. Save log writes it to a file.
- **Latency:** every reply is checked against the limit (default 20 ms). The
  Latency test sends each command 10 times and reports min/avg/max.
  - By default, relay replies are judged on communication time only. They
    include the firmware's 15 ms settle delay, so a relay reply takes about 19 ms.
  - Untick "exclude settle" to apply the limit to the total round trip.
- **Manual control:** buttons for each command, a custom line, and IN1/IN2
  indicators.
- **Safety:** both relays are commanded OFF after every run, on Stop, and when
  the window closes.

Rebuild after editing (needs Python 3.9+ with Tkinter; pyserial and
PyInstaller are installed into a local `.venv`):

```bash
powershell -ExecutionPolicy Bypass -File tools/ignacc_tester/build_exe.ps1
```

Headless runs use the same tests from the command line, for example tests 2–5
with 0.1 s steps:

```bash
tools/ignacc_tester/.venv/Scripts/python.exe tools/ignacc_tester/tester_core.py --port COM15 --tests 2-5 --delay 0.1
```

### Measured timing (CP210x bridge, Windows 11)

| | Round trip |
|---|---|
| `HeyU`, `STATUS` | 4.4–8 ms |
| `INx_ON` / `INx_OFF` (includes 15 ms settle) | 18.6–23 ms |
| Communication only (relay replies minus settle) | ≤ 8 ms |

- The relay pin switches as soon as the command line arrives. `_OK` follows
  15 ms later.
- With a deadline-scheduled host loop, ON pulses of 1.5–2.1 s in 100 ms and
  50 ms steps were all sent on time to within 0.01 ms, and every reply was `_OK`.
- The edges at the pin are expected to be within a few ms of the target,
  because of USB transfer jitter. Relay operate/release time adds a few
  mechanical ms on top. Verify with a scope or logic analyzer if you need
  exact edge timing.
- Do not call pyserial's `flush()` after `write()` on Windows. On the CP210x
  driver it intermittently blocks for about 50 ms.

## Architecture

```
main (composition root)
 ├─> app_fsm ──> protocol        (pure C)
 │      ├────> relay.h           (interface)
 │      └────> link_if.h         (interface)
 ├─> relay      relay_bank (N relays) ──> relay.h ──> relay_gpio ──> pin_hal.h ──> pin_hal_esp
 └─> uart_link  line_assembler (pure C) + IDF UART driver, implements link_if
```

| Component | Role | IDF-free? |
|---|---|---|
| `protocol` | Parse lines into commands and format responses | yes |
| `link_if` | Line type and transmit interface shared by the link and the FSM | yes (header only) |
| `relay` | `relay_t` interface (handle + ops table); `relay_bank` groups any number of `relay_t` for ALL_ON/ALL_OFF; `relay_gpio` on a mockable `pin_hal_t`; `pin_hal_esp` is the only file that uses `driver/gpio.h` | `relay_gpio.c`, `relay_bank.c` yes |
| `uart_link` | IDF UART driver (driver ISR, ring buffer, event queue). The RX task assembles lines and posts them to the command queue. No knowledge of relays | `line_assembler.c` yes |
| `app_fsm` | State machine `INIT → READY`; sole owner of the relays (copies the bank's handles). Works with any channel count. Relays, link and delay are injected. `app_fsm_task.c` is the FreeRTOS runner | `app_fsm.c` yes |
| `main` | Reads Kconfig (channel count and pins), checks the pins at compile time, creates and wires the objects, starts the tasks | – |

Full details, including how the Kconfig channel model works, the
compile-time pin checks and every setting, are in
[docs/configuration-and-architecture.md](docs/configuration-and-architecture.md).
All public headers have Doxygen comments.

### Extending

- **More channels (up to 14):** menuconfig only. No code changes.
- **Different relay hardware** (I/O expander, shift register, a second relay
  board): write a new `relay_t` implementation (one `.c` file with `set` and
  `is_on`) and create it in `main`. The protocol, relay bank and state machine
  don't change.
- **More than 16 channels:** raise `PROTO_MAX_CHANNELS` (and `PROTO_RESPONSE_MAX`
  for the longer `STATUS` line).

Only `main` reads `sdkconfig`. All other components receive plain config
structs, which is what makes them host-testable.

### Failure behaviour

| Event | Behaviour |
|---|---|
| Init failure (GPIO, UART, queue, task) | `app_main` stops. Relays stay at whatever safe state was reached, and `HeyU` gets no reply |
| Relay command failure | Reply `_FAIL` and drive both relays OFF |
| UART FIFO or ring-buffer overflow | Flush, and discard the partial line up to the next `\n` |
| UART frame or parity error | Discard the line up to the next `\n` |
| Panic | Default IDF handler (reboot). The board comes back up with both relays OFF. The host should treat a missing reply as a timeout and repeat the `HeyU` handshake |
