/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "IGNACC Controller", "index.html", [
    [ "IGNACC_Controller", "index.html", "index" ],
    [ "Boot and flashing", "md_docs_2boot-and-flashing.html", [
      [ "1. In plain words", "md_docs_2boot-and-flashing.html#autotoc_md17", null ],
      [ "2. What is in the flash", "md_docs_2boot-and-flashing.html#autotoc_md19", null ],
      [ "3. The boot chain", "md_docs_2boot-and-flashing.html#autotoc_md21", null ],
      [ "4. How <span class=\"tt\">idf.py flash</span> works", "md_docs_2boot-and-flashing.html#autotoc_md23", null ],
      [ "5. UART0 is shared: what this project changed", "md_docs_2boot-and-flashing.html#autotoc_md25", [
        [ "What is different from a default ESP-IDF project", "md_docs_2boot-and-flashing.html#autotoc_md26", null ],
        [ "What is <b>not</b> changed (and can't be from software)", "md_docs_2boot-and-flashing.html#autotoc_md27", null ],
        [ "Getting logs back for debugging", "md_docs_2boot-and-flashing.html#autotoc_md28", null ]
      ] ]
    ] ],
    [ "Code tour", "md_docs_2code-tour.html", [
      [ "Before you start (5 min)", "md_docs_2code-tour.html#autotoc_md31", null ],
      [ "Stop 1 — <span class=\"tt\">main</span>: where everything is built (10 min)", "md_docs_2code-tour.html#autotoc_md33", null ],
      [ "Stop 2 — <span class=\"tt\">relay</span>: switching one pin, safely (10 min)", "md_docs_2code-tour.html#autotoc_md35", null ],
      [ "Stop 3 — <span class=\"tt\">uart_link</span>: bytes in, lines out (8 min)", "md_docs_2code-tour.html#autotoc_md37", null ],
      [ "Stop 4 — <span class=\"tt\">app_fsm</span>: the decision maker (10 min)", "md_docs_2code-tour.html#autotoc_md39", null ],
      [ "Stop 5 — <span class=\"tt\">protocol</span>: text to command, result to text (5 min)", "md_docs_2code-tour.html#autotoc_md41", null ],
      [ "Recap: follow <span class=\"tt\">IN1_ON</span> from end to end", "md_docs_2code-tour.html#autotoc_md43", null ],
      [ "After the tour", "md_docs_2code-tour.html#autotoc_md45", null ]
    ] ],
    [ "Configuration and architecture", "md_docs_2configuration-and-architecture.html", [
      [ "1. The big picture", "md_docs_2configuration-and-architecture.html#autotoc_md48", null ],
      [ "2. Where configuration lives", "md_docs_2configuration-and-architecture.html#autotoc_md50", null ],
      [ "3. The relay channel model (what's different here)", "md_docs_2configuration-and-architecture.html#autotoc_md52", [
        [ "3.1 Kconfig: a count plus one pin per channel", "md_docs_2configuration-and-architecture.html#autotoc_md53", null ],
        [ "3.2 main.c: one list, three compile-time checks", "md_docs_2configuration-and-architecture.html#autotoc_md54", null ],
        [ "3.3 Why 14 is the maximum", "md_docs_2configuration-and-architecture.html#autotoc_md55", null ],
        [ "3.4 How the rest of the firmware adapts", "md_docs_2configuration-and-architecture.html#autotoc_md56", null ]
      ] ],
      [ "4. Other settings", "md_docs_2configuration-and-architecture.html#autotoc_md58", null ],
      [ "5. Extending without modifying the core", "md_docs_2configuration-and-architecture.html#autotoc_md60", null ],
      [ "6. Tests that guard all this", "md_docs_2configuration-and-architecture.html#autotoc_md62", null ],
      [ "7. API reference", "md_docs_2configuration-and-architecture.html#autotoc_md64", null ]
    ] ],
    [ "ESP-IDF primer", "md_docs_2esp-idf-primer.html", [
      [ "1. What is ESP-IDF?", "md_docs_2esp-idf-primer.html#autotoc_md96", null ],
      [ "2. Install it", "md_docs_2esp-idf-primer.html#autotoc_md98", null ],
      [ "3. From source to firmware", "md_docs_2esp-idf-primer.html#autotoc_md100", null ],
      [ "4. Kconfig: how settings reach the code", "md_docs_2esp-idf-primer.html#autotoc_md102", null ],
      [ "5. FreeRTOS: why it's there, and how this project uses it", "md_docs_2esp-idf-primer.html#autotoc_md104", [
        [ "What it is", "md_docs_2esp-idf-primer.html#autotoc_md105", null ],
        [ "Why ESP-IDF is built on it", "md_docs_2esp-idf-primer.html#autotoc_md106", null ],
        [ "What Espressif changed", "md_docs_2esp-idf-primer.html#autotoc_md107", null ],
        [ "Why this project needs it", "md_docs_2esp-idf-primer.html#autotoc_md108", null ]
      ] ],
      [ "6. More from Espressif", "md_docs_2esp-idf-primer.html#autotoc_md110", null ]
    ] ],
    [ "ESP32 primer", "md_docs_2esp32-primer.html", [
      [ "1. What is an ESP32?", "md_docs_2esp32-primer.html#autotoc_md113", null ],
      [ "2. The chip on this board", "md_docs_2esp32-primer.html#autotoc_md115", [
        [ "Memory", "md_docs_2esp32-primer.html#autotoc_md116", null ],
        [ "Peripherals (summary)", "md_docs_2esp32-primer.html#autotoc_md117", null ],
        [ "Pins you must know about", "md_docs_2esp32-primer.html#autotoc_md118", null ]
      ] ],
      [ "3. What is open source, and what isn't", "md_docs_2esp32-primer.html#autotoc_md120", null ],
      [ "4. Official documents", "md_docs_2esp32-primer.html#autotoc_md122", null ]
    ] ],
    [ "Two GitHub accounts, one computer, one repo", "md_docs_2github-two-accounts.html", [
      [ "The idea in one minute", "md_docs_2github-two-accounts.html#autotoc_md125", null ],
      [ "Step 1: One SSH key per account", "md_docs_2github-two-accounts.html#autotoc_md127", null ],
      [ "Step 2: Lock down the key files", "md_docs_2github-two-accounts.html#autotoc_md129", [
        [ "Windows", "md_docs_2github-two-accounts.html#autotoc_md130", null ],
        [ "Linux", "md_docs_2github-two-accounts.html#autotoc_md131", null ]
      ] ],
      [ "Step 3: <span class=\"tt\">~/.ssh/config</span>, which key for which account", "md_docs_2github-two-accounts.html#autotoc_md133", null ],
      [ "Step 4: Test both accounts", "md_docs_2github-two-accounts.html#autotoc_md135", null ],
      [ "Step 5: <span class=\"tt\">.gitconfig</span>, who the commits say they're from", "md_docs_2github-two-accounts.html#autotoc_md137", [
        [ "Optional: a different identity per folder", "md_docs_2github-two-accounts.html#autotoc_md138", null ]
      ] ],
      [ "Step 6: Connect the repo to both accounts", "md_docs_2github-two-accounts.html#autotoc_md140", [
        [ "Add the remotes", "md_docs_2github-two-accounts.html#autotoc_md141", null ],
        [ "First push", "md_docs_2github-two-accounts.html#autotoc_md142", null ],
        [ "Make one <span class=\"tt\">git push</span> update both", "md_docs_2github-two-accounts.html#autotoc_md143", null ]
      ] ],
      [ "Everyday use", "md_docs_2github-two-accounts.html#autotoc_md145", null ],
      [ "Setting this up on Linux, start to finish", "md_docs_2github-two-accounts.html#autotoc_md147", null ],
      [ "Passphrases", "md_docs_2github-two-accounts.html#autotoc_md149", null ],
      [ "Troubleshooting", "md_docs_2github-two-accounts.html#autotoc_md151", null ],
      [ "Undo", "md_docs_2github-two-accounts.html#autotoc_md153", null ]
    ] ],
    [ "Toolchain", "md_docs_2toolchain.html", [
      [ "Overview", "md_docs_2toolchain.html#autotoc_md155", [
        [ "Before every commit", "md_docs_2toolchain.html#autotoc_md156", null ]
      ] ],
      [ "1. Firmware: ESP-IDF", "md_docs_2toolchain.html#autotoc_md158", null ],
      [ "2. Unit tests (no hardware)", "md_docs_2toolchain.html#autotoc_md160", null ],
      [ "3. Hardware testing: IGNACC Tester", "md_docs_2toolchain.html#autotoc_md162", [
        [ "What it does", "md_docs_2toolchain.html#autotoc_md163", null ],
        [ "Get it and run it", "md_docs_2toolchain.html#autotoc_md164", null ],
        [ "How it is built", "md_docs_2toolchain.html#autotoc_md165", null ],
        [ "Adapting it", "md_docs_2toolchain.html#autotoc_md166", null ],
        [ "Other hardware tools", "md_docs_2toolchain.html#autotoc_md167", null ],
        [ "Diagram", "md_docs_2toolchain.html#autotoc_md168", null ]
      ] ],
      [ "4. Documentation", "md_docs_2toolchain.html#autotoc_md170", null ],
      [ "5. Editor and version control", "md_docs_2toolchain.html#autotoc_md172", null ]
    ] ],
    [ "Unit testing", "md_docs_2unit-testing.html", [
      [ "At a glance", "md_docs_2unit-testing.html#autotoc_md174", null ],
      [ "1. What is tested, and what isn't", "md_docs_2unit-testing.html#autotoc_md176", null ],
      [ "2. Install the tools (Windows 11)", "md_docs_2unit-testing.html#autotoc_md178", [
        [ "2.1 C compiler: MinGW-w64 GCC", "md_docs_2unit-testing.html#autotoc_md179", null ],
        [ "2.2 CMake", "md_docs_2unit-testing.html#autotoc_md180", null ],
        [ "2.3 Unity", "md_docs_2unit-testing.html#autotoc_md181", null ],
        [ "2.4 Check the installation", "md_docs_2unit-testing.html#autotoc_md182", null ],
        [ "Linux", "md_docs_2unit-testing.html#autotoc_md183", null ]
      ] ],
      [ "3. Build and run", "md_docs_2unit-testing.html#autotoc_md185", [
        [ "Run one suite or see every test", "md_docs_2unit-testing.html#autotoc_md186", null ]
      ] ],
      [ "4. How a test program is compiled and linked", "md_docs_2unit-testing.html#autotoc_md188", null ],
      [ "5. How the mocks work", "md_docs_2unit-testing.html#autotoc_md190", null ],
      [ "6. Adding a test", "md_docs_2unit-testing.html#autotoc_md192", null ],
      [ "7. Troubleshooting", "md_docs_2unit-testing.html#autotoc_md194", null ]
    ] ],
    [ "VS Code code navigation (clangd) for ESP-IDF projects", "md_docs_2vscode-clangd-setup.html", [
      [ "How it fits together", "md_docs_2vscode-clangd-setup.html#autotoc_md197", null ],
      [ "Step 1: Install the clangd extension", "md_docs_2vscode-clangd-setup.html#autotoc_md199", null ],
      [ "Step 2: Make sure the extension is ENABLED", "md_docs_2vscode-clangd-setup.html#autotoc_md200", null ],
      [ "Step 3: Turn off Microsoft C/C++ IntelliSense", "md_docs_2vscode-clangd-setup.html#autotoc_md201", null ],
      [ "Step 4: Check the clangd binary and builtin headers exist", "md_docs_2vscode-clangd-setup.html#autotoc_md202", null ],
      [ "Step 5: Generate the compile database", "md_docs_2vscode-clangd-setup.html#autotoc_md203", null ],
      [ "Step 6: Workspace settings: <span class=\"tt\">.vscode/settings.json</span>", "md_docs_2vscode-clangd-setup.html#autotoc_md204", null ],
      [ "Step 7: Project file: <span class=\"tt\">.clangd</span> (committed)", "md_docs_2vscode-clangd-setup.html#autotoc_md205", null ],
      [ "Step 8: Restart clangd and verify", "md_docs_2vscode-clangd-setup.html#autotoc_md206", [
        [ "Optional: verify from the command line", "md_docs_2vscode-clangd-setup.html#autotoc_md207", null ]
      ] ],
      [ "Troubleshooting", "md_docs_2vscode-clangd-setup.html#autotoc_md209", null ],
      [ "After upgrading ESP-IDF or the tools", "md_docs_2vscode-clangd-setup.html#autotoc_md210", null ],
      [ "Using this setup in another ESP-IDF project", "md_docs_2vscode-clangd-setup.html#autotoc_md211", null ]
    ] ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "Globals", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"protocol_8h.html#a13462d1dc3fa041509533a4ff7b3d28da797faa4bd89bef4528ec7d455fd516d0"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';