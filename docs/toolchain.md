# Toolchain

Every tool this project uses, what it is for, the version on the reference PC
(Windows 11), and where the details are.

## Overview

| Purpose | Tool | Version | Needs the board? | Details |
|---|---|---|---|---|
| Build and flash the firmware | ESP-IDF | v6.1 | flashing only | section 1 below; background in [esp-idf-primer.md](esp-idf-primer.md) |
| Unit tests (firmware logic) | MinGW GCC + CMake + Unity | GCC 15.2.0 · CMake 4.3.3 · Unity 2.6.0 | **no** | [unit-testing.md](unit-testing.md) |
| Hardware tests over the serial port | **IGNACC Tester** (compiled Python app) | 1.2.0 | **yes** (COM port) | section 3 below |
| API documentation | Doxygen + Graphviz | 1.18.0 · 16.1.0 | no | section 4 below |
| Diagrams | `generate_diagrams.py` (Python standard library) | – | no | [diagrams/README.md](diagrams/README.md) |
| Code navigation in VS Code | clangd (esp-clangd) | 21.1.3 | no | [vscode-clangd-setup.md](vscode-clangd-setup.md) |
| Version control (two GitHub accounts) | Git + SSH | 2.56 | no | [github-two-accounts.md](github-two-accounts.md) |

### Before every commit

| Check | Command | Expected |
|---|---|---|
| Firmware builds | `idf.py build` | `Project build complete` |
| Unit tests pass | `ctest --test-dir test/host/build` | `100% tests passed … out of 5` (75 tests) |
| Board behaves | `IGNACC_Tester.exe`, or `python tester_core.py --port COMx` | `15 passed, 0 failed` |
| Docs current, if comments changed | `doxygen` | `docs/api/doxygen-warnings.log` empty |

The first two need no hardware. The third needs the board on a COM port.

---

## 1. Firmware: ESP-IDF

| Item | Version / location |
|---|---|
| ESP-IDF | **v6.1**, in `C:\esp\v6.1\esp-idf` (installed with the Espressif Installation Manager) |
| Compiler | `xtensa-esp32-elf-gcc` **15.2.0** (crosstool-NG `esp-15.2.0_20251204`) |
| Build tools | CMake 4.0.3 and Ninja 1.12.1, from `C:\Espressif\tools` |
| Python | 3.11 virtual environment for ESP-IDF |
| Flasher | esptool 5.4.0 |
| Shell setup | `C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1` |

Open PowerShell, run the shell-setup script, then from the project root:

```bash
idf.py build
```
```bash
idf.py -p COM15 flash
```
```bash
idf.py menuconfig
```

- `build` compiles into `build/` (git-ignored).
- `flash` builds if needed and writes the board on that COM port.
- `menuconfig` opens the settings (IGNACC Controller → Relays / UART link / Tasks).
  See [configuration-and-architecture.md](configuration-and-architecture.md).

ESP-IDF brings its own CMake (4.0.3). The unit tests use the system CMake
(4.3.3). Both work. Each shell simply finds a different one on PATH.

---

## 2. Unit tests (no hardware)

These are C tests that run on the PC against the real firmware sources, with
fakes in place of the hardware. Everything is covered in
**[unit-testing.md](unit-testing.md)**: install, build, how the tests are
linked, the mocks, and adding tests.

| Item | Version |
|---|---|
| Compiler | MinGW-w64 GCC **15.2.0** (`x86_64-posix-seh`), in `C:\Mingw64` |
| Build | CMake **4.3.3**, GNU Make **4.4.1** (`mingw32-make`), CTest |
| Framework | Unity **2.6.0** (from ESP-IDF) |

---

## 3. Hardware testing: IGNACC Tester

A **compiled Python app** that talks to the board over the serial port, the
same way CANoe/CAPL does. It sends commands, checks every reply, and measures
the round-trip time. Use it to check a board after flashing, to switch relays by
hand, or as a starting point for other serial-command test benches.

This section gives the overview. A full user and developer guide for the app
will follow.

### What it does

- **GUI:** pick the COM port, then connect. The status shows the firmware version from `HeyU`.
- **15 automated tests (98 steps):**
  - handshake, each relay ON/OFF, ALL ON/OFF (including from mixed states)
  - idempotent commands, line endings, unknown commands (`UNK_NACK`)
  - overlong lines, pipelined commands, latency, and reset while all relays are ON
- **How to run them:** all of them, only the ticked ones, or a range. Choose the delay between steps (so you can watch the relays) and a latency limit (default 20 ms).
- **Manual control:** HeyU, STATUS, IN1/IN2 ON/OFF, ALL ON/OFF, or any custom line. Indicators show each relay's state.
- **Safety:** every relay is switched OFF after a run, on Stop, and when the window closes.
- **Command line:** the same tests without the GUI, for scripts or CI.

### Get it and run it

| Option | How |
|---|---|
| **Compiled `.exe`** (no Python needed) | [tools/ignacc_tester/bin/v1.2.0/IGNACC_Tester-v1.2.0-windows-x64.exe](../tools/ignacc_tester/bin/v1.2.0/) |
| Command line, from source | `python tools/ignacc_tester/tester_core.py --port COM15` (add `--tests 7-9`, `--delay 0.2`, `--latency-limit 20 --exclude-settle`; `--port COM15 --list` lists the tests) |

Close PuTTY, Serial Studio or CANoe first: only one program can hold the COM port.

Use the tester version that matches the firmware:

| Firmware | Tester | Adds |
|---|---|---|
| 1.2.0 | `bin/v1.2.0` | `UNK_NACK` tests |
| 1.1.0 | `bin/v1.1.0` | ALL ON / ALL OFF |
| 1.0.0 | `bin/v1.0.0` | first release |

### How it is built

| Item | Version |
|---|---|
| Python | 3.13 (with Tkinter 8.6 for the GUI) |
| Serial | pyserial 3.5 |
| Packaging | PyInstaller 6.11.1 (single-file, windowed `.exe`) |

```bash
powershell -ExecutionPolicy Bypass -File tools/ignacc_tester/build_exe.ps1
```

The script creates `tools/ignacc_tester/.venv` and installs the pinned
packages from `requirements.txt`. It writes `dist/IGNACC_Tester.exe`. To
publish a new version, copy that file to `bin/vX.Y.Z/` with a new
`SHA256SUMS.txt`.

### Adapting it

The app is two files, and most changes touch only one:

| File | Contains |
|---|---|
| `tester_core.py` | serial link, test runner and the list of tests. No GUI code |
| `ignacc_tester_gui.py` | the Tkinter window, which builds its test list from `tester_core.TESTS` |

| You want to… | Change |
|---|---|
| Add a test | Write a function that uses `ctx.expect(cmd, reply)`, `ctx.expect_nack(...)` or `ctx.expect_silent(...)`. Add one `TestCase(...)` line to `TESTS` in `tester_core.py`. It appears in the GUI and on the command line automatically |
| Add a manual button | Add one `(label, command)` pair to the `cmds` list in `ignacc_tester_gui.py` |
| Change timing limits | `SETTLE_MS`, `MAX_LINE_LEN`, `REPLY_TIMEOUT`, `BAUD` at the top of `tester_core.py`; the latency limit in the GUI |
| Test a different serial device | The `Link` class just sends a line and reads a `\r\n`-terminated reply, so any line-based ASCII protocol works. Replace the test functions, keep the runner and GUI |

Example: a new test that switches IN2 twice:

```python
def t_in2_twice(ctx):
    ctx.expect("IN2_ON", "IN2_ON_OK", min_ms=SETTLE_MS)
    ctx.expect("IN2_ON", "IN2_ON_OK", min_ms=SETTLE_MS)
    ctx.expect("IN2_OFF", "IN2_OFF_OK", min_ms=SETTLE_MS)

TESTS.append(TestCase("IN2 twice", "IN2_ON twice, then OFF", t_in2_twice))
```

`min_ms=SETTLE_MS` also checks that `_OK` arrives no sooner than the 15 ms
settle delay.

### Other hardware tools

| Tool | Use |
|---|---|
| `tools/pin_check.py` | Steps IN1/IN2 one at a time, so you can measure the pins with a multimeter |
| `tools/hil_test.py` | The original command-line hardware test (33 checks), kept as a second, independent check |

### Diagram

[module-tester.svg](diagrams/module-tester.svg) shows the app's structure:
GUI, `tester_core`, threads, build and committed binaries.

---

## 4. Documentation

| Item | Version | Command |
|---|---|---|
| Doxygen | 1.18.0 | `doxygen` in the project root, which writes `docs/api/html/index.html` |
| Graphviz | 16.1.0 | used by Doxygen for include and call graphs |
| Diagram generator | Python standard library | `python docs/diagrams/generate_diagrams.py` |

Install on Windows 11:

```bash
winget install --id DimitriVanHeesch.Doxygen -e
```
```bash
winget install --id Graphviz.Graphviz -e
```

The generated HTML is committed, so anyone can open `docs/api/html/index.html`
without installing Doxygen. Doxygen's cache files are git-ignored.

---

## 5. Editor and version control

- **VS Code + clangd:** Ctrl+Click navigation for firmware and test code, using
  esp-clangd 21.1.3. See [vscode-clangd-setup.md](vscode-clangd-setup.md).
- **Git with two GitHub accounts:** one `git push` updates both repos. See
  [github-two-accounts.md](github-two-accounts.md).
