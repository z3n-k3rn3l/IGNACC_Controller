# Unit testing

How the firmware's unit tests work: the framework, the compiler, how a test is
compiled and linked, how to install everything, and how to add a test.

## At a glance

| | |
|---|---|
| Framework | **Unity 2.6.0** (ThrowTheSwitch), the copy that ships with ESP-IDF v6.1 |
| Runs on | **The PC** (Windows 11 here; Linux works too) |
| Hardware needed | **None.** No ESP32, no relays, no COM port |
| Compiler | **MinGW-w64 GCC 15.2.0** (`x86_64-posix-seh-rev1`, MinGW-Builds), C11 |
| Build system | **CMake 4.3.3** (min 3.16) + **GNU Make 4.4.1** (`mingw32-make`), run by **CTest** |
| Location | [test/host/](../test/host) |
| Size | 5 test programs, **75 tests**, runs in under a second |
| Flags | `-Wall -Wextra -Werror`: a warning fails the build |

---

## 1. What is tested, and what isn't

The tests compile the firmware's **pure-C modules**, the ones with no ESP-IDF
calls, with the PC's own compiler. Hardware is replaced by small fakes (mocks).

| Test program | Tests | Module under test | Mocks it uses |
|---|---|---|---|
| `test_protocol` | 19 | `protocol.c`: parsing, replies, `UNK_NACK` | none |
| `test_line_assembler` | 12 | `line_assembler.c`: bytes to lines, overlong lines | none |
| `test_relay_gpio` | 9 | `relay_gpio.c`: OFF before output, read-back | fake GPIO (`pin_hal_t`) |
| `test_relay_bank` | 7 | `relay_bank.c` + the safe-pin list | fake relays (`relay_t`) |
| `test_app_fsm` | 28 | `app_fsm.c` (with the real `protocol.c` and `relay_bank.c`) | fake relays, fake UART link, fake delay |

**Not unit-tested,** because these files call ESP-IDF or FreeRTOS directly:
- `uart_link.c` (UART driver)
- `pin_hal_esp.c` (GPIO driver)
- `app_fsm_task.c` (FreeRTOS task)
- `main.c` (wiring)

They are thin by design, and the hardware tests cover them on the real
board. The hardware tests run in the **IGNACC Tester**, a compiled Python app
that drives the board over the serial port; see
[toolchain.md](toolchain.md), section 3. The full picture is three layers:

| Layer | What it catches | Needs hardware |
|---|---|---|
| Compile-time checks in `main.c` | bad pin configuration | no |
| **Unit tests (this page)** | logic errors, every failure path | **no** |
| IGNACC Tester (15 tests / 98 steps) | wiring, timing, the real UART and relays | yes (COM port) |

The unit tests are the only layer that can exercise failures a working board
never shows: a relay failing halfway through `ALL_ON`, a read-back lost after
the settle delay, 1 or 16 channels.

---

## 2. Install the tools (Windows 11)

You need three things: a C compiler, CMake, and the Unity sources.

### 2.1 C compiler: MinGW-w64 GCC

This PC uses the MinGW-Builds GCC 15.2.0 (64-bit, posix threads, seh) in
`C:\Mingw64`. Install one of:

- **MinGW-Builds (matches this PC):** download the `x86_64-…-release-posix-seh-ucrt-…`
  archive from <https://github.com/niXman/mingw-builds-binaries/releases>.
  Extract it to `C:\Mingw64`, then add `C:\Mingw64\bin` to your user PATH.
- **WinLibs via winget:** run `winget search winlibs` and install the
  `POSIX.UCRT` package.
- **MSYS2:** install it, then run `pacman -S mingw-w64-ucrt-x86_64-gcc` in the UCRT64 shell.

### 2.2 CMake

```bash
winget install --id Kitware.CMake -e
```

### 2.3 Unity

Nothing to install if ESP-IDF is installed: the CMake file uses
`$IDF_PATH/components/unity/unity/src`. The ESP-IDF PowerShell profile sets
`IDF_PATH` (`C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`).

Without ESP-IDF, get Unity on its own and point CMake at it:

```bash
git clone --depth 1 --branch v2.6.0 https://github.com/ThrowTheSwitch/Unity.git C:/tools/Unity
```

Then add `-DUNITY_DIR=C:/tools/Unity/src` to the configure command in section 3.

### 2.4 Check the installation

Open a **new** PowerShell, so it picks up the new PATH, and run:

```bash
gcc --version
```
```bash
mingw32-make --version
```
```bash
cmake --version
```

Expected: GCC 15.x (any recent GCC works), GNU Make 4.x, and CMake 3.16 or newer.

### Linux

```bash
sudo apt install build-essential cmake
```

Use Unity from ESP-IDF or the `git clone` above, and replace
`-G "MinGW Makefiles"` with `-G "Unix Makefiles"` (or leave `-G` out).

---

## 3. Build and run

From the project root, with `IDF_PATH` set (ESP-IDF shell) or with `-DUNITY_DIR=…`:

```bash
cmake -S test/host -B test/host/build -G "MinGW Makefiles"
```
```bash
cmake --build test/host/build
```
```bash
ctest --test-dir test/host/build --output-on-failure
```

- **Configure (`cmake -S … -B …`):** reads `test/host/CMakeLists.txt` and writes
  the makefiles into `test/host/build` (git-ignored). This is only needed once,
  or after changing the CMake file.
- **Build (`cmake --build`):** compiles everything. Any compiler warning stops the
  build, because of `-Werror`.
- **Test (`ctest`):** runs all 5 programs. `--output-on-failure` prints the
  Unity output of any program that fails.

Expected end of the output:

```
100% tests passed, 0 tests failed out of 5
```

CTest counts **programs** (5); Unity counts **tests** inside each (75 in total).

### Run one suite or see every test

```bash
ctest --test-dir test/host/build -R app_fsm -V
```

- `-R app_fsm` runs only the programs whose name matches.
- `-V` prints each test's PASS/FAIL line.

Each program is also a normal `.exe` you can run directly:

```bash
test/host/build/test_app_fsm.exe
```

```
...\test\host\test_app_fsm.c:571:test_init_state_ignores_lines:PASS
...\test\host\test_app_fsm.c:572:test_start_drives_all_off:PASS
...
28 Tests 0 Failures 0 Ignored
OK
```

Each line gives the file, the line of its `RUN_TEST`, the test name and PASS or FAIL.

---

## 4. How a test program is compiled and linked

[test/host/CMakeLists.txt](../test/host/CMakeLists.txt) builds one library and
five programs:

```
unity.c ──────────────► libunity.a          (static library, built once)

test_app_fsm.c ─┐
app_fsm.c ──────┤
protocol.c ─────┼──► test_app_fsm.exe ◄── libunity.a
relay_bank.c ───┘
```

Every test program is created by the `add_unit_test()` helper in the CMake file:

| Step | What `add_unit_test(name, test.c, sources…)` does |
|---|---|
| Compile | builds the test file **plus the real firmware `.c` files it tests**, straight from `components/` (no copies) |
| Headers | adds the 5 `components/*/include` folders, the same headers the firmware uses |
| Flags | `-std=gnu11` (C11), `-Wall -Wextra -Werror` |
| Link | links `libunity.a` (Unity's `UNITY_BEGIN`, `RUN_TEST`, `TEST_ASSERT_*`) |
| Register | `add_test()` makes the program known to CTest |

What each program links:

| Program | Test file | Firmware sources compiled in |
|---|---|---|
| `test_protocol` | `test_protocol.c` | `protocol.c` |
| `test_line_assembler` | `test_line_assembler.c` | `line_assembler.c` |
| `test_relay_gpio` | `test_relay_gpio.c` | `relay_gpio.c` |
| `test_relay_bank` | `test_relay_bank.c` | `relay_bank.c` |
| `test_app_fsm` | `test_app_fsm.c` | `app_fsm.c`, `protocol.c`, `relay_bank.c` |

No ESP-IDF header and no FreeRTOS code is ever compiled here. That is possible
only because of the firmware's design: those modules talk to hardware
exclusively through injected interfaces (`relay_t`, `pin_hal_t`, `link_tx_t`
and a delay function).

The configure step also writes `test/host/build/compile_commands.json`, which
clangd uses for code navigation in the test files (see
[vscode-clangd-setup.md](vscode-clangd-setup.md)).

---

## 5. How the mocks work

No mocking library is used. A mock is just another implementation of the
module's interface, written in the test file.

For example, the fake GPIO in `test_relay_gpio.c`:

```c
static int mock_set_level(void *ctx, int pin, int level)
{
    mock_hal_t *m = ctx;
    m->set_calls++;          /* record what happened */
    if (m->fail_set) {       /* fail on demand */
        return -1;
    }
    m->level[pin] = level;
    return 0;
}

static const pin_hal_ops_t s_mock_ops = { mock_configure_output, mock_set_level, mock_get_level };
static pin_hal_t s_hal = { .ops = &s_mock_ops, .ctx = &s_mock };   /* injected into relay_gpio_create() */
```

The same pattern gives `test_app_fsm.c` fake relays, a fake UART link and a fake
delay. Each logs its calls into one shared **event log**, so a test can check the
**order** of what happened, for example: drive all relays, then exactly one
15 ms delay, then one reply.

| Mock feature | Used to test |
|---|---|
| `fail_with` / `fail_set` flags | `_FAIL` replies and the all-OFF safe state |
| `stuck[pin]`, `drops_after_settle` | read-back mismatch, a relay dropping during the settle delay |
| call counters | idempotent commands, "every relay attempted even after a failure" |
| event log | set → delay → send ordering; "no delay before `_FAIL`" |

Unity calls `setUp()` before and `tearDown()` after every test, so every test
starts from a fresh state.

---

## 6. Adding a test

**To an existing suite:** write a `static void test_…(void)` function using
`TEST_ASSERT_*` macros, and add `RUN_TEST(test_…);` to `main()` in the same file.
Rebuild and run (section 3); no CMake change is needed.

```c
static void test_status_after_all_on(void)
{
    start_ready();
    line("ALL_ON");
    clear_logs();
    line("STATUS");
    TEST_ASSERT_EQUAL_STRING("STATUS IN1=ON IN2=ON\r\n", s_out);
}
```

**A new suite** (for a new module):
1. Create `test/host/test_<module>.c` with `setUp()`, `tearDown()`, the tests,
   and a `main()` that calls `UNITY_BEGIN()`, the `RUN_TEST`s and `return UNITY_END();`.
2. Add one line to `test/host/CMakeLists.txt`:
   ```cmake
   add_unit_test(test_<module>  test_<module>.c  "${COMP}/<component>/<module>.c")
   ```
3. Configure, build and run again (section 3).

The module must be pure C to be unit-testable. If it needs hardware, give it an
interface (like `pin_hal_t`) and test it through a mock.

Common Unity assertions: `TEST_ASSERT_TRUE/FALSE`, `TEST_ASSERT_EQUAL(_INT/_UINT)`,
`TEST_ASSERT_EQUAL_STRING`, `TEST_ASSERT_EQUAL_MEMORY`, `TEST_ASSERT_NULL/NOT_NULL`.
The full list is in `unity.h`.

---

## 7. Troubleshooting

| Problem | Cause | Fix |
|---|---|---|
| `Set IDF_PATH or pass -DUNITY_DIR=…` | Unity not found | run the ESP-IDF PowerShell profile first, or use `-DUNITY_DIR` (section 2.3) |
| `'gcc' is not recognized` / CMake finds no compiler | MinGW `bin` not on PATH | add `C:\Mingw64\bin` to PATH, open a new shell |
| `CMAKE_MAKE_PROGRAM is not set` | `mingw32-make` not on PATH | same as above (it ships with MinGW) |
| CMake complains about a different generator | `test/host/build` was made with another generator | delete `test/host/build` and configure again |
| A warning stops the build | `-Werror` | fix the warning; warnings are treated as errors on purpose |
| `ctest` says `No tests were found` | ran from the wrong folder | use `--test-dir test/host/build` |
| A test fails | real bug, or a changed rule | read the `file:line:test_name:FAIL: …` line; run the `.exe` directly for full output |
