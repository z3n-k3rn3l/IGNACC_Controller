# VS Code code navigation (clangd) for ESP-IDF projects

This guide sets up **Ctrl+Click / Go to Definition, hover, autocomplete and
error squiggles** for this project in VS Code, using **clangd**. It is written
for Windows with ESP-IDF **v6.1** installed by the Espressif Installation
Manager (EIM), and works for any ESP-IDF project.

Tested setup (2026-10-26):

| Item | Value |
|---|---|
| VS Code extension | clangd `llvm-vs-code-extensions.vscode-clangd` 0.6.0 |
| clangd binary | `esp-clangd` esp-21.1.3_20260408 (ships with ESP-IDF v6.1) |
| Builtin headers | `esp-clang` esp-21.1.3_20260408 (same version as esp-clangd) |
| Firmware compiler | `xtensa-esp32-elf-gcc` esp-15.2.0_20251204 |
| Host test compiler | MinGW gcc 15.2 (`C:\Mingw64\bin`) |
| ESP-IDF | `C:\esp\v6.1\esp-idf`, tools in `C:\Espressif\tools` |

---

## How it fits together

```
idf.py build ──> build/compile_commands.json   (exact compiler flags for every .c file)
                         │
.clangd ─────────────────┤  "use build/ for this folder, drop GCC-only flags"
                         ▼
VS Code ──> clangd extension ──> esp-clangd.exe
                                   ├─ --query-driver  -> asks xtensa gcc / MinGW gcc for system headers
                                   └─ --resource-dir  -> builtin headers (stddef.h, stdbool.h, ...)
```

clangd needs three things:

1. **The compile command for each file**: `compile_commands.json`.
2. **The system headers of the real compiler**, from `--query-driver`.
3. **Its own builtin headers**, from `--resource-dir`. The `esp-clangd` package
   does not include them, so we borrow them from `esp-clang`.

If any one of them is missing, navigation breaks.

---

## Step 1: Install the clangd extension

1. Press **Ctrl+Shift+X** to open the Extensions view.
2. Search for **clangd**.
3. Install **clangd** by **LLVM** (`llvm-vs-code-extensions.vscode-clangd`).

If the extension offers to **download a clangd binary**, click **No** (or
ignore it). We use Espressif's `esp-clangd`, because it understands the Xtensa
and RISC-V ESP targets.

## Step 2: Make sure the extension is ENABLED

This was the last missing piece when the setup was first done here.

1. In Extensions, search **clangd** again and open it.
2. If the button says **Enable** or **Enable (Workspace)**, click it, then
   **Restart Extensions** or reload the window.
3. Correct state: the buttons read **Disable** and **Uninstall**.

## Step 3: Turn off Microsoft C/C++ IntelliSense

The Microsoft C/C++ extension (`ms-vscode.cpptools`) and clangd conflict if
both provide IntelliSense. Keep cpptools installed if you use it for
debugging, but disable its IntelliSense engine.

- **Ctrl+,** (Settings) → search `C_Cpp.intelliSenseEngine` → set to **disabled**,
- or let Step 6 set it for this workspace only.

The clangd extension usually offers to do this itself; accepting is fine.

## Step 4: Check the clangd binary and builtin headers exist

`esp-clangd` is installed with ESP-IDF (tools.json marks it `always`).
`esp-clang` is **on request**, so it may be missing on a new PC.

Check in PowerShell:

```bash
dir C:\Espressif\tools\esp-clangd\*\esp-clangd\bin\clangd.exe
```
```bash
dir C:\Espressif\tools\esp-clang\*\esp-clang\lib\clang\*\include\stddef.h
```

If the second command finds nothing, install `esp-clang` from an ESP-IDF
PowerShell (run `C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1` first):

```bash
python $env:IDF_PATH\tools\idf_tools.py install esp-clang
```

Note the **version folder names** (here `esp-21.1.3_20260408`) and the
**clang major version** under `lib\clang\` (here `21`). You need them in
Step 6. `esp-clangd` and `esp-clang` should be the same version.

## Step 5: Generate the compile database

From an ESP-IDF PowerShell in the project folder:

```bash
idf.py build
```

This writes `build/compile_commands.json`. Run `idf.py reconfigure` instead if
you only want the database without compiling. Repeat after adding or
removing source files or components.

For the **host unit tests** (`test/host`), the CMake file sets
`CMAKE_EXPORT_COMPILE_COMMANDS ON`, so configuring them writes
`test/host/build/compile_commands.json`:

```bash
cmake -S test/host -B test/host/build -G "MinGW Makefiles"
```

## Step 6: Workspace settings: `.vscode/settings.json`

Create or update `.vscode/settings.json` in the project root. This file is
machine-specific (absolute paths) and is git-ignored, so **this section is the
reference copy**:

```jsonc
{
    "idf.currentSetup": "C:\\esp\\v6.1\\esp-idf",

    "clangd.path": "C:\\Espressif\\tools\\esp-clangd\\esp-21.1.3_20260408\\esp-clangd\\bin\\clangd.exe",
    "clangd.arguments": [
        "--background-index",
        "--query-driver=**/xtensa-esp*-elf-gcc*,C:/Mingw64/bin/*",
        "--resource-dir=C:/Espressif/tools/esp-clang/esp-21.1.3_20260408/esp-clang/lib/clang/21",
        "--header-insertion=never"
    ],
    "C_Cpp.intelliSenseEngine": "disabled"
}
```

What each line does:

| Setting | Why |
|---|---|
| `idf.currentSetup` | Tells the ESP-IDF extension to use v6.1 for this workspace. It overrides older global settings that point at v5.3.1. |
| `clangd.path` | Use Espressif's clangd instead of a generic one. |
| `--background-index` | Index the whole project so Go to Definition finds function bodies in other files, not just declarations. |
| `--query-driver=...` | Allows clangd to run these compilers to learn their system include paths and target. Without it, `string.h` and `stdio.h` are not found. Patterns are comma-separated globs. The host tests' compile command uses `C:\Mingw64\bin\cc.exe`, so the MinGW pattern is `*`, not just `gcc*`. |
| `--resource-dir=...` | Where clang's builtin headers live (`stddef.h`, `stdbool.h`, `stdint.h`, `float.h`). `esp-clangd` ships only `bin\`, so we point at `esp-clang`. Without it, every file shows errors like `'stddef.h' file not found` and `use of undeclared identifier 'NULL'`. |
| `--header-insertion=never` | Stop clangd auto-adding `#include` lines while you type. |
| `C_Cpp.intelliSenseEngine` | Keeps Microsoft IntelliSense off in this workspace (see Step 3). |

## Step 7: Project file: `.clangd` (committed)

At the project root, `.clangd`:

```yaml
CompileFlags:
  CompilationDatabase: build
  Remove: [-m*, -f*]
  Add: [-Wdocumentation]

Diagnostics:
  UnusedIncludes: None
```

- **`CompilationDatabase: build`**: use `build/compile_commands.json` for all
  files under this folder.
- **`Remove: [-m*, -f*]`**: drop GCC-only flags clang doesn't understand,
  such as `-mlongcalls`, `-fstrict-volatile-bitfields` and
  `-fno-tree-switch-conversion`. This is Espressif's recommended setting.
- **`Add: [-Wdocumentation]`**: flags Doxygen mistakes as you type, e.g. a
  `@param` naming a parameter that doesn't exist.
- **`UnusedIncludes: None`**: no "unused include" warnings. ESP-IDF headers
  trigger many false positives.

In `test/host/.clangd` (committed), the host tests use their own database:

```yaml
CompileFlags:
  CompilationDatabase: build
```

The `build` path is relative to the folder of the `.clangd` file, so this
means `test/host/build/`.

## Step 8: Restart clangd and verify

1. **Ctrl+Shift+P** → **clangd: Restart language server**, or
   **Developer: Reload Window**.
2. Open `main/main.c`.
3. **View → Output**, then pick **clangd** in the dropdown. You should see:
   ```
   Loaded compilation database from C:/Users/.../IGNACC_Controller/build/compile_commands.json
   System include extraction: target extracted: "xtensa-esp-elf"
   ```
4. **Ctrl+Click** these. Each should open the file shown:

   | In file | Click on | Opens |
   |---|---|---|
   | `main/main.c` | `relay_gpio_create` | `components/relay/include/relay_gpio.h` |
   | `main/main.c` | `CONFIG_IGNACC_RELAY1_GPIO` | `build/config/sdkconfig.h` |
   | `main/main.c` | `xQueueCreate` | ESP-IDF FreeRTOS `queue.h` |
   | `components/uart_link/uart_link.c` | `uart_driver_install` | ESP-IDF `driver/uart.h` |
   | `test/host/test_app_fsm.c` | `app_fsm_handle_line` | `components/app_fsm/app_fsm.c` |

   The first Ctrl+Click on a function may land on its header declaration.
   Once the background index finishes (a few seconds, shown in the status
   bar), it jumps to the function body.

### Optional: verify from the command line

`clangd --check` parses a file exactly like the editor and prints any errors:

```bash
C:\Espressif\tools\esp-clangd\esp-21.1.3_20260408\esp-clangd\bin\clangd.exe "--query-driver=**/xtensa-esp*-elf-gcc*,C:/Mingw64/bin/*" "--resource-dir=C:/Espressif/tools/esp-clang/esp-21.1.3_20260408/esp-clang/lib/clang/21" --check=main\main.c
```

The last line should read `All checks completed, 0 errors`. Lines starting with
`tweak: ... FAIL` are clangd testing its refactoring actions, not code errors.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Ctrl+Click does nothing; no **clangd** entry in the Output dropdown | Extension not installed, disabled, or no `.c` file opened | Steps 1–2, then open a `.c` file |
| Output shows clangd trying to download, or `clangd not found` | `clangd.path` missing or wrong | Step 6; check the path with Step 4 |
| `'stddef.h' file not found`, `undeclared identifier 'NULL'`, `size_t` unknown | No builtin headers (`--resource-dir` missing or wrong) | Steps 4 and 6 |
| `'string.h'` / `'stdio.h'` not found | `--query-driver` does not match the compiler in `compile_commands.json` | Open the database, look at the compiler path of a failing file, adjust the glob |
| Everything red and "unknown argument `-mlongcalls`" | `.clangd` missing, or `Remove` line missing | Step 7 |
| `sdkconfig.h` / `CONFIG_...` not found, or a file "has no compile command" | Project not built yet, or the file is new | Step 5, then restart clangd |
| Two sets of squiggles, or IntelliSense fighting clangd | Microsoft IntelliSense still enabled | Step 3 |
| Works in firmware files but not in `test/host` files | Host tests not configured, so there's no `test/host/build/compile_commands.json` | Step 5 (host test configure) |
| Goes to the declaration instead of the body | Background index still running | Wait for indexing to finish in the status bar |

## After upgrading ESP-IDF or the tools

The version folder names change, so update the paths:

1. Find the new folders:
   ```bash
   dir C:\Espressif\tools\esp-clangd
   ```
   ```bash
   dir C:\Espressif\tools\esp-clang
   ```
2. Update `clangd.path` and `--resource-dir` in `.vscode/settings.json`.
   Check the clang major version folder (`lib\clang\21` may become `22`).
3. Update `idf.currentSetup` to the new ESP-IDF path.
4. Run `idf.py fullclean`, then `idf.py build`, then **clangd: Restart language server**.

## Using this setup in another ESP-IDF project

Copy `.clangd` and `.vscode/settings.json` into the other project. Drop the
MinGW part of `--query-driver` if it has no host tests. Then do Step 5 and
Step 8.
